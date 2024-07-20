import os.path
import subprocess
import logging
from logging import info


def check_build_and_run():
    info("testing compilation of hello world...")

    logging.basicConfig(level=logging.INFO)

    p = subprocess.run(["../../bin/check/gsc", "build", "release", "--verbose"], capture_output=True, text=True)

    print(p.stdout)

    assert p.returncode == 0

    p = subprocess.run(["bin/release.out"], capture_output=True, text=True)

    print(p.stdout)

    assert "Hello, world!" in p.stdout

if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    info("check if binary exists...")
    assert os.path.exists("../../bin/check/gsc")

    check_build_and_run()
