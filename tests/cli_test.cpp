// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "cli.hpp"

#include "harness.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <span>
#include <string_view>

using namespace cartomancer;          // NOLINT(google-build-using-namespace)
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
    REQUIRE_FALSE(parsed.error.has_value());
    REQUIRE(parsed.opts.which == command::none);
}

TEST_CASE("subcommands are recognised", "[cli]")
{
    REQUIRE(parse_args({"validate"}).opts.which == command::validate);
    REQUIRE(parse_args({"list"}).opts.which == command::list);
}

TEST_CASE("show and draw are not stubbed, so they are unknown subcommands", "[cli]")
{
    // ADR-009 specifies them; TASK-010 deliberately does not ship them, and an
    // unimplemented subcommand that exists is worse than one that does not.
    REQUIRE(parse_args({"show"}).error.has_value());
    REQUIRE(parse_args({"draw"}).error.has_value());
    REQUIRE(run_cli({"show", "major_arcana.00"}).status == 4);
    REQUIRE(run_cli({"draw"}).status == 4);
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
    // Otherwise `cartomancer --nonsense list` would report a stranger error
    // than the one the user made.
    auto const parsed = parse_args({"--nonsense", "list"});
    REQUIRE(parsed.error.has_value());
    REQUIRE(parsed.error.value_or("") == "unknown flag: --nonsense");
}

TEST_CASE("flag values are accepted both attached and separated", "[cli]")
{
    REQUIRE(parse_args({"validate", "--level=error"}).opts.level == arcana::severity::error);
    REQUIRE(parse_args({"validate", "--level", "error"}).opts.level == arcana::severity::error);
    REQUIRE(parse_args({"list", "--format=json"}).opts.format == output_format::json);
    REQUIRE(parse_args({"list", "--format", "json"}).opts.format == output_format::json);
}

TEST_CASE("every severity name parses", "[cli]")
{
    REQUIRE(parse_args({"validate", "--level", "pedantic"}).opts.level
            == arcana::severity::pedantic);
    REQUIRE(parse_args({"validate", "--level", "info"}).opts.level == arcana::severity::info);
    REQUIRE(parse_args({"validate", "--level", "warning"}).opts.level == arcana::severity::warning);
    REQUIRE(parse_args({"validate", "--level", "error"}).opts.level == arcana::severity::error);
}

TEST_CASE("the default level is info and the default format is text", "[cli]")
{
    auto const parsed = parse_args({"validate"});
    REQUIRE(parsed.opts.level == arcana::severity::info);
    REQUIRE(parsed.opts.format == output_format::text);
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
    // A user who supplies two deck selectors has a wrong belief about one of
    // them, so this is exit 4 rather than a silent precedence win. ADR-009.
    auto const result = run_cli({"validate", "--deck", "clean-deck", "error-deck"});
    REQUIRE(result.status == 4);
    REQUIRE(result.err.contains("not both"));
}

TEST_CASE("the flag --deck alone and TARGET alone are both fine", "[cli]")
{
    REQUIRE_FALSE(parse_args({"validate", "--deck", "clean-deck"}).error.has_value());
    REQUIRE_FALSE(parse_args({"validate", "clean-deck"}).error.has_value());
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

TEST_CASE("a bare invocation prints help and exits 0", "[cli]")
{
    auto const result = run_cli({});
    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("Usage:"));
}

TEST_CASE("the flag --version prints cartomancer and the libarcana it linked", "[cli]")
{
    // A validator's answer is a function of the library's rule set, so the
    // library version is part of the answer. ADR-009.
    auto const result = run_cli({"--version"});
    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("cartomancer "));
    REQUIRE(result.out.contains("libarcana "));

    auto const lines = std::ranges::count(result.out, '\n');
    REQUIRE(lines == 2);
}

TEST_CASE("global flags are accepted before and after the subcommand", "[cli]")
{
    REQUIRE(parse_args({"--format", "json", "list"}).opts.format == output_format::json);
    REQUIRE(parse_args({"list", "--format", "json"}).opts.format == output_format::json);
}

TEST_CASE("exit codes are the ADR-009 integers", "[cli]")
{
    REQUIRE(to_int(exit_code::ok) == 0);
    REQUIRE(to_int(exit_code::warnings) == 1);
    REQUIRE(to_int(exit_code::errors) == 2);
    REQUIRE(to_int(exit_code::unloadable) == 3);
    REQUIRE(to_int(exit_code::usage) == 4);
}
