import subprocess
import sys
import logging
from logging import info, error
import os

BIN_DIR = "bin/tests/logging/"


def run_check_output():
    info("started check output...")

    p = subprocess.run(BIN_DIR + "output", capture_output=True, text=True)

    info("checking exit code...")

    # check exit code
    assert p.returncode == 0

    output = p.stderr

    # check if logs appear in default log output (stderr)
    info("checking stderr...")

    assert "logging some debug..." in output
    assert "logging some info..." in output
    assert "logging some warning..." in output
    assert "logging some error..." in output


def run_check_level():
    info("started check level...")

    p = subprocess.run(BIN_DIR + "level", capture_output=True, text=True)

    info("checking exit code...")

    # check exit code
    assert p.returncode == 0

    output = p.stderr

    # check if logs appear in default log output (stderr)
    info("checking stderr...")

    assert "logging some debug..." not in output
    assert "logging some info..." not in output
    assert "logging some warning..." in output
    assert "logging some error..." in output


def run_check_panic():
    info("started check panic...")

    p = subprocess.run(BIN_DIR + "panic", capture_output=True, text=True)

    info("checking exit code...")

    # check exit code
    assert p.returncode == 1

    output = p.stderr

    # check if logs appear (not) in default log output (stderr)
    info("checking stderr...")

    assert "before exit" in output
    assert "oooops something happened" in output
    assert "after exit" not in output


def run_check_stream():
    info("started check panic...")

    info("creating temporary folder...")

    if not os.path.exists("tmp"):
        os.mkdir("tmp")

    info("cleaning temporary folder...")

    if os.path.exists("tmp/test.log"):
        os.remove("tmp/test.log")

    info("launching test binary...")

    p = subprocess.run(BIN_DIR + "stream", capture_output=True, text=True)

    info("checking exit code...")

    # check exit code
    assert p.returncode == 0

    with open("tmp/test.log", "r") as file:
        assert "should be in both" in "".join(file.readlines())

    output = p.stderr

    # check if logs appear (not) in default log output (stderr)
    info("checking stderr...")

    assert "should only be in stderr" in output
    assert "should be in both" in output


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)

    target = sys.argv[1]

    info(f"starting logging test suite with target: {target}")

    match target:
        case "check_output":
            run_check_output()
        case "check_panic":
            run_check_panic()
        case "check_stream":
            run_check_stream()
        case "check_level":
            run_check_level()
        case _:
            error(f"unknown target: {target}")
            exit(1)
