// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "app.hpp"

#include "catalogue.hpp"
#include "cli/color.hpp"
#include "cli/parse.hpp"
#include "cli/text.hpp"
#include "list.hpp"
#include "validate.hpp"

#include <arcana/version.hpp>
#include <cartomancer/version.hpp>

#include <unistd.h>
#include <cstdlib>
#include <format>
#include <optional>
#include <ostream>
#include <string_view>
#include <utility>

namespace cartomancer
{

namespace
{

void write_version(cli::streams sink)
{
    sink.out << std::format("cartomancer {}\n", version);
    sink.out << std::format("libarcana {}\n", arcana::library_version());
}

[[nodiscard]] std::optional<std::string_view> environment(char const* name)
{
    char const* value = std::getenv(name);
    if (value == nullptr)
        return std::nullopt;

    return std::string_view(value);
}

// Everything impure about presentation happens here, once: the environment,
// isatty and the window size are read and handed to the pure resolvers.
[[nodiscard]] cli::theme resolve_theme(cli::options const& requested)
{
    bool const tty = isatty(STDOUT_FILENO) == 1;

    auto const depth = cli::resolve_color_depth(
        requested.color, requested.color_explicit,
        // https://no-color.org <3
        environment("NO_COLOR"), tty, environment("COLORTERM"), environment("TERM")
    );

    return {
        .depth = depth,
        .style = cli::for_depth(depth),
        .mark = cli::glyphs_for_locale(
            environment("LC_ALL"), environment("LC_CTYPE"), environment("LANG")
        ),
        .width = cli::resolve_width(tty, environment("COLUMNS"), cli::window_columns()),
    };
}

[[nodiscard]] cli::exit_code report_usage_error(std::string_view message, cli::streams sink)
{
    sink.err << std::format(
        "{}{}{} cartomancer: {}\n", sink.style.style.error, sink.style.mark.bad,
        sink.style.style.reset, message
    );
    sink.err << "Usage: cartomancer --help\n";
    return cli::exit_code::usage;
}

[[nodiscard]] cli::exit_code dispatch(
    cli::options const& opts, arcana::library_options lib_options, cli::streams sink
)
{
    if (opts.help)
    {
        sink.out << cli::usage_text(sink.style.depth);
        return cli::exit_code::ok;
    }

    if (opts.version)
    {
        write_version(sink);
        return cli::exit_code::ok;
    }

    if (opts.list_codes)
        return run_list_codes(opts, sink);

    if (opts.explain.has_value())
        return run_explain(opts, *opts.explain, sink);

    if (opts.which == cli::command::none)
    {
        sink.out << cli::usage_text(sink.style.depth);
        return cli::exit_code::ok;
    }

    arcana::deck_library const library(std::move(lib_options));

    switch (opts.which)
    {
        case cli::command::validate:
            return run_validate(opts, library, sink);
        case cli::command::list:
            return run_list(opts, library, sink);
        case cli::command::none:
            break;
    }

    std::unreachable();
}

[[nodiscard]] int run_parsed(
    cli::parse_result const& parsed, arcana::library_options lib_options, cli::streams sink
)
{
    if (!parsed.has_value())
        return cli::to_int(report_usage_error(parsed.error(), sink));

    return cli::to_int(dispatch(*parsed, std::move(lib_options), sink));
}

}  // namespace

int run_with_library(
    std::span<std::string_view const> args, arcana::library_options lib_options, cli::streams sink
)
{
    return run_parsed(cli::parse(args), std::move(lib_options), sink);
}

int run(std::span<std::string_view const> args, cli::streams sink)
{
    auto const parsed = cli::parse(args);

    sink.style = resolve_theme(parsed.value_or(cli::options{}));

    return run_parsed(parsed, {}, sink);
}

}  // namespace cartomancer
