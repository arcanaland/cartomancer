// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "validate.hpp"

#include "harness.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace cartomancer;           // NOLINT(google-build-using-namespace)
using namespace cartomancer::cli;      // NOLINT(google-build-using-namespace)
using namespace cartomancer::testing;  // NOLINT(google-build-using-namespace)

namespace
{

[[nodiscard]] arcana::diagnostic at(arcana::severity level)
{
    return {.level = level, .code = "test", .message = "m"};
}

[[nodiscard]] std::string deck_path(std::string_view directory_name)
{
    return (library_root() / directory_name).string();
}

}  // namespace

TEST_CASE("the exit code is the worst reported diagnostic", "[validate]")
{
    std::vector<arcana::diagnostic> const nothing;
    REQUIRE(code_for(nothing) == exit_code::ok);

    std::vector const info{at(arcana::severity::info)};
    REQUIRE(code_for(info) == exit_code::ok);

    std::vector const warned{at(arcana::severity::info), at(arcana::severity::warning)};
    REQUIRE(code_for(warned) == exit_code::warnings);

    std::vector const failed{at(arcana::severity::warning), at(arcana::severity::error)};
    REQUIRE(code_for(failed) == exit_code::errors);
}

TEST_CASE("the flag --level removes diagnostics below the floor", "[validate]")
{
    std::vector const found{
        at(arcana::severity::pedantic),
        at(arcana::severity::info),
        at(arcana::severity::warning),
        at(arcana::severity::error),
    };

    REQUIRE(apply_floor(found, arcana::severity::pedantic).size() == 4);
    REQUIRE(apply_floor(found, arcana::severity::info).size() == 3);
    REQUIRE(apply_floor(found, arcana::severity::warning).size() == 2);
    REQUIRE(apply_floor(found, arcana::severity::error).size() == 1);
}

TEST_CASE("a clean deck exits 0", "[validate]")
{
    auto const result = run_cli({"validate", deck_path("clean-deck")});
    REQUIRE(result.status == 0);
}

TEST_CASE("a deck with warnings and no errors exits 1", "[validate]")
{
    auto const result = run_cli({"validate", deck_path("warning-deck")});
    REQUIRE(result.status == 1);
}

TEST_CASE("a deck with an error exits 2", "[validate]")
{
    auto const result = run_cli({"validate", deck_path("error-deck")});
    REQUIRE(result.status == 2);
}

TEST_CASE("the flag --level error exits 0 on a warnings-only deck", "[validate]")
{
    auto const result = run_cli({"validate", "--level", "error", deck_path("warning-deck")});

    REQUIRE(result.status == 0);
    REQUIRE(result.out.contains("0 error(s), 0 warning(s)"));
}

TEST_CASE("the flag --level error still exits 2 on a deck with an error", "[validate]")
{
    auto const result = run_cli({"validate", "--level", "error", deck_path("error-deck")});
    REQUIRE(result.status == 2);
}

TEST_CASE("a directory that is not a deck exits 3", "[validate]")
{
    auto const result = run_cli({"validate", (fixtures() / "not-a-deck").string()});

    REQUIRE(result.status == 3);
    REQUIRE_FALSE(result.err.empty());
}

TEST_CASE("an absent path exits 3", "[validate]")
{
    auto const result = run_cli({"validate", "/nonexistent-deck-path"});
    REQUIRE(result.status == 3);
}

TEST_CASE("a malformed manifest exits 3", "[validate]")
{
    auto const result = run_cli({"validate", deck_path("malformed-deck")});
    REQUIRE(result.status == 3);
}

TEST_CASE("the flag --deck resolves a discovered deck by directory name", "[validate]")
{
    REQUIRE(run_cli({"validate", "--deck", "clean-deck"}).status == 0);
    REQUIRE(run_cli({"validate", "--deck", "error-deck"}).status == 2);
}

TEST_CASE("a TARGET that is not a path is looked up as a directory name", "[validate]")
{
    REQUIRE(run_cli({"validate", "error-deck"}).status == 2);
}

TEST_CASE("the flag --deck naming no discovered deck exits 3", "[validate]")
{
    auto const result = run_cli({"validate", "--deck", "no-such-deck"});
    REQUIRE(result.status == 3);
}

TEST_CASE("text output has no escape sequences when color is off", "[validate]")
{
    auto const result = run_cli({"validate", deck_path("error-deck")});
    REQUIRE_FALSE(result.out.contains("\033["));
}

TEST_CASE("text output has escape sequences when color is on", "[validate]")
{
    auto const result = run_cli({"validate", deck_path("error-deck")}, true);
    REQUIRE(result.out.contains("\033["));
}
