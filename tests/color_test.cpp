// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "cli/color.hpp"

#include "cli/parse.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <optional>
#include <string_view>
#include <vector>

using namespace cartomancer;       // NOLINT(google-build-using-namespace)
using namespace cartomancer::cli;  // NOLINT(google-build-using-namespace)

namespace
{

constexpr bool tty = true;
constexpr bool pipe = false;

constexpr std::optional<std::string_view> unset;

[[nodiscard]] options parse_args(std::initializer_list<std::string_view> args)
{
    std::vector<std::string_view> const argv(args);
    return parse(argv).value();
}

// resolve_color_depth with nothing advertised, which is the ansi16 floor.
[[nodiscard]] color_depth depth_of(
    color_mode mode, bool explicitly_set, std::optional<std::string_view> no_color, bool terminal
)
{
    return resolve_color_depth(mode, explicitly_set, no_color, terminal, unset, unset);
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

TEST_CASE("with no flag, color follows the terminal", "[color]")
{
    REQUIRE(depth_of(color_mode::automatic, false, unset, tty) == color_depth::ansi16);
    REQUIRE(depth_of(color_mode::automatic, false, unset, pipe) == color_depth::none);
}

TEST_CASE("NO_COLOR with any non-empty value disables color", "[color]")
{
    REQUIRE(depth_of(color_mode::automatic, false, "1", tty) == color_depth::none);
    REQUIRE(depth_of(color_mode::automatic, false, "0", tty) == color_depth::none);
    REQUIRE(depth_of(color_mode::automatic, false, "anything", tty) == color_depth::none);
}

TEST_CASE("an empty NO_COLOR does not disable color", "[color]")
{
    // https://no-color.org says any non-empty value.
    REQUIRE(depth_of(color_mode::automatic, false, "", tty) == color_depth::ansi16);
}

TEST_CASE("an explicit --color beats the ambient NO_COLOR", "[color]")
{
    REQUIRE(depth_of(color_mode::always, true, "1", pipe) == color_depth::ansi16);
    REQUIRE(depth_of(color_mode::truecolor, true, "1", pipe) == color_depth::truecolor);
    REQUIRE(depth_of(color_mode::indexed_256, true, "1", pipe) == color_depth::indexed_256);
}

TEST_CASE("the flag --color=never wins over a terminal", "[color]")
{
    REQUIRE(depth_of(color_mode::never, true, unset, tty) == color_depth::none);
}

TEST_CASE("the flag --color=always wins over a pipe", "[color]")
{
    REQUIRE(depth_of(color_mode::always, true, unset, pipe) == color_depth::ansi16);
}

TEST_CASE("an explicit --color=auto asks for the terminal check", "[color]")
{
    REQUIRE(depth_of(color_mode::automatic, true, "1", tty) == color_depth::ansi16);
    REQUIRE(depth_of(color_mode::automatic, true, unset, pipe) == color_depth::none);
}

TEST_CASE("the values 256 and truecolor select a depth of their own", "[color]")
{
    REQUIRE(depth_of(color_mode::indexed_256, true, unset, tty) == color_depth::indexed_256);
    REQUIRE(depth_of(color_mode::truecolor, true, unset, tty) == color_depth::truecolor);
    REQUIRE(depth_of(color_mode::always, true, unset, tty) == color_depth::ansi16);

    REQUIRE(for_depth(color_depth::indexed_256).accent != for_depth(color_depth::ansi16).accent);
    REQUIRE(for_depth(color_depth::truecolor).accent != for_depth(color_depth::indexed_256).accent);
}

TEST_CASE("what the terminal advertises is consulted only under auto and always", "[color]")
{
    constexpr std::string_view rich = "truecolor";
    constexpr std::string_view indexed = "xterm-256color";

    REQUIRE(
        resolve_color_depth(color_mode::automatic, false, unset, tty, rich, unset) ==
        color_depth::truecolor
    );
    REQUIRE(
        resolve_color_depth(color_mode::automatic, false, unset, tty, "24bit", unset) ==
        color_depth::truecolor
    );
    REQUIRE(
        resolve_color_depth(color_mode::always, true, unset, pipe, unset, indexed) ==
        color_depth::indexed_256
    );
    REQUIRE(
        resolve_color_depth(color_mode::automatic, false, unset, tty, unset, "xterm") ==
        color_depth::ansi16
    );

    // An explicit depth asserts
    REQUIRE(
        resolve_color_depth(color_mode::indexed_256, true, unset, tty, rich, unset) ==
        color_depth::indexed_256
    );
    REQUIRE(
        resolve_color_depth(color_mode::never, true, unset, tty, rich, indexed) == color_depth::none
    );
}

TEST_CASE("the palette at depth none is empty in every role", "[color]")
{
    auto const blank = for_depth(color_depth::none);

    REQUIRE(blank.strong.empty());
    REQUIRE(blank.muted.empty());
    REQUIRE(blank.accent.empty());
    REQUIRE(blank.success.empty());
    REQUIRE(blank.error.empty());
    REQUIRE(blank.warning.empty());
    REQUIRE(blank.info.empty());
    REQUIRE(blank.pedantic.empty());
    REQUIRE(blank.reset.empty());
}

TEST_CASE("color off emits no escape sequences at all", "[color]")
{
    theme const plain;

    REQUIRE(severity_style(plain, arcana::severity::error).empty());
    REQUIRE(plain.style.reset.empty());
}

TEST_CASE("color on emits a distinct sequence per severity", "[color]")
{
    theme const lit{.depth = color_depth::ansi16, .style = for_depth(color_depth::ansi16)};

    auto const error = severity_style(lit, arcana::severity::error);
    auto const warning = severity_style(lit, arcana::severity::warning);

    REQUIRE_FALSE(error.empty());
    REQUIRE_FALSE(warning.empty());
    REQUIRE(error != warning);
    REQUIRE_FALSE(lit.style.reset.empty());
}

TEST_CASE("a UTF-8 codeset selects the unicode glyphs", "[color]")
{
    struct locale_case
    {
        std::optional<std::string_view> lc_all;
        std::optional<std::string_view> lc_ctype;
        std::optional<std::string_view> lang;
        bool unicode;
    };

    constexpr std::array cases{
        locale_case{"en_US.UTF-8", unset, unset, true},
        locale_case{"C.utf8", unset, unset, true},
        locale_case{"en_GB.utf-8", unset, unset, true},
        locale_case{"de_DE.UTF-8@euro", unset, unset, true},
        locale_case{"C", unset, unset, false},
        locale_case{"POSIX", unset, unset, false},
        locale_case{"en_US.ISO-8859-1", unset, unset, false},
        locale_case{unset, unset, unset, false},

        // LC_ALL beats LC_CTYPE beats LANG, and an empty value counts as unset.
        locale_case{"C", "en_US.UTF-8", "en_US.UTF-8", false},
        locale_case{unset, "C", "en_US.UTF-8", false},
        locale_case{"", "", "en_US.UTF-8", true},
        locale_case{unset, unset, "C.UTF-8", true},
    };

    for (auto const& one : cases)
    {
        auto const chosen = glyphs_for_locale(one.lc_all, one.lc_ctype, one.lang);
        REQUIRE(chosen.ok == (one.unicode ? unicode_marks.ok : ascii_marks.ok));
    }
}

TEST_CASE("glyphs and color are independent", "[color]")
{
    // NO_COLOR still gets the check mark and LC_CTYPE=C still gets color.
    REQUIRE(glyphs_for_locale("en_US.UTF-8", unset, unset).ok == unicode_marks.ok);
    REQUIRE(depth_of(color_mode::automatic, false, "1", tty) == color_depth::none);
    REQUIRE(glyphs_for_locale("C", unset, unset).ok == ascii_marks.ok);
    REQUIRE(depth_of(color_mode::automatic, false, unset, tty) == color_depth::ansi16);
}

TEST_CASE("COLUMNS overrides the window, and a pipe is unbounded", "[color]")
{
    REQUIRE(resolve_width(tty, "40", 120) == 40);
    REQUIRE(resolve_width(pipe, "40", 120) == 40);

    // Non-numeric, zero and trailing junk are all ignored.
    REQUIRE(resolve_width(tty, "wide", 120) == 120);
    REQUIRE(resolve_width(tty, "0", 120) == 120);
    REQUIRE(resolve_width(tty, "40x", 120) == 120);
    REQUIRE(resolve_width(tty, "", 120) == 120);

    // 80 when stdout is a terminal but nothing answered.
    REQUIRE(resolve_width(tty, unset, std::nullopt) == 80);

    // 0 is unbounded, and is what a non-tty gets.
    REQUIRE(resolve_width(pipe, unset, std::nullopt) == 0);
    REQUIRE(resolve_width(pipe, unset, 120) == 0);
}

TEST_CASE("fit shortens to a column budget", "[color]")
{
    REQUIRE(fit("short", 10, "...") == "short");
    REQUIRE(fit("short", 5, "...") == "short");
    REQUIRE(fit("truncate-me", 8, "...") == "trunc...");
    REQUIRE(fit("truncate-me", 8, "…") == "truncat…");
    REQUIRE(fit("truncate-me", 0, "...").empty());

    // Cuts land on codepoint boundaries
    REQUIRE(display_width(fit("ααααα", 3, "…")) == 3);
    REQUIRE(fit("ααααα", 3, "…") == "αα…");

    // When even the marker will not fit, a bare cut is all that is left.
    // Not sure why we're testing this, but meh
    REQUIRE(fit("truncate-me", 2, "...") == "tr");
}

TEST_CASE("display width counts codepoints, not bytes", "[color]")
{
    REQUIRE(display_width("abc") == 3);
    REQUIRE(display_width("✓") == 1);
    REQUIRE(display_width("→ deck.toml") == 11);
    REQUIRE(display_width("") == 0);
}
