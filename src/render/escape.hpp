// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Finding the escape sequences in text the deck wrote.

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace cartomancer::render
{

// The length in bytes of the escape sequence at the front of text, or 0 when
// text does not start with one.
//
// CSI is `ESC [`, then parameter bytes 0x30-0x3F, then intermediate bytes
// 0x20-0x2F, then exactly one final byte 0x40-0x7E. The Go tool terminated on
// `m` alone, so a cursor sequence such as `ESC[?25l` swallowed the rest of the
// line. A sequence truncated at the end of the input is consumed whole, and a
// lone ESC is one byte and nothing more.
[[nodiscard]] std::size_t escape_length(std::string_view text) noexcept;

// text with every escape sequence removed.
[[nodiscard]] std::string strip_escapes(std::string_view text);

// The columns text occupies, escapes not counted.
//
// Carries display_width()'s one-column-per-codepoint approximation; see the
// caveat on that function.
[[nodiscard]] std::size_t visible_width(std::string_view text) noexcept;

}  // namespace cartomancer::render
