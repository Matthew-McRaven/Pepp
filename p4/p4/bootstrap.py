def bootstrap(VM, nativeWords):
	for word in nativeWords: VM.nativeWord(word.FORTH["name"], word)
	
