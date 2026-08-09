// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "cli/color.hpp"

#include "cli/parse.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string_view>
#include <vector>

using namespace cartomancer;       // NOLINT(google-build-using-namespace)
using namespace cartomancer::cli;  // NOLINT(google-build-using-namespace)

namespace
{

constexpr bool tty = true;
constexpr bool pipe = false;

[[nodiscard]] options parse_args(std::initializer_list<std::string_view> args)
{
    std::vector<std::string_view> const argv(args);
    return parse(argv).opts;
}

}  // namespace

TEST_CASE("every --color WHEN parses", "[color]")
{
    REQUIRE(parse_color_mode("auto") == color_mode::automatic);
    REQUIRE(parse_color_mode("always") == color_mode::always);
    REQUIRE(parse_color_mode("never") == color_mode::never);
    REQUIRE(parse_color_mode("256") == color_mode::indexed_256);
    REQUIRE(parse_color_mode("truecolor") == color_mode::truecolor);
    REQUIRE_FALSE(parse_color_mode("beige").has_value());
}

TEST_CASE("the flag --no-color is an exact alias for --color=never", "[color]")
{
    auto const aliased = parse_args({"validate", "--no-color"});
    auto const spelled = parse_args({"validate", "--color=never"});

    REQUIRE(aliased.color == spelled.color);
    REQUIRE(aliased.color_explicit);
    REQUIRE(spelled.color_explicit);
}

TEST_CASE("with no flag, colour follows the terminal", "[color]")
{
    REQUIRE(resolve_color(color_mode::automatic, false, std::nullopt, tty));
    REQUIRE_FALSE(resolve_color(color_mode::automatic, false, std::nullopt, pipe));
}

TEST_CASE("NO_COLOR with any non-empty value disables colour", "[color]")
{
    REQUIRE_FALSE(resolve_color(color_mode::automatic, false, "1", tty));
    REQUIRE_FALSE(resolve_color(color_mode::automatic, false, "0", tty));
    REQUIRE_FALSE(resolve_color(color_mode::automatic, false, "anything", tty));
}

TEST_CASE("an empty NO_COLOR does not disable colour", "[color]")
{
    // https://no-color.org says any non-empty value.
    REQUIRE(resolve_color(color_mode::automatic, false, "", tty));
}

TEST_CASE("an explicit --color beats the ambient NO_COLOR", "[color]")
{
    REQUIRE(resolve_color(color_mode::always, true, "1", pipe));
    REQUIRE(resolve_color(color_mode::truecolor, true, "1", pipe));
    REQUIRE(resolve_color(color_mode::indexed_256, true, "1", pipe));
}

TEST_CASE("the flag --color=never wins over a terminal", "[color]")
{
    REQUIRE_FALSE(resolve_color(color_mode::never, true, std::nullopt, tty));
}

TEST_CASE("the flag --color=always wins over a pipe", "[color]")
{
    REQUIRE(resolve_color(color_mode::always, true, std::nullopt, pipe));
}

TEST_CASE("an explicit --color=auto asks for the terminal check and nothing else", "[color]")
{
    // ADR-009: an explicit flag beats an ambient one, and `auto` is explicit
    // when it is typed.
    REQUIRE(resolve_color(color_mode::automatic, true, "1", tty));
    REQUIRE_FALSE(resolve_color(color_mode::automatic, true, std::nullopt, pipe));
}

TEST_CASE("colour off emits no escape sequences at all", "[color]")
{
    REQUIRE(severity_color(arcana::severity::error, false).empty());
    REQUIRE(color_reset(false).empty());
}

TEST_CASE("colour on emits a distinct sequence per severity", "[color]")
{
    auto const error = severity_color(arcana::severity::error, true);
    auto const warning = severity_color(arcana::severity::warning, true);

    REQUIRE_FALSE(error.empty());
    REQUIRE_FALSE(warning.empty());
    REQUIRE(error != warning);
    REQUIRE_FALSE(color_reset(true).empty());
}
