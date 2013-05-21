#!/bin/sh

cd `dirname "$0"`

git clone --branch=master .. choice
git clone git://github.com/kripken/emscripten.git emscripten

./emscripten/emcc \
  --pre-js em_pre.js --post-js em_post.js \
  emscripten/system/lib/dlmalloc.c \
  choice/choice.c \
  choice/example.c \
  -o example.js
