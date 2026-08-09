// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

namespace cartomancer::json
{

// Minimal pretty-printing JSON writer.
//
// ADR-009 makes every field name in --format json part of the CLI's API, and
// the two shapes it fixes are small and closed, so this is a writer rather
// than a document model. It tracks nesting only well enough to place commas
// and indentation.
class writer
{
  public:
    explicit writer(std::ostream& out) : out_(&out) {}

    void begin_object();
    void end_object();

    void begin_array();
    void end_array();

    // Write a key. The next call writes its value.
    void key(std::string_view name);

    void string(std::string_view value);
    void number(std::size_t value);
    void boolean(bool value);
    void null();

    // ADR-009: empty optionals are present and null, never omitted, so a
    // consumer can index without a membership test.
    void string_or_null(std::optional<std::string> const& value);

    // Finish the document with a trailing newline.
    void finish();

  private:
    void punctuate();
    void indent();

    std::ostream* out_;
    std::size_t depth_ = 0;
    bool first_ = true;
    bool after_key_ = false;
};

// The JSON string escape, exposed for tests.
[[nodiscard]] std::string escape(std::string_view value);

}  // namespace cartomancer::json
