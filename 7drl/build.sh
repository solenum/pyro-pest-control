#!/bin/sh

emmake make

cd build
emcc -O3 -s USE_SDL=2 -s USE_WEBGL2=0 -s ALLOW_MEMORY_GROWTH=1 -s EMULATE_FUNCTION_POINTER_CASTS=0 --closure 1 -flto --llvm-lto 1 --shell-file data/template.html --preload-file data rl.bc -o index.html
cd ..