// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Which deck a command acts on.

#pragma once

#include <arcana/deck.hpp>
#include <arcana/error.hpp>
#include <arcana/library.hpp>

#include <expected>
#include <memory>
#include <optional>
#include <string>

namespace cartomancer
{

// A deck the CLI resolved to, and what came of loading it.
struct deck_target
{
    // What we resolved, for the message when it will not load.
    std::string name;

    std::expected<std::shared_ptr<arcana::deck const>, arcana::error> deck;
};

// Resolve a deck: --deck NAME, then the positional TARGET, then the library's
// reference deck.
//
// This is ADR-009's order, and the "." default belongs to `validate` alone —
// `show` and `draw` in an arbitrary directory should reach for the reference
// deck, not for whatever the shell happens to be sitting in.
[[nodiscard]] deck_target resolve_deck(
    arcana::deck_library const& library, std::optional<std::string> const& named,
    std::optional<std::string> const& target
);

}  // namespace cartomancer
