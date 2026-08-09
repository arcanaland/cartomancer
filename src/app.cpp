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

[[nodiscard]] std::optional<std::string_view> ambient_no_color()
{
    char const* value = std::getenv("NO_COLOR");
    if (value == nullptr)
        return std::nullopt;

    return std::string_view(value);
}

[[nodiscard]] cli::exit_code report_usage_error(std::string_view message, cli::streams sink)
{
    sink.err << std::format("cartomancer: {}\n", message);
    sink.err << "Usage: cartomancer --help\n";
    return cli::exit_code::usage;
}

[[nodiscard]] cli::exit_code dispatch(
    cli::options const& opts, arcana::library_options lib_options, cli::streams sink
)
{
    if (opts.help)
    {
        sink.out << cli::usage_text();
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
        sink.out << cli::usage_text();
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

// The one place an `exit_code` becomes the process's integer.
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

    // A failed parse yields no options to consult, so colour falls back to the
    // ambient answer. Nothing on the usage-error path is coloured, so the only
    // effect is that colour no longer depends on how far the parse got before
    // it gave up.
    auto const requested = parsed.value_or(cli::options{});

    sink.use_color = cli::resolve_color(
        requested.color, requested.color_explicit, ambient_no_color(), isatty(STDOUT_FILENO) == 1
    );

    return run_parsed(parsed, {}, sink);
}

}  // namespace cartomancer
