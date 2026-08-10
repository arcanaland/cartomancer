// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "cli/parse.hpp"

#include "harness.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <span>
#include <string_view>

using namespace cartomancer;           // NOLINT(google-build-using-namespace)
using namespace cartomancer::cli;      // NOLINT(google-build-using-namespace)
using namespace cartomancer::testing;  // NOLINT(google-build-using-namespace)

namespace
{

[[nodiscard]] parse_result parse_args(std::initializer_list<std::string_view> args)
{
    std::vector<std::string_view> const argv(args);
    return parse(argv);
}

}  // namespace

TEST_CASE("a bare invocation asks for no command", "[cli]")
{
    auto const parsed = parse_args({});
    REQUIRE(parsed.has_value());
    REQUIRE(parsed->which == command::none);
}

TEST_CASE("subcommands are recognised", "[cli]")
{
    REQUIRE(parse_args({"validate"})->which == command::validate);
    REQUIRE(parse_args({"list"})->which == command::list);
}

TEST_CASE("an unknown subcommand is a usage error", "[cli]")
{
    auto const result = run_cli({"summon"});
    REQUIRE(result.status == 4);
    REQUIRE(result.err.contains("unknown subcommand: summon"));
}

TEST_CASE("an unknown flag is a usage error", "[cli]")
{
    auto const result = run_cli({"--nonsense"});
    REQUIRE(result.status == 4);
    REQUIRE(result.err.contains("unknown flag: --nonsense"));
}

TEST_CASE("an unknown flag does not swallow the argument after it", "[cli]")
{
    auto const parsed = parse_args({"--nonsense", "list"});
    REQUIRE_FALSE(parsed.has_value());
    REQUIRE(parsed.error() == "unknown flag: --nonsense");
}

TEST_CASE("flag values are accepted both attached and separated", "[cli]")
{
    REQUIRE(parse_args({"validate", "--level=error"})->level == arcana::severity::error);
    REQUIRE(parse_args({"validate", "--level", "error"})->level == arcana::severity::error);
    REQUIRE(parse_args({"list", "--format=json"})->format == output_format::json);
    REQUIRE(parse_args({"list", "--format", "json"})->format == output_format::json);
}

TEST_CASE("every severity name parses", "[cli]")
{
    REQUIRE(parse_args({"validate", "--level", "pedantic"})->level == arcana::severity::pedantic);
    REQUIRE(parse_args({"validate", "--level", "info"})->level == arcana::severity::info);
    REQUIRE(parse_args({"validate", "--level", "warning"})->level == arcana::severity::warning);
    REQUIRE(parse_args({"validate", "--level", "error"})->level == arcana::severity::error);
}

TEST_CASE("the default level is info and the default format is text", "[cli]")
{
    auto const parsed = parse_args({"validate"});
    REQUIRE(parsed->level == arcana::severity::info);
    REQUIRE(parsed->format == output_format::text);
}

TEST_CASE("an unparseable flag value is a usage error", "[cli]")
{
    REQUIRE(run_cli({"validate", "--level", "shouting"}).status == 4);
    REQUIRE(run_cli({"list", "--format", "yaml"}).status == 4);
    REQUIRE(run_cli({"list", "--color", "beige"}).status == 4);
}

TEST_CASE("a flag with nothing after it is a usage error", "[cli]")
{
    auto const result = run_cli({"validate", "--level"});
    REQUIRE(result.status == 4);
    REQUIRE(result.err.contains("--level needs a value"));
}

TEST_CASE("the flag --deck and TARGET together are a usage error", "[cli]")
{
    auto const result = run_cli({"validate", "--deck", "clean-deck", "error-deck"});
    REQUIRE(result.status == 4);
    REQUIRE(result.err.contains("not both"));
}

TEST_CASE("the flag --deck alone and TARGET alone are both fine", "[cli]")
{
    REQUIRE(parse_args({"validate", "--deck", "clean-deck"}).has_value());
    REQUIRE(parse_args({"validate", "clean-deck"}).has_value());
}

TEST_CASE("a second positional after TARGET is a usage error", "[cli]")
{
    REQUIRE(run_cli({"validate", "clean-deck", "error-deck"}).status == 4);
}

TEST_CASE("the flag --help exits 0 and prints the surface", "[cli]")
{
    auto const result = run_cli({"--help"});
    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("validate"));
    REQUIRE(result.out.contains("list"));
}

TEST_CASE("the flag --help is styled by depth and by depth alone", "[cli]")
{
    auto const plain = run_cli({"--help"});
    auto const lit = run_cli({"--help"}, styled(color_depth::truecolor));

    REQUIRE_FALSE(plain.out.contains("\033["));
    REQUIRE(lit.out.contains("\033["));
    REQUIRE(lit.out != plain.out);

    // Glyphs and width do not reach --help, so neither changes a byte of it.
    cli::theme const dressed{
        .depth = color_depth::none,
        .style = for_depth(color_depth::none),
        .mark = unicode_marks,
        .width = 20,
    };
    REQUIRE(run_cli({"--help"}, dressed).out == plain.out);
}

TEST_CASE("a bare invocation prints help and exits 0", "[cli]")
{
    auto const result = run_cli({});
    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("Usage:"));
}

TEST_CASE("the flag --version prints cartomancer and libarcana versions", "[cli]")
{
    auto const result = run_cli({"--version"});
    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("cartomancer "));
    REQUIRE(result.out.contains("libarcana "));

    auto const lines = std::ranges::count(result.out, '\n');
    REQUIRE(lines == 2);
}

TEST_CASE("global flags are accepted before and after the subcommand", "[cli]")
{
    REQUIRE(parse_args({"--format", "json", "list"})->format == output_format::json);
    REQUIRE(parse_args({"list", "--format", "json"})->format == output_format::json);
}

TEST_CASE("exit codes are integers", "[cli]")
{
    REQUIRE(to_int(exit_code::ok) == 0);
    REQUIRE(to_int(exit_code::warnings) == 1);
    REQUIRE(to_int(exit_code::errors) == 2);
    REQUIRE(to_int(exit_code::unloadable) == 3);
    REQUIRE(to_int(exit_code::usage) == 4);
}
