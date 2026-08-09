// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "app.hpp"
#include "cli.hpp"

#include <string_view>

namespace cartomancer
{

// --list-codes: every rule arcana::rules() reports, one per line.
//
// No implementation-state column: arcana::rule carries no checked/pending
// field, and hardcoding the codes that happen to be implemented turns an
// honesty feature into a lie. ADR-009 Consequences records the cross-repo ask.
[[nodiscard]] int run_list_codes(options const& opts, streams sink);

// --explain CODE: one catalogue entry, or exit 4 for a code that is not in the
// catalogue.
//
// @param code the rule code asked for, already lifted out of options::explain
[[nodiscard]] int run_explain(options const& opts, std::string_view code, streams sink);

}  // namespace cartomancer
