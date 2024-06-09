#!/bin/sh

# Author: Sven Vogel
# Created: 25.05.2024
# Description: Builds the standard library into bin/std

echo "+--------------------------------------+"
echo "| CONFIGURE STD LIBRARY                |"
echo "+--------------------------------------+"

cmake lib
if [ ! $? -eq 0 ]; then
  echo "===> failed to configure build"
  exit 1
fi

echo "+--------------------------------------+"
echo "| BUILD STD LIBRARY                    |"
echo "+--------------------------------------+"

cd lib || exit 1
make -B
if [ ! $? -eq 0 ]; then
  echo "===> failed to build standard library"
  exit 1
fi

echo "+--------------------------------------+"
echo "| successfully build standard library  |"
echo "+--------------------------------------+"
