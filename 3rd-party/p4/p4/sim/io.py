import p4.io.std, p4.io.fallback, p4.io.buffer


class IO:
    def __init__(self, _input, output, log):
        self._input = _input
        self.output = output
        self.log = log
    def buffer_text(self, text):
        buffer = p4.io.buffer.Input(text)
        self._input.current(buffer)
    def word_impl(self):
        ret, blank = "", ' \r\n\t'

        # Keep reading until we get at least one non-blank character.
        while True:
            ch = self._input.key()
            # "eat" all whitespace between words, but stop at a word boundary
            if ch in blank:
                if len(ret) > 0:
                    break
                else:
                    continue
            else:
                ret += ch

        return ret


def stdio():
    return IO(p4.io.fallback.Input(p4.io.std.Input()), p4.io.std.Output(), p4.io.std.Log())
