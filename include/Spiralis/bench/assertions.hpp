#ifndef ____SP_BENCH_ASSERTIONS____
#define ____SP_BENCH_ASSERTIONS____
#pragma once

#include "arg_pack.hpp"

namespace sp {
namespace test {


// ============================================================
// ============================================================

template <typename... Args>
void assert(
    bool expected,
    bool result,
    const sp::string& expression,
    const sp::string& file,
    size_type line,
    const sp::string& argument_names,
    const arg_pack<Args...>& arguments
){
    if(result == expected) return;


    // --------------------------------------------------------
    // --------------------------------------------------------

    sp::print(
        sp::console::FG_BRIGHT_RED,
        "\n[*] ASSERTION ERROR:\n"
    );

    sp::print(
        sp::console::FG_BRIGHT_MAGENTA,
        "File: ",
        file,
        "\nLine: ",
        line,
        "\n"
    );

    sp::print(sp::console::RESET_EFFECTS);


    // --------------------------------------------------------
    // Expected / actual
    // --------------------------------------------------------

    sp::string true_string = ""_sp + sp::console::FG_BRIGHT_GREEN;

    true_string += "true";
    true_string += sp::console::RESET_EFFECTS;


    sp::string false_string = ""_sp + sp::console::FG_BRIGHT_RED;

    false_string += "false";
    false_string += sp::console::RESET_EFFECTS;


    sp::println(
        sp::console::BOLD,
        "  Condition: ",
        sp::console::RESET_EFFECTS,
        expression,

        sp::console::BOLD,
        "\n  Expected: ",
        sp::console::RESET_EFFECTS,
        expected ? true_string : false_string,

        sp::console::BOLD,
        "\n  Result: ",
        sp::console::RESET_EFFECTS,
        result ? true_string : false_string
    );


    // --------------------------------------------------------
    // --------------------------------------------------------

    SP_IF_CONSTEXPR(sizeof...(Args) > 0){
        sp::print(sp::console::FG_BRIGHT_CYAN);

        auto names = argument_names.split<sp::vector<sp::string>>(",");

        for(auto& name : names) name.trim();

        ull index = 0;

        arguments.apply([&](auto&& value){
            sp::string name = (index < names.size()) ? names[index] : sp::string("arg");
            ++index;
            sp::print(
                "  ",
                name,
                ": ",
                value,
                '\n'
            );
        });
        sp::println(sp::console::RESET_EFFECTS);
    }else{
        sp::println();
    }

    // --------------------------------------------------------
    // --------------------------------------------------------

    throw sp::exceptions::TestAssertionException("");
}

} // namespace test
} // namespace sp

#endif // ____SP_BENCH_ASSERTIONS____