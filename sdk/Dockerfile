FROM alpine:3.19.1
LABEL authors="servostar"
LABEL version="0.2.5"
LABEL description="base image for building the gemstone programming language compiler"
LABEL website="https://github.com/Servostar/gemstone"

# install dependencies
RUN apk add build-base gcc make cmake bison flex git python3 graphviz glib glib-dev llvm17-dev

# create user for build
RUN adduser --disabled-password lorang
WORKDIR /home/lorang
USER lorang
