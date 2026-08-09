# Cartomancer

Cartomancer is a command-line tool for validating and inspecting tarot decks that conform to
the [Tarot Deck Specification](https://github.com/arcanaland/specifications). It is a thin
client over [`libarcana`](https://github.com/arcanaland/libarcana), which owns the deck
model and the diagnostic catalogue; Cartomancer owns the command surface and the terminal.

## Commands

```
cartomancer validate [TARGET]      judge a deck against the rule catalogue
  --format text|json               default text
  --level pedantic|info|warning|error   default info
  --explain CODE                   print one catalogue entry and exit
  --list-codes                     print the whole catalogue and exit

cartomancer list                   the decks installed on this system

Global: --deck NAME, --format WHAT, --color WHEN, --no-color, --version, --help
```

`TARGET` is overloaded on purpose: a path that exists on disk is loaded directly, and
anything else is looked up as a directory name in the deck library. Which deck a command
acts on is resolved as `--deck NAME`, then positional `TARGET`, then the current directory.
Passing both `--deck` and `TARGET` is a usage error rather than a silent precedence win.

`--color WHEN` takes `auto|always|never|256|truecolor`, and `--no-color` is an exact alias
for `--color=never`. [`NO_COLOR`](https://no-color.org/) with any non-empty value forces
`never`, unless `--color` is given explicitly.

Decks are read from `$XDG_DATA_HOME/tarot/decks/`.

## Exit codes

`validate` is meant to gate commits, so the mapping from deck state to integer is a
contract:

| Code | Meaning |
|---|---|
| `0` | No diagnostics at or above `--level` |
| `1` | Warnings at or above `--level`, and no errors |
| `2` | At least one `error` diagnostic |
| `3` | The deck could not be loaded at all — absent, unreadable, or a malformed manifest |
| `4` | Usage error — unknown flag or subcommand, or two deck selectors |

`3` is deliberately distinct from `2`: `2` says "I read this deck and it is
non-conforming", `3` says "I could not read it, so I am telling you nothing about its
conformance." A CI job that conflates them reports a broken checkout as a spec violation.

`--level` is a threshold rather than a display filter. Diagnostics below it are neither
printed nor counted toward the exit code, which makes `--level error` the CI invocation
that tolerates warnings.

`list` exits `0` even when it finds malformed decks — reporting them is its job.

## Machine-readable output

`--format json` writes one JSON object on stdout, and every field name in it is API:

```console
$ cartomancer validate --format json some-deck | jq .summary
{ "error": 1, "warning": 0, "info": 0, "pedantic": 0 }
```

`summary` counts *reported* diagnostics, i.e. after the `--level` floor, so it always
reconciles with the `diagnostics` array beside it. Empty optionals (`card`, `path`, `key`,
`author`, `icon`) are present and `null`, never omitted, so a consumer can index without a
membership test.

## Known limitations

- **`[deck].schema_version` is not dispatched on.** `libarcana`'s loader reads every deck
  under 1.0 rules regardless of the version it declares, so a 2.0 deck is parsed as 1.0.
  This is a library limitation, not a Cartomancer one, and it will otherwise be diagnosed
  later as a Cartomancer bug.
- **`--list-codes` and `--explain` carry no implementation-state column.** They report the
  whole catalogue, but `arcana::rule` exposes nothing that distinguishes a rule with a
  check body from one that is merely catalogued, so Cartomancer cannot say which of the
  rules it lists are actually enforced. Hardcoding the implemented subset would turn an
  honesty feature into a lie, so it is left out until the library exposes the state.
- **`show` and `draw` are specified but not built.** They need an image decoder, a
  quantizer and a CSI-correct escape scanner, none of which exist yet. They are not
  stubbed: invoking them is an unknown-subcommand usage error.
- **No `deck` subcommand and no config file.** The Go implementation had both. `deck`'s
  listing behaviour is now `list`; its other subcommands and the configured default deck
  have no replacement, and the default deck is the library's reference deck instead.
- **No release artifact.** The `goreleaser` pipeline built the Go binary and is gone. A
  static musl build is a later task.

## Building

C++26, CMake ≥ 4.3, Conan 2, Catch2 — the same floor as `libarcana`, because Cartomancer
links it. Everything runs inside a Podman container via the `justfile`:

```console
$ just build-image
$ just build
$ just test
```

`libarcana` does not yet publish a consumable Conan package, so Cartomancer finds it with
`find_package(arcana CONFIG REQUIRED)` against an install prefix. Stage one first:

```console
$ just -f ../libarcana/justfile install
```

That lands at `../libarcana/build/RelWithDebInfo/stage/usr/local`, which
`scripts/podman-shim.sh` mounts into the build container at `/opt/arcana`. Set
`ARCANA_PREFIX` to point it somewhere else.

The rest of the gate:

```console
$ just check-format    # clang-format
$ just tidy            # clang-tidy
$ just lint-reuse      # REUSE 3.3 compliance
```

## Licence

MIT. See [`LICENSE`](LICENSE). The repository is [REUSE](https://reuse.software/) 3.3
compliant.
