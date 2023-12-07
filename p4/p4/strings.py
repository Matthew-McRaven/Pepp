def readStr(VM, addr):
	outStr = ""
	while (ch:=VM.memory.read_b8(addr, signed=False))  != 0: 
		outStr += chr(ch)
		addr += 1
	return outStr
