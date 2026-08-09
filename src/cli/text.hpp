// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The fixed strings the CLI prints.

#pragma once

#include <arcana/validation.hpp>

#include <string_view>

namespace cartomancer::cli
{

// The spelling of a severity in both the text and the JSON output.
[[nodiscard]] std::string_view severity_name(arcana::severity level) noexcept;

// The --help text, also printed for a bare `cartomancer`.
[[nodiscard]] std::string_view usage_text() noexcept;

}  // namespace cartomancer::cli
