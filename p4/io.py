# Helper class to buffer values returned from input()
class STDIN:
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


class InFile:
    def __init__(self, text):
        self.buffer = text

    def key(self):
        if self.buffer:
            ret = self.buffer[0]
            self.buffer = self.buffer[1:]
            return ret
        else:
            raise StopIteration()

    def peek(self):
        if self.buffer:
            return self.buffer[0]
        else:
            return None

class STDOUT:
    def write(self, char):
        print(char, end="")
class OutBuffer:
    def __init__(self):
        self.buffer = ""
    def write(self, string):
        self.buffer += string

class SwitchBuffer:
    def __init__(self):
        self.__stdin = STDIN()
        self.__in_buf = self.__stdin
        self.__stdout = STDOUT()
        self.__out_buf = self.__stdout

    def in_buf(self, text):
        self.__in_buf = InFile(text)

    def key(self):
        try:
            return self.__in_buf.key()
        except StopIteration:
            self.__in_buf = self.__stdin
            return self.key()

    def peek(self):
        return self.__in_buf.peek()

    def write(self, string):
        self.__out_buf.write(string)
    def out_buf(self, out_buf):
        if out_buf is None: self.__out_buf = self.__stdout
        else: self.__out_buf = out_buf


__io = SwitchBuffer()


def io(): return __io


def open_file(text): __io.in_buf(text)

def buffer_output(): __io.out_buf(OutBuffer())

def word_impl():
    ret = ""
    blank = ' \r\n\t'

    # Keep reading until we get at least one non-blank character.
    while True:
        ch = io().key()
        # "eat" all whitespace between words, but stop at a word boundary
        if ch in blank:
            if len(ret) > 0: break
            else: continue
        else: ret += ch

    return ret
