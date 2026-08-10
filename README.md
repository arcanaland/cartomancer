# Cartomancer

Cartomancer is a command-line tool for validating and inspecting tarot decks that conform to
Arcana Land's [Tarot Deck spec](https://github.com/arcanaland/specifications).

## Status

Cartomancer is currently undergoing a complete rewrite from Go to C++. This will be a breaking change.

Starting with v0.3:
- The v2 tarot deck spec is the only supported version.
- ANSI card rendering is currently not supported, but the plan is to reintroduce it

Decks are read from `$XDG_DATA_HOME/tarot/decks/`.

## Output

Text output is for people; `--format json` is for programs, and is the only surface with a
stable shape.

- **`list`'s table shows `VERSION` where it used to show the deck's `id`.** For every deck
  seen in practice `id` and the directory name are the same string, so the column was a
  third of the line repeated. `id` is still in `--format json`.
- **`--list-codes` prints aligned columns with a header, not tab-separated fields.** `cut -f`
  no longer works on it; use `--list-codes --format json`.
- **Colour follows `--color=auto|always|never|256|truecolor`**, [`NO_COLOR`](https://no-color.org),
  and whether stdout is a terminal. `256` and `truecolor` select a palette depth; the
  16-colour floor is what an unrecognised terminal gets, so light and dark themes both work
  unconfigured.
- **Glyphs follow the locale.** `LC_ALL`, `LC_CTYPE` or `LANG` naming a UTF-8 codeset gets
  `✓ ✗ ⚠ → … —`; anything else gets `[ok] [x] [!] -> ... -`. Set `LC_CTYPE=C` to force ASCII
  on a terminal whose font lacks them. Glyphs are independent of colour.
- **Width comes from the terminal**, overridden by `COLUMNS`. Piped output is never
  truncated unless `COLUMNS` says so.

## Building

```console
$ just build-image
$ just build
$ just test
```
