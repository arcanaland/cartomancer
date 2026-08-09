# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, cmake_layout


class CartomancerConan(ConanFile):
    name = "cartomancer"
    version = "0.3.0"
    settings = "os", "compiler", "build_type", "arch"

    # TODO: add arcana here when we package it as a conan package
    requires = "nlohmann_json/3.12.0"
    test_requires = "catch2/3.15.2"

    def layout(self):
        cmake_layout(self)

    def generate(self):
        toolchain = CMakeToolchain(self, generator="Ninja")
        toolchain.generate()
        CMakeDeps(self).generate()
