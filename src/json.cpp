// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "json.hpp"

#include <array>
#include <cstdint>

namespace cartomancer::json
{

namespace
{

constexpr std::size_t indent_width = 2;
constexpr std::uint8_t first_printable = 0x20;
constexpr std::uint8_t nibble_bits = 4;
constexpr std::uint8_t nibble_mask = 0x0F;

}  // namespace

std::string escape(std::string_view value)
{
    std::string result;
    result.reserve(value.size());

    for (char const character : value)
    {
        auto const byte = static_cast<std::uint8_t>(character);
        switch (character)
        {
            case '"':
                result += R"(\")";
                break;
            case '\\':
                result += R"(\\)";
                break;
            case '\n':
                result += R"(\n)";
                break;
            case '\r':
                result += R"(\r)";
                break;
            case '\t':
                result += R"(\t)";
                break;
            case '\b':
                result += R"(\b)";
                break;
            case '\f':
                result += R"(\f)";
                break;
            default:
                if (byte < first_printable)
                {
                    static constexpr std::array<char, 17> hex = {"0123456789abcdef"};
                    result += R"(\u00)";
                    result += hex.at(byte >> nibble_bits);
                    result += hex.at(byte & nibble_mask);
                }
                else
                {
                    result += character;
                }
                break;
        }
    }

    return result;
}

void writer::indent()
{
    for (std::size_t level = 0; level < depth_ * indent_width; ++level)
        *out_ << ' ';
}

// Emit whatever separates this value from the previous one: a comma between
// siblings, a newline and indentation inside a container, nothing at all when
// we have just written a key.
void writer::punctuate()
{
    if (after_key_)
    {
        after_key_ = false;
        return;
    }

    if (depth_ == 0)
        return;

    if (!first_)
        *out_ << ',';

    *out_ << '\n';
    indent();
    first_ = false;
}

void writer::begin_object()
{
    punctuate();
    *out_ << '{';
    ++depth_;
    first_ = true;
}

void writer::end_object()
{
    --depth_;
    if (!first_)
    {
        *out_ << '\n';
        indent();
    }
    *out_ << '}';
    first_ = false;
}

void writer::begin_array()
{
    punctuate();
    *out_ << '[';
    ++depth_;
    first_ = true;
}

void writer::end_array()
{
    --depth_;
    if (!first_)
    {
        *out_ << '\n';
        indent();
    }
    *out_ << ']';
    first_ = false;
}

void writer::key(std::string_view name)
{
    punctuate();
    *out_ << '"' << escape(name) << "\": ";
    after_key_ = true;
}

void writer::string(std::string_view value)
{
    punctuate();
    *out_ << '"' << escape(value) << '"';
}

void writer::number(std::size_t value)
{
    punctuate();
    *out_ << value;
}

void writer::boolean(bool value)
{
    punctuate();
    *out_ << (value ? "true" : "false");
}

void writer::null()
{
    punctuate();
    *out_ << "null";
}

void writer::string_or_null(std::optional<std::string> const& value)
{
    if (value.has_value())
        string(*value);
    else
        null();
}

void writer::finish()
{
    *out_ << '\n';
}

}  // namespace cartomancer::json
