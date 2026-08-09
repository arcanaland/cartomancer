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

using document = nlohmann::ordered_json;

[[nodiscard]] document from_path(std::filesystem::path const& value);

[[nodiscard]] document from_path(std::optional<std::filesystem::path> const& value);

[[nodiscard]] document from_view(std::string_view value);

// Serialize to out with two-space indent
void write(std::ostream& out, document const& doc);

}  // namespace cartomancer::json
