// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "cli/options.hpp"
#include "cli/streams.hpp"

#include <arcana/library.hpp>

namespace cartomancer
{

// `cartomancer list`
[[nodiscard]] cli::exit_code run_list(
    cli::options const& opts, arcana::deck_library const& library, cli::streams sink
);

}  // namespace cartomancer
