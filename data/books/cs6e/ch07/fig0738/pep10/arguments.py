from pep10.symbol import SymbolEntry


class Identifier:
    def __init__(self, symbol: SymbolEntry):
        self.symbol: SymbolEntry = symbol

    def __int__(self):
        return int(self.symbol)

    def __str__(self):
        return str(self.symbol)


class Hexadecimal:
    def __init__(self, value: int):
        self.value: int = value

    def __int__(self):
        return self.value

    def __str__(self):
        # 0-padded, 4-length hex-string
        return f"0x{self.value:04x}"


class Decimal:
    def __init__(self, value: int):
        self.value: int = value

    def __int__(self):
        return self.value

    def __str__(self):
        return f"{self.value}"


# SOLUTION START
class StringConstant:
    def __init__(self, value: bytes):
        self.value: bytes = value

    def __int__(self) -> int:
        return int.from_bytes(self.value)

    def __str__(self):
        # ret[0:1] ==b' and ret[-1] == '
        # Need to drop binary prefix and replace single with double quotes
        ret = repr(self.value)[2:-1]
        # Repr singles quotes our string by default, so doubles quotes are unescaped.
        ret = ret.replace('"', '\\"')
        return f'"{ret}"'


# SOLUTION END
