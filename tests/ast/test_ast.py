import subprocess
import sys
import logging
from logging import info, error
import os

BIN_DIR = "bin/tests/ast/"


def run_check_build_tree():
    info("started check tree build...")

    p = subprocess.run(BIN_DIR + "build_tree", capture_output=True, text=True)

    info("checking exit code...")

    # check exit code
    assert p.returncode == 0


def run_check_print_node():
    info("started check node print...")

    p = subprocess.run(BIN_DIR + "print_node", capture_output=True, text=True)

    info("checking exit code...")

    # check exit code
    assert p.returncode == 0


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)

    target = sys.argv[1]

    info(f"starting ast test suite with target: {target}")

    match target:
        case "check_build_tree":
            run_check_build_tree()
        case "check_print_node":
            run_check_print_node()
        case _:
            error(f"unknown target: {target}")
            exit(1)
