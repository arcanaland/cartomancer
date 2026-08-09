// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "text.hpp"

namespace cartomancer::cli
{

std::string_view severity_name(arcana::severity level) noexcept
{
    switch (level)
    {
        case arcana::severity::pedantic:
            return "pedantic";
        case arcana::severity::info:
            return "info";
        case arcana::severity::warning:
            return "warning";
        case arcana::severity::error:
            return "error";
    }
    return "unknown";
}

std::string_view usage_text() noexcept
{
    return R"(cartomancer - swiss-army knife for tarot decks

Usage:
  cartomancer <command> [flags] [TARGET]

Commands:
  validate [TARGET]   validate a deck against the tarot deck spec
  list                list the decks installed on this system

Validate flags:
  --format text|json                  output format (default text)
  --level pedantic|info|warning|error  report and exit on this floor (default info)
  --explain CODE                      print one catalogue entry and exit
  --list-codes                        print the whole catalogue and exit

List flags:
  --format text|json                  output format (default text)

Global flags:
  --deck NAME         act on a discovered deck by directory name
  --color WHEN        auto|always|never|256|truecolor (default auto)
  --no-color          alias for --color=never
  --version           print the cartomancer and libarcana versions
  --help              print this text

TARGET is a deck directory, or a discovered deck's directory name. Passing
both --deck and TARGET is a usage error.

Exit codes:
  0  no diagnostics at or above --level
  1  warnings at or above --level, and no errors
  2  at least one error diagnostic
  3  the deck could not be loaded at all
  4  usage error
)";
}

}  // namespace cartomancer::cli
