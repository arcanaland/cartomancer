# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# Recipes marked [script] run inside a podman container
set script-interpreter := ['./scripts/podman-shim.sh']

image := "cartomancer-builder"
build_root := "build"
build_type := "RelWithDebInfo"
build_dir := build_root / build_type
preset := "conan-" + lowercase(build_type)

# Where scripts/podman-shim.sh mounts libarcana's staged install prefix.
arcana_prefix := "/opt/arcana"

export CARTOMANCER_IMAGE := image

default:
    @just --list

# Build the container image.
build-image:
    podman build -t {{image}} -f Containerfile .

# Conan and cmake
[script]
configure:
    conan profile path default >/dev/null 2>&1 || conan profile detect

    conan install . --build=missing \
        -s build_type={{build_type}} \
        -s compiler.cppstd=26

    cmake --preset {{preset}} \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DCMAKE_PREFIX_PATH={{arcana_prefix}}

# Full CMake build.
[script]
build: configure
    cmake --build --preset {{preset}}

# Run the test suite.
[script]
test: build
    ctest --preset {{preset}} --output-on-failure

# Run the ctest cases matching a regex, e.g. `just test-match 'validate'`.
[script]
test-match pattern: build
    ctest --preset {{preset}} --output-on-failure -R '{{pattern}}'

# Run the test binary directly, passing the rest to Catch2.
# e.g. `just run-test --list-tests`
#      `just run-test '[validate]' -s`
[script]
run-test *args: build
    {{build_dir}}/tests/cartomancer_test {{args}}

# Run the built CLI
[script]
run *args: build
    {{build_dir}}/src/cartomancer {{args}}

# Run the test binary under gdb
[script]
debug-test *args: build
    gdb -q -ex run --args {{build_dir}}/tests/cartomancer_test --break {{args}}

# Run an arbitrary command inside the build container.
[script]
sh +cmd:
    {{cmd}}

# clang-format in place.
[script]
format *files:
    clang-format -i {{ if files == "" { "$(git ls-files '*.cpp' '*.hpp')" } else { files } }}

# Check the clang-format
[script]
check-format:
    clang-format --dry-run --Werror $(git ls-files '*.cpp' '*.hpp')

# clang-tidy in place
[script]
tidy: configure
    run-clang-tidy -quiet -p {{build_dir}} $(git ls-files 'src/*.cpp')

# Check that every file declares its copyright and licence (REUSE 3.3).
[script]
lint-reuse:
    reuse lint

# Trash build artifacts
clean:
    rm -rf {{build_root}} CMakeUserPresets.json
