#ifndef ____SP_BENCH_TEST_REGISTRY____
#define ____SP_BENCH_TEST_REGISTRY____
#pragma once

#include "../../setup/init.hpp"
#include "../../containers/string.hpp"
#include "../../io/IO.hpp"
#include "../../core/exceptions.hpp"

namespace sp {
namespace test {

// ============================================================
// ============================================================

struct test_case {
    const char* name;
    void (*function)();
    bool enabled = true;
};


// ============================================================
// ============================================================

struct test_run_result{
    size_type total = 0;
    size_type passed = 0;
    size_type failed = 0;
    size_type errors = 0;
};


// ============================================================
// ============================================================

struct test_registry_t{
private:
    sp::vector<test_case> test_cases;
public:
    // --------------------------------------------------------
    // --------------------------------------------------------

    SP_FORCEINLINE void add_test(const char* name, void (*function)(), bool enabled = true){
        test_cases.push_back({
            name,
            function,
            enabled
        });
    }


    // --------------------------------------------------------
    // --------------------------------------------------------

    test_run_result run(){
        test_run_result result;

        for(auto& test : test_cases){
            if(!test.enabled) continue;
            ++result.total;
            try{
                test.function();
                ++result.passed;
                sp::println(
                    sp::console::FG_BRIGHT_GREEN,
                    "[PASS] ",
                    test.name,
                    sp::console::RESET_EFFECTS
                );
            }

            // ------------------------------------------------
            // ------------------------------------------------

            catch(const sp::exceptions::TestAssertionException&){
                ++result.failed;
                sp::println(
                    sp::console::FG_BRIGHT_RED,
                    "[ASSERTION FAIL] ",
                    test.name,
                    "\nAborting test.",
                    sp::console::RESET_EFFECTS
                );
                break;
            }

            catch(const sp::exceptions::TestFailException&){
                ++result.failed;
                sp::println(
                    sp::console::FG_BRIGHT_RED,
                    "[FAIL] ",
                    test.name,
                    sp::console::RESET_EFFECTS
                );
            }


            // ------------------------------------------------
            // ------------------------------------------------
            // Unexpected Errors: Not thrown by assertion
            catch(...){
                ++result.errors;
                sp::println(
                    sp::console::FG_BRIGHT_YELLOW,
                    "[ERROR] ",
                    test.name,
                    sp::console::RESET_EFFECTS
                );
            }
        }
        // ----------------------------------------------------
        // ----------------------------------------------------
        sp::println(
            sp::console::FG_BRIGHT_MAGENTA,
            "\nTests: ",result.total,
            "\nPassed: ",result.passed,
            "\nFailed: ",result.failed,
            "\nErrors: ",result.errors,
            sp::console::RESET_EFFECTS
        );
        return result;
    }


    // --------------------------------------------------------
    // --------------------------------------------------------

    SP_FORCEINLINE const sp::vector<test_case>&
    get_tests() const{
        return test_cases;
    }
};

inline test_registry_t test_registry;


// ============================================================
// ============================================================

struct test_registration{

    test_registration(const char* name, void (*function)(), bool enabled = true){
        test_registry.add_test(
            name,
            function,
            enabled
        );
    }
};

} // namespace test
} // namespace sp

#endif // ____SP_BENCH_TEST_REGISTRY____