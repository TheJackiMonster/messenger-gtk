#!/bin/sh
cd "${MESON_SOURCE_ROOT}"
if test -d "./.git"
then
  git submodule init
  git submodule update
fi
