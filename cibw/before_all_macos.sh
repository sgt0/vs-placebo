#!/usr/bin/env sh
set -e

brew update
brew install shaderc glslang dovi_tool
brew install libplacebo --HEAD
