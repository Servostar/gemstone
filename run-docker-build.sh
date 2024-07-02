#!/usr/bin/env sh

# Author: Sven Vogel
# Created: 17.05.2024
# Description: Builds the Dockerfile for SDK and DEVKIT

echo "+--------------------------------------+"
echo "| CHECKING prelude                     |"
echo "+--------------------------------------+"

if [ -z "$SDK" ]; then
  echo "no SDK specified, sourcing .env"
  source ./.env

  if [ -z "$SDK" ]; then
    echo "no SDK specified"
    exit 1
  else
    echo "using SDK $SDK"
  fi

else
  echo "using SDK $SDK"
fi

echo "+--------------------------------------+"
echo "| BUILDING SDK                         |"
echo "+--------------------------------------+"

docker build --tag servostar/gemstone:sdk-"$SDK" sdk/.
if [ ! $? -eq 0 ]; then
  echo "===> failed to build sdk"
  exit 1
fi

echo "+--------------------------------------+"
echo "| BUILDING DEVKIT                      |"
echo "+--------------------------------------+"

docker build --tag servostar/gemstone:devkit-"$SDK" .
if [ ! $? -eq 0 ]; then
  echo "===> failed to build devkit"
  exit 1
fi

echo "+--------------------------------------+"
echo "| RUNNING check test                   |"
echo "+--------------------------------------+"

docker run --rm --name "devkit-$SDK-check-test"- servostar/gemstone:devkit-"$SDK" sh run-check-test.sh
if [ ! $? -eq 0 ]; then
  echo "===> failed to run build or checks"
  exit 1
fi

echo "+--------------------------------------+"
echo "| DONE                                 |"
echo "+--------------------------------------+"
