from typing import Dict


class SymbolEntry:
    def __init__(self, name: str):
        self.name: str = name
        self.definition_count: int = 0
        self.value: int | None = None

    def is_undefined(self):
        return self.definition_count == 0

    def is_multiply_defined(self):
        return self.definition_count > 1

    def __int__(self) -> int:
        return 0 if self.value is None else self.value

    def __str__(self):
        return self.name


class SymbolTable:
    def __init__(self) -> None:
        self._table: Dict[str, SymbolEntry] = {}

    def reference(self, name: str) -> SymbolEntry:
        if name not in self._table:
            self._table[name] = SymbolEntry(name)
        return self._table[name]

    def define(self, name: str) -> SymbolEntry:
        (sym := self.reference(name)).definition_count += 1
        return sym

    def __contains__(self, name: str) -> bool:
        return name in self._table

    def __getitem__(self, name: str):
        return self._table[name]


def add_OS_symbols(st: SymbolTable):
    st.define("pwrOff").value = 0xFFFF
    st.define("charOut").value = 0xFFFE
    st.define("charIn").value = 0xFFFD
    st.define("DECI").value = 0
    st.define("DECO").value = 1
    st.define("HEXO").value = 2
    st.define("STRO").value = 3
    st.define("SNOP").value = 4
