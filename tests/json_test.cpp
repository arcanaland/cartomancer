// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "json.hpp"

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <sstream>
#include <string>
#include <string_view>

using namespace cartomancer;  // NOLINT(google-build-using-namespace)

TEST_CASE("strings are escaped", "[json]")
{
    REQUIRE(json::escape("plain") == "plain");
    REQUIRE(json::escape(R"(a "quoted" word)") == R"(a \"quoted\" word)");
    REQUIRE(json::escape("back\\slash") == R"(back\\slash)");
    REQUIRE(json::escape("two\nlines") == R"(two\nlines)");
    REQUIRE(json::escape("a\tb") == R"(a\tb)");
    REQUIRE(json::escape(std::string_view("\x01", 1)) == R"(\u0001)");
}

TEST_CASE("an empty object and an empty array have no interior", "[json]")
{
    std::ostringstream out;
    json::writer writer(out);
    writer.begin_object();
    writer.key("empty");
    writer.begin_array();
    writer.end_array();
    writer.end_object();

    REQUIRE(out.str() == "{\n  \"empty\": []\n}");
}

TEST_CASE("nested containers are comma-separated and indented", "[json]")
{
    std::ostringstream out;
    json::writer writer(out);
    writer.begin_object();
    writer.key("items");
    writer.begin_array();
    writer.begin_object();
    writer.key("id");
    writer.number(1);
    writer.end_object();
    writer.begin_object();
    writer.key("id");
    writer.number(2);
    writer.end_object();
    writer.end_array();
    writer.key("done");
    writer.boolean(true);
    writer.end_object();
    writer.finish();

    REQUIRE(out.str() == R"({
  "items": [
    {
      "id": 1
    },
    {
      "id": 2
    }
  ],
  "done": true
}
)");
}

TEST_CASE("an empty optional is written as null, not omitted", "[json]")
{
    // ADR-009: present and null, never omitted, so a consumer can index
    // without a membership test.
    std::ostringstream out;
    json::writer writer(out);
    writer.begin_object();
    writer.key("card");
    writer.string_or_null(std::nullopt);
    writer.key("key");
    writer.string_or_null(std::optional<std::string>{"deck.id"});
    writer.end_object();

    REQUIRE(out.str() == "{\n  \"card\": null,\n  \"key\": \"deck.id\"\n}");
}
