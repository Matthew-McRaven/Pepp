class Input:
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


class Output:
    def __init__(self):
        self.buffer = ""
    def write(self, string):
        self.buffer += string
