import subprocess
import sys
import logging
from logging import info, error

BIN_DIR = "bin/tests/logging/"


def run_logger_test():
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


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)

    target = sys.argv[1]

    info(f"starting logging test suite with target: {target}")

    match target:
        case "check_output":
            run_logger_test()
        case "check_panic":
            run_check_panic()
        case _:
            error(f"unknown target: {target}")
            exit(1)
