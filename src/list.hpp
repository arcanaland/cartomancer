// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "cli/options.hpp"
#include "cli/streams.hpp"

#include <arcana/library.hpp>

namespace cartomancer
{

// `cartomancer list`: the discovered decks, then the malformed ones.
//
// Always exits 0 when the roots were scanned, malformed decks included --
// reporting them is the command's job, so doing it is a success. ADR-009.
[[nodiscard]] int run_list(
    cli::options const& opts, arcana::deck_library const& library, cli::streams sink
);

}  // namespace cartomancer
