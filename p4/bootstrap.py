import p4.dictionary
from .dictionary import Flags as _Flags, find as _find, cwa as _cwa, header as _header, writeTokens as _writeTokens
from .dictionary import dump as _dump, defforth as _def

def bootstrap(VM, nativeWords):
	referers, words = [], {word.FORTH["name"]:word for word in nativeWords}

	for word in sorted(nativeWords, key=lambda item: item.FORTH["priority"]):
		VM.nativeWord(word.FORTH["name"], word, immediate=("immediate" in word.FORTH))
		if "pad" in word.FORTH: 
			word.pad = VM.tcb.here()
			VM.tcb.here(VM.tcb.here() + int(word.FORTH["pad"]))
		if "refs" in word.FORTH: referers.append(word)

	for word in referers:
		# Change list to dict
		word.FORTH["refs"] = {referent:words[referent] for referent in word.FORTH["refs"]}