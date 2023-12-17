from .dictionary import Flags as _Flags, find as _find, cwa as _cwa, header as _header, writeTokens as _writeTokens
from .dictionary import dump as _dump, defforth as _def
def bootstrap(VM, nativeWords):
	for word in nativeWords: 
		VM.nativeWord(word.FORTH["name"], word, immediate=("immediate" in word.FORTH))
		if "pad" in word.FORTH: 
			word.pad = VM.tcb.here()
			VM.tcb.here(VM.tcb.here() + int(word.FORTH["pad"]))
	# Don't insert CWA of DOCOL. The CWA isn't actually executable. We must indirect the CWA to get an executable token.
	interpretWords = [
		(":", 0, ["WORD", "CREATE", "LIT", "DOCOL", ",", "LATEST", "HIDDEN", "[", "EXIT"]),
		(";", _Flags.IMMEDIATE, ["LIT", "EXIT", ",", "LATEST", "HIDDEN", "]", "EXIT"]),
		# Consumes the NEXT word in the input stream and marks it as hidden
		("HIDE", _Flags.IMMEDIATE, ["WORD", "FIND", "HIDDEN", "EXIT"]),
		# Not using JonesForth "cheat", since I did not understand the implementation.
		("'", _Flags.IMMEDIATE, ["WORD", "FIND", ">CWA", "?", "EXIT"]),
		# Used to restart interpreter
		#("QUIT", 0, "R0 RSP! INTERPRET BRANCH -6".split())
	]
	for word in interpretWords: _def(VM, word)
	
