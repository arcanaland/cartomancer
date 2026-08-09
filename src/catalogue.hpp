// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "cli/options.hpp"
#include "cli/streams.hpp"

#include <string_view>

namespace cartomancer
{

// Report every rule that will be used for deck validation
[[nodiscard]] cli::exit_code run_list_codes(cli::options const& opts, cli::streams sink);

// Report an explanation for a given validation code
//
// @param code the rule code asked for
// @param exit 4 for a code that is not in the catalogue
[[nodiscard]] cli::exit_code run_explain(
    cli::options const& opts, std::string_view code, cli::streams sink
);

}  // namespace cartomancer
