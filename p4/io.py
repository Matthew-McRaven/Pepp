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


class File:
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


class SwitchBuffer:
    def __init__(self):
        self.__stdin = STDIN()
        self.which = self.__stdin

    def file(self, text):
        self.which = File(text)

    def key(self):
        try:
            return self.which.key()
        except StopIteration:
            self.which = self.__stdin
            return self.key()

    def peek(self):
        return self.which.peek()


__stdin = SwitchBuffer()


def stdin(): return __stdin


def open_file(text): __stdin.file(text)


def word_impl():
    ret = ""
    blank = ' \r\n\t'

    # Keep reading until we get at least one non-blank character.
    while True:
        ch = stdin().key()
        # "eat" all whitespace between words, but stop at a word boundary
        if ch in blank:
            if len(ret) > 0: break
            else: continue
        else: ret += ch

    return ret
