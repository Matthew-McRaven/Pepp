import subprocess
import sys,os

class CMake:
    def __init__(self,cmakePath=None):
        self.cmakePath=cmakePath
    def build(self,cmakeRoot,buildPath,cwd=None):
        env = os.environ
        env["CMAKE_PREFIX_PATH"]="/Users/matthewmcraven/Qt/6.8.0/macos"
        out = subprocess.run([self.cmakePath,"-S", cmakeRoot,"-B", buildPath], cwd=cwd, env=env)
        out = subprocess.run([self.cmakePath, "--build", buildPath, "-j"],env=env)

