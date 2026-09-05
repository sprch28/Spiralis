#ifndef ____SP_THREAD____
#define ____SP_THREAD____
#pragma once

#include "../setup/init.hpp"
#include "../containers/array.hpp"
#include "../containers/pair.hpp"

#include <pthread.h>
#include <unistd.h>
#include <tuple>
#include <queue>
#include <functional>

// Better implementation soon to come

namespace sp{

template <bool autoJoin = true>
class thread{
private:
    pthread_t _thread;
    bool _is_joinable = false;

    template <typename F, typename... Args>
    struct ThreadData {
        F func;
        std::tuple<Args...> args;
        ThreadData(F&& f, Args&&... a) : func(sp::forward<F>(f)), args(sp::forward<Args>(a)...) {}
    };
public:
    thread(const thread&) = delete;
    thread& operator=(const thread&) = delete;
    template <typename Func, typename... Args>
    SP_FORCEINLINE thread(Func f, Args&&... args){
        // Package the function arguments
        auto data = new ThreadData<Func, Args...>(sp::forward<Func>(f), sp::forward<Args>(args)...);
        int result = pthread_create(&_thread, NULL, [](void* raw_data) -> void*{
            // Convert raw data back into its correct types
            auto* typed_data = static_cast<ThreadData<Func, Args...>*>(raw_data);
            // Unlock tuple, call function
            std::apply(typed_data->func, typed_data->args);
            delete typed_data;
            return NULL;
        }, data);

        if (result == 0) {
            _is_joinable = true;
        } else {
            delete data; // Handle creation failure
            _is_joinable = false;
        }
    }

    thread() {}

    template <typename Func, typename... Args>
    SP_FORCEINLINE void run(Func f, Args&&... args){
        auto data = new ThreadData<Func, Args...>(sp::forward<Func>(f), sp::forward<Args>(args)...);
        int result = pthread_create(&_thread, NULL, [](void* raw_data) -> void*{
            auto* typed_data = static_cast<ThreadData<Func, Args...>*>(raw_data);
            std::apply(typed_data->func, typed_data->args);
            delete typed_data;
            return NULL;
        }, data);

        if (result == 0) {
            _is_joinable = true;
        } else {
            delete data; // Handle creation failure
            _is_joinable = false;
        }
    }

    SP_FORCEINLINE void join(){
        if(_is_joinable) {
            pthread_join(_thread, NULL);
            _is_joinable = false; // Mark as no longer joinable
        }
    }

    SP_FORCEINLINE void detach(){
        pthread_detach(_thread);
        _is_joinable = false;
    }

    SP_FORCEINLINE ~thread(){
        SP_IF_CONSTEXPR(autoJoin){
            join();
        }
    }
};

class thread_pool{
private:
    sp::array<pthread_t, 1> _threads;
    std::queue<std::function<void()>> _queue;
    size_t _active_tasks = 0; // Track tasks in queue + tasks running
    bool _end = false;
    pthread_mutex_t _lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t _cond = PTHREAD_COND_INITIALIZER;
    pthread_cond_t _wait_cond = PTHREAD_COND_INITIALIZER;
public:
    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;
    void join_all(){
        pthread_mutex_lock(&_lock);
        _end = true;
        pthread_cond_broadcast(&_cond); // Wake up all threads to let them exit
        pthread_mutex_unlock(&_lock);

        for(size_t i = 0; i < _threads.size(); i++) {
            pthread_join(_threads[i], NULL);
        }
    }

    void wait_all() {
        pthread_mutex_lock(&_lock);
        // Use a while loop to handle spurious wakeups 
        // and ensure we don't wait if work is already done.
        while(_active_tasks > 0){
            pthread_cond_wait(&_wait_cond, &_lock);
        }
        pthread_mutex_unlock(&_lock);
    }
    // auto-init to max threads
    thread_pool() : thread_pool(sysconf(_SC_NPROCESSORS_ONLN)){}
    thread_pool(long num_threads) : _threads(min((long)sysconf(_SC_NPROCESSORS_ONLN), num_threads)){
        _active_tasks = 0;
        _end = false;
        size_t actual_threads = min((long)sysconf(_SC_NPROCESSORS_ONLN), num_threads);
        for(size_t i = 0; i < _threads.size(); i++) {
            pthread_create(&_threads[i], NULL, [](void* arg) -> void* {
                auto* self = static_cast<thread_pool*>(arg);
                while(true) {
                    pthread_mutex_lock(&self->_lock);
                    while(self->_queue.empty() && !self->_end){
                        pthread_cond_wait(&self->_cond, &self->_lock);
                    }
                    if(self->_end && self->_queue.empty()) {
                        pthread_mutex_unlock(&self->_lock);
                        break;
                    }
                    auto task = sp::move(self->_queue.front());
                    self->_queue.pop();
                    pthread_mutex_unlock(&self->_lock);
                    task();
                    pthread_mutex_lock(&self->_lock);
                    self->_active_tasks--;
                    if(self->_active_tasks == 0){
                        pthread_cond_broadcast(&self->_wait_cond);
                    }
                    pthread_mutex_unlock(&self->_lock);
                }
                return NULL;
            }, this);
        }
    }
    ~thread_pool(){ join_all(); }

    template <typename F, typename... Args>
    SP_FORCEINLINE void enqueue(F&& f, Args&&... args) {
        auto task = [f = sp::forward<F>(f), args_tuple = std::make_tuple(sp::forward<Args>(args)...)]() mutable {
            std::apply(sp::move(f), sp::move(args_tuple));
        };
        pthread_mutex_lock(&_lock);
        _active_tasks++;
        _queue.push(sp::move(task));
        pthread_cond_signal(&_cond); 
        pthread_mutex_unlock(&_lock);
    }

    _SP_FUNC_NI_ size_type num_threads() { return _threads.size(); }
    _SP_FUNC_NI_ size_type size() { return _threads.size(); }
    _SP_FUNC_NI_ size_type queue_empty() { return _queue.empty(); }
    _SP_FUNC_NI_ size_type num_tasks_left() { return _queue.size(); }
};

static thread_pool sp_global_pool;




}

#endif
