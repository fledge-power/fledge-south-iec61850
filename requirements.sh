#!/usr/bin/env bash

proj_directory=$(pwd)

echo $proj_directory
directory=$1
if [ ! -d $directory ]; then
  mkdir -p $directory
else
  directory=~
fi

if [ ! -d $directory/libiec61850 ]; then
  cd $directory
  echo Fetching MZA libiec61850 library
  git clone https://github.com/mz-automation/libiec61850.git
  cd libiec61850
  cd third-party/mbedtls
  wget -c https://github.com/Mbed-TLS/mbedtls/archive/refs/tags/v2.28.3.tar.gz -O - | tar -xz
  mv mbedtls-2.28.3 mbedtls-2.28
  cd ../../
  mkdir build
  cd build
  cmake -DBUILD_TESTS=NO -DBUILD_EXAMPLES=NO ..
  make
  sudo make install
fi

 