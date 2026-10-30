// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "cli/options.hpp"
#include "cli/streams.hpp"

#include <arcana/card.hpp>
#include <arcana/deck.hpp>
#include <arcana/library.hpp>

namespace cartomancer
{

// `cartomancer show CARD-ID [TARGET]`
[[nodiscard]] cli::exit_code run_show(
    cli::options const& opts, arcana::deck_library const& library, cli::streams sink
);

// One card: its artwork, and an info panel down the right.
//
// `draw` prints exactly this, once per card it drew.
void write_card(
    arcana::deck const& subject, arcana::card const& one, cli::options const& opts,
    cli::streams sink
);

}  // namespace cartomancer
