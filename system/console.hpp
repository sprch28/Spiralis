#ifndef ____SP_CONSOLE_HPP____
#define ____SP_CONSOLE_HPP____
#pragma once
#include "../setup/init.hpp"

namespace sp {
namespace console {

// Reset
constexpr const char* RESET_EFFECTS = "\033[0m";

// Text Styles
constexpr const char* BOLD = "\033[1m";
constexpr const char* DIM = "\033[2m";
constexpr const char* ITALIC = "\033[3m";
constexpr const char* UNDERLINE = "\033[4m";
constexpr const char* BLINK = "\033[5m";
constexpr const char* BLINK_FAST = "\033[6m";
constexpr const char* REVERSE = "\033[7m";
constexpr const char* HIDDEN = "\033[8m";
constexpr const char* STRIKETHROUGH = "\033[9m";

// Reset Individual Styles
constexpr const char* RESET_BOLD = "\033[21m";
constexpr const char* RESET_DIM = "\033[22m";
constexpr const char* RESET_ITALIC = "\033[23m";
constexpr const char* RESET_UNDERLINE = "\033[24m";
constexpr const char* RESET_BLINK = "\033[25m";
constexpr const char* RESET_REVERSE = "\033[27m";
constexpr const char* RESET_HIDDEN = "\033[28m";
constexpr const char* RESET_STRIKETHROUGH = "\033[29m";

// Foreground Colors (Standard)
constexpr const char* FG_BLACK = "\033[30m";
constexpr const char* FG_RED = "\033[31m";
constexpr const char* FG_GREEN = "\033[32m";
constexpr const char* FG_YELLOW = "\033[33m";
constexpr const char* FG_BLUE = "\033[34m";
constexpr const char* FG_MAGENTA = "\033[35m";
constexpr const char* FG_CYAN = "\033[36m";
constexpr const char* FG_WHITE = "\033[37m";
constexpr const char* FG_DEFAULT = "\033[39m";

// Foreground Colors (Bright/High Intensity)
constexpr const char* FG_BRIGHT_BLACK = "\033[90m";
constexpr const char* FG_BRIGHT_RED = "\033[91m";
constexpr const char* FG_BRIGHT_GREEN = "\033[92m";
constexpr const char* FG_BRIGHT_YELLOW = "\033[93m";
constexpr const char* FG_BRIGHT_BLUE = "\033[94m";
constexpr const char* FG_BRIGHT_MAGENTA = "\033[95m";
constexpr const char* FG_BRIGHT_CYAN = "\033[96m";
constexpr const char* FG_BRIGHT_WHITE = "\033[97m";

// Background Colors (Standard)
constexpr const char* BG_BLACK = "\033[40m";
constexpr const char* BG_RED = "\033[41m";
constexpr const char* BG_GREEN = "\033[42m";
constexpr const char* BG_YELLOW = "\033[43m";
constexpr const char* BG_BLUE = "\033[44m";
constexpr const char* BG_MAGENTA = "\033[45m";
constexpr const char* BG_CYAN = "\033[46m";
constexpr const char* BG_WHITE = "\033[47m";
constexpr const char* BG_DEFAULT = "\033[49m";

// Background Colors (Bright/High Intensity)
constexpr const char* BG_BRIGHT_BLACK = "\033[100m";
constexpr const char* BG_BRIGHT_RED = "\033[101m";
constexpr const char* BG_BRIGHT_GREEN = "\033[102m";
constexpr const char* BG_BRIGHT_YELLOW = "\033[103m";
constexpr const char* BG_BRIGHT_BLUE = "\033[104m";
constexpr const char* BG_BRIGHT_MAGENTA = "\033[105m";
constexpr const char* BG_BRIGHT_CYAN = "\033[106m";
constexpr const char* BG_BRIGHT_WHITE = "\033[107m";

} // namespace console
} // namespace sp
#endif // ____SP_CONSOLE_HPP____