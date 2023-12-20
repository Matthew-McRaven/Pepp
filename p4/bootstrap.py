from .dictionary import Flags as _Flags, find as _find, cwa as _cwa, header as _header, writeTokens as _writeTokens
from .dictionary import dump as _dump, defforth as _def
def bootstrap(VM, nativeWords):
	referers, words = [], {word.FORTH["name"]:word for word in nativeWords}
	for word in nativeWords:
		VM.nativeWord(word.FORTH["name"], word, immediate=("immediate" in word.FORTH))
		if "pad" in word.FORTH: 
			word.pad = VM.tcb.here()
			VM.tcb.here(VM.tcb.here() + int(word.FORTH["pad"]))
		if "refs" in word.FORTH: referers.append(word)

	# Don't insert CWA of DOCOL. The CWA isn't actually executable. We must indirect the CWA to get an executable token.
	interpretWords = [
		# Fetch opcode for ENTER to avoid needless pointer chase at runtime.
		(":", 0, ["WORD", "CREATE", "LIT", "ENTER", "@", ",", "LATEST", "HIDDEN", "[", "EXIT"]),
		# Fixup Code Len
		("FCL", 0, "LATEST >CODELEN LATEST >CWA HERE - TRUNC !c EXIT".split()),
		# Fetch opcode for EXIT to avoid needless pointer chase at runtime.
		(";", _Flags.IMMEDIATE, ["LIT", "EXIT", "@", ",", "LATEST", "HIDDEN", "]", "FCL", "EXIT"]),
		# Consumes the NEXT word in the input stream and marks it as hidden
		("HIDE", _Flags.IMMEDIATE, ["WORD", "FIND", "HIDDEN", "EXIT"]),
		# Not using JonesForth "cheat", since I did not understand the implementation.
		("'", _Flags.IMMEDIATE, ["WORD", "FIND", ">CWA", "@", "EXIT"]),
		#("IF", _Flags.IMMEDIATE, "' 0BRANCH , HERE @ 0 ,".split())
		# Used to restart interpreter
		#("QUIT", 0, "P0 PSP! R0 RSP! INTERPRET BRANCH -6".split())
	]
	for word in interpretWords: _def(VM, word)

	for word in referers:
		# Change list to dict
		word.FORTH["refs"] = {referent:words[referent] for referent in word.FORTH["refs"]}