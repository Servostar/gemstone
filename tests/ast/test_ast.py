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
1 module
2 expr
3 value
4 value
5 value
6 while
7 if
8 else if
9 else
10 condition
11 decl
12 assign
13 def
14 value
15 +
16 -
17 *
18 /
19 &
20 |
21 ^
22 !
23 &&
24 ||
25 ^^
26 !!
27 ==
28 >
29 <
30 typecast
31 transmute
32 funcall
33 value
34 typedef
35 box
36 fun
37 value
38 list
39 expr list
40 arg list
41 param list
42 stmt list
43 ident list
44 value
45 type
46 value
47 value
48 value
49 -
50 parameter
51 value
52 parameter-declaration
53 address of
54 deref
55 ref
56 value
""" == p.stdout


def run_check_print_graphviz():
    info("started check print graphviz...")

    info("creating temporary folder...")

    if not os.path.exists("tmp"):
        os.mkdir("tmp")

    info("cleaning temporary folder...")

    if os.path.exists("tmp/graph.gv"):
        os.remove("tmp/graph.gv")

    if os.path.exists("tmp/graph.svg"):
        os.remove("tmp/graph.svg")

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
