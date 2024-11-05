class Input:
    def __init__(self, backup):
        self.backup = backup
        self._current = self.backup

    def current(self, current=None):
        if current is None: self._current = self.backup
        else: self._current = current

    def key(self):
        try:
            return self._current.key()
        except StopIteration:
            self._current = self.backup
            return self.key()

    def peek(self):
        return self._current.peek()
