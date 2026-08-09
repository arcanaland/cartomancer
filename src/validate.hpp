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

// `cartomancer validate [TARGET]`: judge one deck against the catalogue.
//
// Exits 0/1/2 by the worst diagnostic left after the --level floor, or 3 when
// the deck could not be loaded at all.
[[nodiscard]] cli::exit_code run_validate(
    cli::options const& opts, arcana::deck_library const& library, cli::streams sink
);

// The diagnostics at or above `floor`.
//
// --level is a threshold rather than a display filter: what it removes affects
// the exit code too, which is what makes `--level error` the CI invocation
// that tolerates warnings. ADR-009.
[[nodiscard]] std::vector<arcana::diagnostic> apply_floor(
    std::span<arcana::diagnostic const> found, arcana::severity floor
);

// The exit code for an already-filtered diagnostic list.
[[nodiscard]] cli::exit_code code_for(std::span<arcana::diagnostic const> reported) noexcept;

}  // namespace cartomancer
