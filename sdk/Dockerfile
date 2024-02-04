FROM almalinux:9.3
LABEL authors="servostar"
LABEL version="0.1.0"
LABEL description="base image for building the gemstone programming language compiler"
LABEL website="https://github.com/Servostar/gemstone"

# install dependencies
RUN yum install cmake gcc flex byacc -y

# create user for build
RUN adduser lorang
WORKDIR /home/lorang
USER lorang