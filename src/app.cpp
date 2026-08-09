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

[[nodiscard]] int dispatch(
    cli::parse_result const& parsed, arcana::library_options lib_options, cli::streams sink
)
{
    if (parsed.error.has_value())
    {
        sink.err << std::format("cartomancer: {}\n", *parsed.error);
        sink.err << "Usage: cartomancer --help\n";
        return cli::to_int(cli::exit_code::usage);
    }

    auto const& opts = parsed.opts;

    if (opts.help)
    {
        sink.out << cli::usage_text();
        return cli::to_int(cli::exit_code::ok);
    }

    if (opts.version)
    {
        write_version(sink);
        return cli::to_int(cli::exit_code::ok);
    }

    if (opts.list_codes)
        return run_list_codes(opts, sink);

    if (opts.explain.has_value())
        return run_explain(opts, *opts.explain, sink);

    if (opts.which == cli::command::none)
    {
        sink.out << cli::usage_text();
        return cli::to_int(cli::exit_code::ok);
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

}  // namespace

int run_with_library(
    std::span<std::string_view const> args, arcana::library_options lib_options, cli::streams sink
)
{
    return dispatch(cli::parse(args), std::move(lib_options), sink);
}

int run(std::span<std::string_view const> args, cli::streams sink)
{
    auto const parsed = cli::parse(args);

    sink.colored = cli::resolve_color(
        parsed.opts.color, parsed.opts.color_explicit, ambient_no_color(),
        isatty(STDOUT_FILENO) == 1
    );

    return dispatch(parsed, {}, sink);
}

}  // namespace cartomancer
