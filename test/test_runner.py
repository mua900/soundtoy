import subprocess
from typing import List

test_programs : List[str] = [
    "tree",
    "bytecode",
    "builtins",
]

dependecies : List[str] = [
    "../st/common/common.cpp",
    "../st/evaluation/evaluator.cpp",
    "../st/evaluation/builtin.cpp",
    "../st/evaluation/expr.cpp",
    "../st/evaluation/bytecode.cpp",
    "../st/evaluation/api.cpp",
]

def compile_tests():
    cmd = ["g++"]

    cmd.append("-std=c++20")
    
    for dep in dependecies:
        cmd.append(dep)

    cmd.append("-I")
    cmd.append("../st/common")

    cmd.append("-I")
    cmd.append("../st/evaluation")

    for test in test_programs:
        command_line = cmd.copy()
        out_name = test

        test_src = str(test + ".cpp")

        command_line.append(test_src)
        command_line.append("-o")
        command_line.append(out_name)

        print(f"Compiling {test}. cmd: {command_line}")

        compile_result = subprocess.run(command_line)
        if compile_result.returncode != 0:
            print(f"Error trying to compile {test}")
            exit()

def run_tests():
    for test in test_programs:
        result = subprocess.run(str("./" + test), check=True)

compile_tests()
run_tests()
