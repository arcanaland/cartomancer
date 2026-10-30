// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "draw.hpp"

#include "cli/text.hpp"
#include "show.hpp"
#include "target.hpp"

#include <algorithm>
#include <cstdint>
#include <expected>
#include <format>
#include <numeric>
#include <ostream>
#include <random>
#include <vector>

namespace cartomancer
{

namespace
{

// A shuffle of the deck's card positions.
//
// Not deck::random_card(), which answers a different question: it draws one
// card with replacement, so `draw 3` built on it could deal the same card
// three times. A draw is cards taken off a shuffled deck.
[[nodiscard]] std::vector<std::size_t> shuffled(std::size_t count, std::uint64_t seed)
{
    std::vector<std::size_t> order(count);
    std::iota(order.begin(), order.end(), 0);

    std::mt19937_64 engine(seed);
    std::ranges::shuffle(order, engine);

    return order;
}

[[nodiscard]] std::uint64_t seed_from(cli::options const& opts)
{
    if (opts.seed.has_value())
        return *opts.seed;

    std::random_device entropy;
    return (static_cast<std::uint64_t>(entropy()) << 32U) | entropy();
}

}  // namespace

cli::exit_code run_draw(
    cli::options const& opts, arcana::deck_library const& library, cli::streams sink
)
{
    // `draw` has no positional TARGET — its positional is N — so the deck is
    // --deck or the reference deck.
    auto const what = resolve_deck(library, opts.deck, std::nullopt);
    if (!what.deck.has_value())
    {
        cli::write_problem(
            sink, std::format("cannot read deck {}: {}", what.name, what.deck.error().message)
        );
        return cli::exit_code::unloadable;
    }

    auto const& subject = **what.deck;

    if (opts.count > subject.cards.size())
    {
        cli::write_problem(
            sink, std::format(
                      "cannot draw {} cards: {} has {}", opts.count, subject.metadata.name,
                      subject.cards.size()
                  )
        );
        return cli::exit_code::usage;
    }

    auto const order = shuffled(subject.cards.size(), seed_from(opts));

    for (std::size_t drawn = 0; drawn < opts.count; ++drawn)
    {
        if (drawn > 0)
            sink.out << '\n';

        write_card(subject, subject.cards[order[drawn]], opts, sink);
    }

    return cli::exit_code::ok;
}

}  // namespace cartomancer
