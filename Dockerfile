FROM servostar/gemstone:sdk-0.2.1-alpine-3.19.1
LABEL authors="servostar"
LABEL version="0.2.1"
LABEL description="docker image for setting up the build pipeline on SDK"
LABEL website="https://github.com/Servostar/gemstone"

RUN git clone https://github.com/Servostar/gemstone.git /home/lorang

RUN cmake .
