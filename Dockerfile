FROM servostar/gemstone:sdk-0.2.5-alpine-3.19.1
LABEL authors="servostar"
LABEL version="0.2.5"
LABEL description="docker image for setting up the build pipeline on SDK"
LABEL website="https://github.com/Servostar/gemstone"

COPY --chown=lorang src /home/lorang/src
COPY --chown=lorang tests /home/lorang/tests
COPY --chown=lorang CMakeLists.txt /home/lorang/
COPY --chown=lorang run-check-test.sh /home/lorang/
COPY --chown=lorang .env /home/lorang/
COPY --chown=lorang run-docker-build.sh /home/lorang/
COPY --chown=lorang run-lib-build.sh /home/lorang/
COPY --chown=lorang dep /home/lorang/dep
COPY --chown=lorang .git /home/lorang/.git

RUN cmake .
