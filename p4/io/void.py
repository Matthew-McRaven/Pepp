import logging


class Input:
    def key(self): raise StopIteration()

    def peek(self): return None

class Output:
    def write(self, string): pass

__Log = logging.getLogger()
__Log.addFilter(lambda x: False)

def Log(): return __Log