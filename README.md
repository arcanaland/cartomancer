# Cartomancer

Cartomancer is a command-line tool for validating and inspecting tarot decks that conform to
Arcana Land's [Tarot Deck spec](https://github.com/arcanaland/specifications).

## Status

Cartomancer is currently undergoing a complete rewrite from Go to C++. This will be a breaking change.

Starting with v0.3:
- The v2 tarot deck spec is the only supported version.
- ANSI card rendering is currently not supported, but the plan is to reintroduce it

Decks are read from `$XDG_DATA_HOME/tarot/decks/`.

## Building

```console
$ just build-image
$ just build
$ just test
```
