// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "target.hpp"

#include <expected>
#include <filesystem>
#include <format>
#include <string>
#include <system_error>
#include <utility>

namespace cartomancer
{

namespace
{

namespace fs = std::filesystem;

[[nodiscard]] deck_target missing(std::string name, std::string message)
{
    return {
        .name = std::move(name),
        .deck = std::unexpected(
            arcana::error{.code = arcana::error_code::not_found, .message = std::move(message)}
        ),
    };
}

[[nodiscard]] deck_target by_name(arcana::deck_library const& library, std::string const& name)
{
    auto const summary = library.find(name);
    if (!summary.has_value())
    {
        // A name with a separator was almost certainly meant as a path.
        return missing(
            name, name.contains(fs::path::preferred_separator)
                      ? std::format("no deck directory at '{}'", name)
                      : std::format("no deck named '{}' in the library", name)
        );
    }

    return {.name = summary->path.string(), .deck = library.load(name)};
}

}  // namespace

deck_target resolve_deck(
    arcana::deck_library const& library, std::optional<std::string> const& named,
    std::optional<std::string> const& target
)
{
    if (named.has_value())
        return by_name(library, *named);

    if (target.has_value())
    {
        std::error_code failed;
        if (fs::is_directory(*target, failed))
        {
            auto const absolute = fs::weakly_canonical(*target, failed);
            auto const path = failed ? fs::path(*target) : absolute;
            return {.name = path.string(), .deck = library.load_external(*target)};
        }

        return by_name(library, *target);
    }

    auto const& reference = library.reference();
    if (!reference.has_value())
        return missing(
            "reference deck",
            "no deck given and no reference deck installed; pass --deck NAME or a directory"
        );

    return {.name = reference->path.string(), .deck = library.load_reference()};
}

}  // namespace cartomancer
