// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "app.hpp"

#include "catalogue.hpp"
#include "cli.hpp"
#include "list.hpp"
#include "validate.hpp"

#include <arcana/version.hpp>
#include <cartomancer/version.hpp>

#include <cstdlib>
#include <format>
#include <optional>
#include <ostream>
#include <string_view>
#include <unistd.h>
#include <utility>

namespace cartomancer
{

namespace
{

// A validator's answer is a function of the library's rule set, so a bug
// report that omits the library version is unactionable. ADR-009.
void write_version(streams sink)
{
    sink.out << std::format("cartomancer {}\n", version);
    sink.out << std::format("libarcana {}\n", arcana::library_version());
}

[[nodiscard]] std::optional<std::string_view> ambient_no_color()
{
    char const* value = std::getenv("NO_COLOR");
    if (value == nullptr)
        return std::nullopt;
    return std::string_view(value);
}

[[nodiscard]] int dispatch(
    parse_result const& parsed, arcana::library_options lib_options, streams sink
)
{
    if (parsed.error.has_value())
    {
        sink.err << std::format("cartomancer: {}\n", *parsed.error);
        sink.err << "try 'cartomancer --help'\n";
        return to_int(exit_code::usage);
    }

    auto const& opts = parsed.opts;

    if (opts.help)
    {
        sink.out << usage_text();
        return to_int(exit_code::ok);
    }

    if (opts.version)
    {
        write_version(sink);
        return to_int(exit_code::ok);
    }

    // --list-codes and --explain report the catalogue, not a deck, so they
    // short-circuit before any library is scanned. ADR-009 lists them under
    // `validate`; they are accepted bare as well, which is how TASK-010's
    // acceptance check invokes them.
    if (opts.list_codes)
        return run_list_codes(opts, sink);

    if (opts.explain.has_value())
        return run_explain(opts, *opts.explain, sink);

    if (opts.which == command::none)
    {
        sink.out << usage_text();
        return to_int(exit_code::ok);
    }

    arcana::deck_library const library(std::move(lib_options));

    switch (opts.which)
    {
        case command::validate:
            return run_validate(opts, library, sink);
        case command::list:
            return run_list(opts, library, sink);
        case command::none:
            break;
    }

    std::unreachable();
}

}  // namespace

int run_with_library(
    std::span<std::string_view const> args, arcana::library_options lib_options, streams sink
)
{
    return dispatch(parse(args), std::move(lib_options), sink);
}

int run(std::span<std::string_view const> args, streams sink)
{
    auto const parsed = parse(args);

    sink.colored = resolve_color(
        parsed.opts.color,
        parsed.opts.color_explicit,
        ambient_no_color(),
        isatty(STDOUT_FILENO) == 1
    );

    return dispatch(parsed, {}, sink);
}

}  // namespace cartomancer
