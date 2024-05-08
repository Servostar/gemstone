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

    assert """0 stmt
1 expr
2 value
3 value
4 value
5 while
6 if
7 else if
8 else
9 condition
10 decl
11 assign
12 def
13 value
14 +
15 -
16 *
17 /
18 &
19 |
20 ^
21 !
22 &&
23 ||
24 ^^
25 !!
26 ==
27 >
28 <
29 cast
30 as
31 value
32 value
33 typedef
34 box
35 fun
36 value
""" == p.stdout


def run_check_print_graphviz():
    info("started check print graphviz...")

    p = subprocess.run(BIN_DIR + "print_graphviz", capture_output=True, text=True)

    info("checking exit code...")

    # check exit code
    assert p.returncode == 0

    info("converting gv to svg...")

    p = subprocess.run(["dot", "-Tsvg", "tmp/graph.gv", "-otmp/graph.svg"])

    info("checking exit code...")
    assert p.returncode == 0

    info("checking svg output...")
    with open("tmp/graph.svg", "r") as file:
        string = "".join(file.readlines())
        assert "2.3" in string
        assert "0.79" in string
        assert "stmt" in string
        assert "if" in string


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)

    target = sys.argv[1]

    info(f"starting ast test suite with target: {target}")

    match target:
        case "check_build_tree":
            run_check_build_tree()
        case "check_print_node":
            run_check_print_node()
        case "check_print_graphviz":
            run_check_print_graphviz()
        case _:
            error(f"unknown target: {target}")
            exit(1)
