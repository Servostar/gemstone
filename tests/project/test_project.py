import subprocess
import logging
from logging import info, error

if __name__ == "__main__":
    info("started check output...")

    p = subprocess.run([ "../../bin/debug/gsc", "build", "all" ], capture_output=True, text=True)

    info("checking exit code...")

    print(p.stdout)

    # check exit code
    assert p.returncode == 0
