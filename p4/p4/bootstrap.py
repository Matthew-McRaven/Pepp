from .dictionary import Flags as _Flags, find as _find, cwa as _cwa, header as _header, writeTokens as _writeTokens
from .dictionary import dump as _dump
def bootstrap(VM, nativeWords):
	for word in nativeWords: 
		VM.nativeWord(word.FORTH["name"], word)
		if "pad" in word.FORTH: 
			word.pad = VM.tcb.here()
			VM.tcb.here(VM.tcb.here() + int(word.FORTH["pad"]))
	interpretWords = [
		(":", 0, ["DOCOL", "WORD", "CREATE", "LIT", "DOCOL", ",", "LATEST", "HIDDEN", "[", "EXIT"]),
		(";", _Flags.IMMEDIATE, ["DOCOL", "LIT", "EXIT", "COMMA", "LATEST", "FETCH", "HIDDEN", "LBRAC"])
	]
	for word in interpretWords:
		name, flags, tokenStrs = word
		entries = [_find(VM, len(token), token) for token in tokenStrs]
		if 0 in entries: raise Exception("That didn't work")
		_header(VM, name, True if (flags & _Flags.IMMEDIATE) else False)
		tokens = [_cwa(VM, entry) for entry in entries]
		_writeTokens(VM, tokens)
	
