#ifndef ____SP_THREAD____
#define ____SP_THREAD____
#pragma once

#include "../setup/init.hpp"
#include "../containers/array.hpp"
#include "../containers/pair.hpp"

#include <tuple>
#include <queue>
#include <functional>

#if defined(_WIN32) || defined(_WIN64)
    #include <sysinfoapi.h>
#else
    #include <pthread.h>
    #include <unistd.h> // sysconf
#endif

namespace sp {

// ------------------------------------------------------------
// Portable helper
// ------------------------------------------------------------
namespace detail {
    inline long get_num_processors() noexcept {
    #if defined(_WIN32) || defined(_WIN64)
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        return static_cast<long>(si.dwNumberOfProcessors > 0 ? si.dwNumberOfProcessors : 1);
    #elif defined(_SC_NPROCESSORS_ONLN)
        long n = sysconf(_SC_NPROCESSORS_ONLN);
        return n > 0 ? n : 1;
    #else
        return 1;
    #endif
    }
} // namespace detail

// ------------------------------------------------------------
// thread
// ------------------------------------------------------------
template <bool autoJoin = true>
class thread {
private:
    pthread_t _thread{};
    bool _is_joinable = false;

    template <typename F, typename... Args>
    struct ThreadData {
        F func;
        std::tuple<Args...> args;
        ThreadData(F&& f, Args&&... a)
            : func(sp::forward<F>(f)), args(sp::forward<Args>(a)...) {}
    };

public:
    thread(const thread&) = delete;
    thread& operator=(const thread&) = delete;

    template <typename Func, typename... Args>
    SP_FORCEINLINE thread(Func f, Args&&... args) {
        auto* data = new ThreadData<Func, Args...>(
            sp::forward<Func>(f), sp::forward<Args>(args)...);

        int result = pthread_create(&_thread, nullptr,
            [](void* raw_data) -> void* {
                auto* typed = static_cast<ThreadData<Func, Args...>*>(raw_data);
                std::apply(typed->func, typed->args);
                delete typed;
                return nullptr;
            }, data);

        if (result == 0) {
            _is_joinable = true;
        } else {
            delete data;
            _is_joinable = false;
        }
    }

    thread() = default;

    template <typename Func, typename... Args>
    SP_FORCEINLINE void run(Func f, Args&&... args) {
        auto* data = new ThreadData<Func, Args...>(
            sp::forward<Func>(f), sp::forward<Args>(args)...);

        int result = pthread_create(&_thread, nullptr,
            [](void* raw_data) -> void* {
                auto* typed = static_cast<ThreadData<Func, Args...>*>(raw_data);
                std::apply(typed->func, typed->args);
                delete typed;
                return nullptr;
            }, data);

        if (result == 0) {
            _is_joinable = true;
        } else {
            delete data;
            _is_joinable = false;
        }
    }

    SP_FORCEINLINE void join() {
        if (_is_joinable) {
            pthread_join(_thread, nullptr);
            _is_joinable = false;
        }
    }

    SP_FORCEINLINE void detach() {
        if (_is_joinable) {
            pthread_detach(_thread);
            _is_joinable = false;
        }
    }

    SP_FORCEINLINE ~thread() {
        SP_IF_CONSTEXPR(autoJoin) {
            join();
        }
    }
};

// ------------------------------------------------------------
// thread_pool
// ------------------------------------------------------------
class thread_pool {
private:
    sp::array<pthread_t, 1> _threads;   // will be resized in constructor
    std::queue<std::function<void()>> _queue;
    size_t _active_tasks = 0;
    bool _end = false;
    pthread_mutex_t _lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t  _cond = PTHREAD_COND_INITIALIZER;
    pthread_cond_t  _wait_cond = PTHREAD_COND_INITIALIZER;

public:
    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

    void join_all() {
        pthread_mutex_lock(&_lock);
        _end = true;
        pthread_cond_broadcast(&_cond);
        pthread_mutex_unlock(&_lock);

        for (size_t i = 0; i < _threads.size(); ++i) {
            pthread_join(_threads[i], nullptr);
        }
    }

    void wait_all() {
        pthread_mutex_lock(&_lock);
        while (_active_tasks > 0) {
            pthread_cond_wait(&_wait_cond, &_lock);
        }
        pthread_mutex_unlock(&_lock);
    }

    // Default: use all available processors
    thread_pool() : thread_pool(detail::get_num_processors()) {}

    explicit thread_pool(long num_threads) {
        long max_available = detail::get_num_processors();
        long actual = (num_threads > 0) ? min(max_available, num_threads) : max_available;
        if (actual < 1) actual = 1;

        _threads = sp::array<pthread_t, 1>(static_cast<size_type>(actual));

        _active_tasks = 0;
        _end = false;

        for (size_t i = 0; i < _threads.size(); ++i) {
            pthread_create(&_threads[i], nullptr,
                [](void* arg) -> void* {
                    auto* self = static_cast<thread_pool*>(arg);
                    while (true) {
                        pthread_mutex_lock(&self->_lock);
                        while (self->_queue.empty() && !self->_end) {
                            pthread_cond_wait(&self->_cond, &self->_lock);
                        }
                        if (self->_end && self->_queue.empty()) {
                            pthread_mutex_unlock(&self->_lock);
                            break;
                        }
                        auto task = sp::move(self->_queue.front());
                        self->_queue.pop();
                        pthread_mutex_unlock(&self->_lock);

                        task();

                        pthread_mutex_lock(&self->_lock);
                        self->_active_tasks--;
                        if (self->_active_tasks == 0) {
                            pthread_cond_broadcast(&self->_wait_cond);
                        }
                        pthread_mutex_unlock(&self->_lock);
                    }
                    return nullptr;
                }, this);
        }
    }

    ~thread_pool() { join_all(); }

    template <typename F, typename... Args>
    SP_FORCEINLINE void enqueue(F&& f, Args&&... args) {
        auto task = [func = sp::forward<F>(f),
                     args_tuple = std::make_tuple(sp::forward<Args>(args)...)]() mutable {
            std::apply(sp::move(func), sp::move(args_tuple));
        };

        pthread_mutex_lock(&_lock);
        _active_tasks++;
        _queue.push(sp::move(task));
        pthread_cond_signal(&_cond);
        pthread_mutex_unlock(&_lock);
    }

    _SP_FUNC_NI_ size_type num_threads() const { return _threads.size(); }
    _SP_FUNC_NI_ size_type size()        const { return _threads.size(); }
    _SP_FUNC_NI_ bool      queue_empty() const { return _queue.empty(); }
    _SP_FUNC_NI_ size_type num_tasks_left() const { return _queue.size(); }
};

// Global pool (optional – keep if you use it)
static thread_pool sp_global_pool;

} // namespace sp

#endif // ____SP_THREAD____