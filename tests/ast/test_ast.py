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
37 fun
38 fun
39 fun
40 value
41 list
42 expr list
43 arg list
44 param list
45 stmt list
46 ident list
47 value
48 type
49 value
50 value
51 value
52 -
53 parameter
54 value
55 parameter-declaration
56 address of
57 deref
58 ref
59 value
60 value
61 ret
""" == p.stdout


def run_check_print_graphviz():
    info("started check print graphviz...")

    info("creating temporary folder...")

    if not os.path.exists("tmp"):
        os.makedirs("tmp", exist_ok=True)

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
