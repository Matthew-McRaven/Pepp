def bootstrap(VM, nativeWords):
	for word in nativeWords: 
		VM.nativeWord(word.FORTH["name"], word)
		if "pad" in word.FORTH: 
			word.pad = VM.tcb.here()
			VM.tcb.here(VM.tcb.here() + int(word.FORTH["pad"]))
	
