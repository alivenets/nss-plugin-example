#!/bin/sh

rm -r sdbus-cpp
git clone https://github.com/Kistler-Group/sdbus-cpp.git -b v0.8.1
cd sdbus-cpp
mkdir build
cd build
cmake .. -DBUILD_CODE_GEN=ON -DBUILD_DOC=OFF -DCMAKE_INSTALL_PREFIX=/usr
make -j4 && sudo make install
