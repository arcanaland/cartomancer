// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "cli/options.hpp"
#include "cli/streams.hpp"

#include <arcana/library.hpp>
#include <arcana/validation.hpp>

#include <span>
#include <vector>

namespace cartomancer
{

// `cartomancer validate [TARGET]`
//
// @return 0/1/2 by the worst diagnostic left after the level floor. Or 3 when
//         the deck could not be loaded at all.
[[nodiscard]] cli::exit_code run_validate(
    cli::options const& opts, arcana::deck_library const& library, cli::streams sink
);

// @return the diagnostics at or above a given floor.
[[nodiscard]] std::vector<arcana::diagnostic> apply_floor(
    std::span<arcana::diagnostic const> found, arcana::severity floor
);

// @return the exit code for an already-filtered diagnostic list.
[[nodiscard]] cli::exit_code code_for(std::span<arcana::diagnostic const> reported) noexcept;

}  // namespace cartomancer
