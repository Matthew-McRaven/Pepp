from .dictionary import Flags as _Flags, find as _find, cwa as _cwa, header as _header, writeTokens as _writeTokens
from .dictionary import dump as _dump
def bootstrap(VM, nativeWords):
	for word in nativeWords: 
		VM.nativeWord(word.FORTH["name"], word)
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
	]
	
	# Find the interpreter for FORTH words, and grab the "machine code" which implements it. 
	docol_cwa = _cwa(VM, _find(VM, len("DOCOL"), "DOCOL"))
	docol = [VM.memory.read_b16(docol_cwa, False)]
	
	for word in interpretWords:
		name, flags, tokenStrs = word
		entries = [_find(VM, len(token), token) for token in tokenStrs]
		#print(*zip(tokenStrs, entries)) # Debug helper when entries contains 0.
		if 0 in entries: raise Exception("That didn't work")
		_header(VM, name, True if (flags & _Flags.IMMEDIATE) else False) 
		# Prepend the implementation of DOCOL, so that the first cell at CWA for each word is executable. 
		tokens = docol + [_cwa(VM, entry) for entry in entries]
		_writeTokens(VM, tokens)
	
