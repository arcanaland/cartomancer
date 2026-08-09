# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, cmake_layout


class CartomancerConan(ConanFile):
    name = "cartomancer"
    version = "0.3.0"
    settings = "os", "compiler", "build_type", "arch"

    # `arcana` is deliberately absent: libarcana ships no consumable Conan
    # package, so it is found via CMAKE_PREFIX_PATH against an install prefix.
    # Nothing else belongs here -- in particular NOT tomlplusplus, which
    # arcana keeps private to its exports as of libarcana@87b0fb9.
    requires = "nlohmann_json/3.12.0"
    test_requires = "catch2/3.15.2"

    def layout(self):
        cmake_layout(self)

    def generate(self):
        toolchain = CMakeToolchain(self, generator="Ninja")
        toolchain.generate()
        CMakeDeps(self).generate()
