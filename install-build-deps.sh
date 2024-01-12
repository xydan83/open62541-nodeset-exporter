#!/usr/bin/env bash

#
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
#  Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
#

# fail on errors
set -e

# check if run under root
# shellcheck disable=SC2046
if test $(id -u) != 0; then
  SUDO=sudo
fi

export CLANG_VERSION="${CLANG_VERSION:-14}"
export GCC_VERSION="${GCC_VERSION:-12}"

echo -e "\e[31mInstalling build dependencies for Linux Ubuntu...\e[0m"

function install_build_dependencies_ubuntu() {
  echo "Updating apt definitions..."
  ${SUDO} apt update
  echo "Installing build environment..."
  ${SUDO} apt -y --no-install-suggests --no-install-recommends install \
    cmake \
    clang-format-"${CLANG_VERSION}" \
    clang-tidy-"${CLANG_VERSION}" \
    gcc-"${GCC_VERSION}" \
    g++-"${GCC_VERSION}" \
    gcovr \
    libc-dev \
    libsnmp-dev \
    make \
    jq

  echo "Setting default compiler to gcc-${GCC_VERSION}..."
  ${SUDO} update-alternatives \
    --install /usr/bin/gcc gcc /usr/bin/gcc-"${GCC_VERSION}" 100 \
    --slave /usr/bin/g++ g++ /usr/bin/g++-"${GCC_VERSION}" \
    --slave /usr/bin/gcov gcov /usr/bin/gcov-"${GCC_VERSION}"

  echo "Setting default clang to clang-${CLANG_VERSION}..."
  ${SUDO} update-alternatives \
    --install /usr/bin/clang clang /usr/bin/clang-"${CLANG_VERSION}" 100 \
    --slave /usr/bin/clang++ clang++ /usr/bin/clang++-"${CLANG_VERSION}" \
    --slave /usr/bin/asan_symbolize asan_symbolize /usr/bin/asan_symbolize-"${CLANG_VERSION}" \
    --slave /usr/bin/c-index-test c-index-test /usr/bin/c-index-test-"${CLANG_VERSION}" \
    --slave /usr/bin/clang-check clang-check /usr/bin/clang-check-"${CLANG_VERSION}" \
    --slave /usr/bin/clang-cl clang-cl /usr/bin/clang-cl-"${CLANG_VERSION}" \
    --slave /usr/bin/clang-cpp clang-cpp /usr/bin/clang-cpp-"${CLANG_VERSION}" \
    --slave /usr/bin/clang-format clang-format /usr/bin/clang-format-"${CLANG_VERSION}" \
    --slave /usr/bin/clang-format-diff clang-format-diff /usr/bin/clang-format-diff-"${CLANG_VERSION}" \
    --slave /usr/bin/clang-import-test clang-import-test /usr/bin/clang-import-test-"${CLANG_VERSION}" \
    --slave /usr/bin/clang-include-fixer clang-include-fixer /usr/bin/clang-include-fixer-"${CLANG_VERSION}" \
    --slave /usr/bin/clang-offload-bundler clang-offload-bundler /usr/bin/clang-offload-bundler-"${CLANG_VERSION}" \
    --slave /usr/bin/clang-query clang-query /usr/bin/clang-query-"${CLANG_VERSION}" \
    --slave /usr/bin/clang-rename clang-rename /usr/bin/clang-rename-"${CLANG_VERSION}" \
    --slave /usr/bin/clang-reorder-fields clang-reorder-fields /usr/bin/clang-reorder-fields-"${CLANG_VERSION}" \
    --slave /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-"${CLANG_VERSION}" \
    --slave /usr/bin/lldb lldb /usr/bin/lldb-"${CLANG_VERSION}" \
    --slave /usr/bin/lldb-server lldb-server /usr/bin/lldb-server-"${CLANG_VERSION}"

  echo "Installing python 3..."
  ${SUDO} apt -y install \
    python3 \
    python3-pip

  echo "Installing Conan package manager..."
  pip3 install --no-cache-dir "conan>=1.0,<2.0"

  echo "WARNING: It is recommended to re-login / reboot for new tools to become available on \$PATH."
}

function install_build_dependencies_macos() {
  echo "Updating HomeBrew definitions..."
  brew update
  echo "Installing build environment..."
  brew install \
    clang-format \
    cmake \
    gcc \
    python@3.10
  echo "Installing Conan package manager..."
  pip3 install conan
  echo "WARNING: It is recommended to re-login / reboot for new tools to become available on \$PATH."
}

if [[ "${OSTYPE}" == "linux-gnu" ]]; then
  if [[ ! -f "/etc/os-release" ]]; then
    echo "Unable to detect operating system: /etc/os-release not found!"
    exit 1
  fi

  source /etc/os-release
  case ${ID} in
  ubuntu)
    case "${VERSION}" in
    *Focal* | *Jammy*)
      install_build_dependencies_ubuntu
      ;;
    *)
      echo "Unsupported Ubuntu version: ${VERSION}"
      exit 1
      ;;
    esac
    ;;
  *)
    echo "Unsupported Linux distribution: ${ID}"
    ;;
  esac
elif [[ "${OSTYPE}" == "darwin"* ]]; then
  install_build_dependencies_macos
else
  echo "Unsupported operating system: ${OSTYPE}"
fi
