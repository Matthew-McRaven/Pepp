import sys
import logging

# Helper class to buffer values returned from input()
class Input:
    def __init__(self):
        self.buffer = ""

    def key(self):
        if self.buffer:
            ret = self.buffer[0]
            self.buffer = self.buffer[1:]
            return ret
        else:
            self.buffer = input("$").rstrip() + "\n"
            return self.key()

    def peek(self):
        if self.buffer:
            return self.buffer[0]
        else:
            return None

class Output:
    def write(self, char):
        print(char, end="")


# Standard error will conform to logging API.
def Log(): return logging.getLogger()
