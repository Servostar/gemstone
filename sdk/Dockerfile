FROM alpine:3.19.1
LABEL authors="servostar"
LABEL version="0.2.6"
LABEL description="base image for building the gemstone programming language compiler"
LABEL website="https://github.com/Servostar/gemstone"

# install dependencies
RUN apk add build-base gcc clang make cmake bison flex git python3 graphviz glib glib-dev llvm17-libs llvm17-dev

# create user for build
RUN adduser --disabled-password lorang
WORKDIR /home/lorang
USER lorang
