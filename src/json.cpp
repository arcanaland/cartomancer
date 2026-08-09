// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "json.hpp"

#include <string>

namespace cartomancer::json
{

namespace
{

constexpr int indent_width = 2;

}  // namespace

document from_path(std::filesystem::path const& value)
{
    return value.string();
}

document from_path(std::optional<std::filesystem::path> const& value)
{
    if (!value.has_value())
        return nullptr;
    return from_path(*value);
}

document from_view(std::string_view value)
{
    return std::string{value};
}

void write(std::ostream& out, document const& doc)
{
    out << doc.dump(indent_width, ' ', false, document::error_handler_t::replace) << '\n';
}

}  // namespace cartomancer::json
