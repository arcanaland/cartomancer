// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <optional>
#include <ostream>
#include <string_view>

namespace cartomancer::json
{

// `ordered_json`, never `nlohmann::json`: the default object type is
// `std::map`, which serializes keys in alphabetical order. ADR-009 makes every
// field name in --format json part of the CLI's API, and the order they are
// written in is how those shapes are documented -- so insertion order it is.
using document = nlohmann::ordered_json;

// A path as the bytes we print everywhere else.
//
// nlohmann converts std::filesystem::path itself, but by way of UTF-8, which
// on this platform is a different answer from path::string() for a name the
// filesystem accepted and Unicode would not. Paths go through here instead.
[[nodiscard]] document from_path(std::filesystem::path const& value);

// ADR-009: empty optionals are present and null, never omitted, so a consumer
// can index without a membership test.
[[nodiscard]] document from_path(std::optional<std::filesystem::path> const& value);

// arcana hands us string_views into static storage; nlohmann wants an owner.
[[nodiscard]] document from_view(std::string_view value);

// Serialize to `out`: two-space indent, non-ASCII passed through, one trailing
// newline.
//
// Deck names and paths are filesystem bytes and are not guaranteed to be valid
// UTF-8. dump() throws type_error.316 on those by default, which would take
// the process down partway through a report it had already committed to; we
// substitute U+FFFD instead.
void write(std::ostream& out, document const& doc);

}  // namespace cartomancer::json
