// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "json.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <optional>
#include <sstream>
#include <string>

using namespace cartomancer;  // NOLINT(google-build-using-namespace)

namespace
{

[[nodiscard]] std::string dumped(json::document const& doc)
{
    std::ostringstream out;
    json::write(out, doc);
    return out.str();
}

}  // namespace

// nlohmann owns escaping, indentation and comma placement now; what is left to
// test is the four decisions in json.hpp, each of which is observable API.

TEST_CASE("keys keep the order they were written in", "[json]")
{
    // The guard on `document` being ordered_json rather than nlohmann::json.
    // The latter is a std::map and would emit these alphabetically, silently
    // reordering every ADR-009 shape.
    json::document doc;
    doc["zeta"] = 1;
    doc["alpha"] = 2;
    doc["mu"] = 3;

    REQUIRE(dumped(doc) == R"({
  "zeta": 1,
  "alpha": 2,
  "mu": 3
}
)");
}

TEST_CASE("output is two-space indented and ends in exactly one newline", "[json]")
{
    json::document const doc{
        {"items", json::document::array({json::document{{"id", 1}}})},
        {"empty", json::document::array()},
        {"done", true},
    };

    REQUIRE(dumped(doc) == R"({
  "items": [
    {
      "id": 1
    }
  ],
  "empty": [],
  "done": true
}
)");
}

TEST_CASE("an empty optional is written as null, not omitted", "[json]")
{
    // ADR-009: present and null, never omitted, so a consumer can index
    // without a membership test.
    json::document const doc{
        {"card", std::optional<std::string>{}},
        {"key", std::optional<std::string>{"deck.id"}},
        {"path", json::from_path(std::optional<std::filesystem::path>{})},
        {"icon", json::from_path(std::optional<std::filesystem::path>{"art/icon.png"})},
    };

    REQUIRE(dumped(doc) == R"({
  "card": null,
  "key": "deck.id",
  "path": null,
  "icon": "art/icon.png"
}
)");
}

TEST_CASE("valid non-ASCII is passed through rather than escaped", "[json]")
{
    json::document const doc{{"name", "Tarot de Marseille — Grimaud"}};

    REQUIRE(dumped(doc).contains("Tarot de Marseille — Grimaud"));
}

TEST_CASE("a name the filesystem accepted but Unicode would not is replaced", "[json]")
{
    // Deck directory names are filesystem bytes. dump() throws type_error.316
    // on invalid UTF-8 by default, which would abort a report already partly
    // committed to; json::write substitutes U+FFFD instead.
    json::document const doc{{"directory_name", std::string("bad\xffname")}};

    REQUIRE_NOTHROW(dumped(doc));
    REQUIRE(dumped(doc).contains("bad\xef\xbf\xbdname"));
}

TEST_CASE("a path is rendered as its native bytes", "[json]")
{
    json::document const doc{{"path", json::from_path(std::filesystem::path{"/decks/rws"})}};

    REQUIRE(dumped(doc).contains(R"("path": "/decks/rws")"));
}
