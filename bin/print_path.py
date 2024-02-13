import sys
import os

with open("pythonpath.txt", "w") as f:
    f.write(f'{os.linesep.join([sys.prefix,*sys.path])}')
