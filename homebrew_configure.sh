#!/bin/sh

rm -rf build
mkdir build
cd build

cmake -DReadline_INCLUDE_DIR=/usr/local/Cellar/readline/6.2.1/include \
      -DReadline_LIBRARY=/usr/local/Cellar/readline/6.2.1/lib/libreadline.dylib \
      -DCMAKE_INSTALL_PREFIX=~/software/ridl \
      ..
