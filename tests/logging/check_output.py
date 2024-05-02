import subprocess


def run_logger_test():
    p = subprocess.run("bin/tests/logging/output", capture_output=True, text=True)

    # check exit code
    if p.returncode != 0:
        exit(p.returncode)

    output = p.stderr

    # check if logs appear in default log output (stderr)

    assert "logging some debug..." in output
    assert "logging some info..." in output
    assert "logging some warning..." in output
    assert "logging some error..." in output


if __name__ == "__main__":
    run_logger_test()
