# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

FROM fedora:44

RUN dnf install -y --setopt=install_weak_deps=False \
        gcc-c++ \
        cmake \
        ninja-build \
        pkgconf-pkg-config \
        git \
        clang-tools-extra \
        python3-pip \
        gdb \
        libasan \
        libubsan \
    && dnf clean all

RUN pip install --no-cache-dir conan reuse

WORKDIR /src
