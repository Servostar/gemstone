FROM servostar/gemstone:sdk-0.1.0-alma-9.3
LABEL authors="servostar"
LABEL version="0.1.0"
LABEL description="docker image for setting up the build pipeline on SDK"
LABEL website="https://github.com/Servostar/gemstone"

COPY --chown=lorang src /home/lorang/src
COPY --chown=lorang CMakeLists.txt /home/lorang/

RUN cmake .