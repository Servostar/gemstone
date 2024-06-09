#!/usr/bin/env sh

# Author: Sven Vogel
# Created: 02.05.2024
# Description: Builds the project and runs tests
#              Returns 0 on success and 1 when something went wrong

echo "+--------------------------------------+"
echo "| BUILDING all TARGETS                 |"
echo "+--------------------------------------+"

cmake .

make -B
if [ ! $? -eq 0 ]; then
  echo "===> failed to build targets"
  exit 1
fi

sh -c ./run-lib-build.sh

echo "+--------------------------------------+"
echo "| RUNNING CODE CHECK                   |"
echo "+--------------------------------------+"

make check
if [ ! $? -eq 0 ]; then
  echo "===> failed code check..."
  exit 1
fi

echo "+--------------------------------------+"
echo "| RUNNING TESTS                        |"
echo "+--------------------------------------+"

ctest -VV --output-on-failure --schedule-random -j 4
if [ ! $? -eq 0 ]; then
  echo "===> failed tests..."
  exit 1
fi

echo "+--------------------------------------+"
echo "| COMPLETED CHECK + TESTS SUCCESSFULLY |"
echo "+--------------------------------------+"
