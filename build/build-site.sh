#!/bin/sh

cd `dirname "$0"`

git clone --branch=master .. choice
git clone git://github.com/kripken/emscripten.git emscripten

./emscripten/emcc choice/choice.c choice/example.c \
  emscripten/system/lib/dlmalloc.c \
  --pre-js em_pre.js --post-js em_post.js \
  -o example.js
