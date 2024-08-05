import os.path
import subprocess
import sys
import logging
from logging import info


def check_accept():
    info("testing handling of input file...")

    logging.basicConfig(level=logging.INFO)

    test_file_name = sys.argv[1]

    p = subprocess.run(["./gsc", "compile", test_file_name], capture_output=True, text=True)

    assert p.returncode == 0


def check_abort():
    info("testing handling of missing input file...")

    logging.basicConfig(level=logging.INFO)

    p = subprocess.run(["./gsc", "compile", "foo.gsc"], capture_output=True, text=True)

    assert p.returncode == 1


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    info("check if binary exists...")
    assert os.path.exists("./gsc")

    check_accept()
    check_abort()
