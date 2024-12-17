import subprocess
import sys,os

class CMake:
    def __init__(self,cmake_path=None):
        self.cmake_path=cmake_path
    def build(self, cmake_root, build_path, cwd=None, stdout=None, stderr=None):
        env = os.environ
        out = subprocess.run([self.cmake_path,"-S", cmake_root,"-B", build_path], cwd=cwd, env=env, stdout=stdout,stderr=stderr)
        out.check_returncode()
        out = subprocess.run([self.cmake_path, "--build", build_path, "-j"],env=env,stdout=stdout,stderr=stderr)
        out.check_returncode()

