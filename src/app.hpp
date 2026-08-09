// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "cli/streams.hpp"

#include <arcana/library.hpp>

#include <span>
#include <string_view>

namespace cartomancer
{

// Parse `args` (argv without argv[0]) and run the requested command.
//
// @return the process exit code
int run(std::span<std::string_view const> args, cli::streams sink);

// Parse `args` (argv without argv[0]) and run the requested command
// with a specific deck library.
//
// @return the process exit code
int run_with_library(
    std::span<std::string_view const> args, arcana::library_options lib_options, cli::streams sink
);

}  // namespace cartomancer
