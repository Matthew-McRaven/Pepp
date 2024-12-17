import subprocess
import sys,os

class CMake:
    def __init__(self,cmakePath=None):
        self.cmakePath=cmakePath
    def build(self,cmakeRoot,buildPath,cwd=None):
        out = subprocess.run([self.cmakePath,"-S", cmakeRoot,"-B", buildPath], cwd=cwd, env=os.environ)
        out = subprocess.run([self.cmakePath, "--build", buildPath, "-j"],env=os.environ)
