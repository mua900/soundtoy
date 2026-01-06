import subprocess
from typing import List

test_programs : List[str] = [
    "tree",
    "bytecode",
    "builtins",
]

dependecies : List[str] = [
    "../st/evaluator.cpp",
    "../st/common.cpp",
    "../st/builtin.cpp",
    "../st/expr.cpp",
]

def compile_tests():
    cmd = ["g++"]
    for dep in dependecies:
        cmd.append(dep)

    for test in test_programs:
        command_line = cmd.copy()
        out_name = test

        test_src = str(test + ".cpp")

        command_line.append(test_src)
        command_line.append("-o")
        command_line.append(out_name)

        print("Compiling {test}. cmd: ", command_line)

        compile_result = subprocess.run(command_line)
        if compile_result.returncode != 0:
            print("Error trying to compile {test}")
            exit()

def run_tests():
    for test in test_programs:
        result = subprocess.run(str("./" + test), capture_output=True, check=True)
        result.stdout  # TODO

compile_tests()
run_tests()
