def readStr(VM, addr):
	outStr = ""
	while (ch:=VM.memory.read_b8(addr, signed=False))  != 0: 
		outStr += chr(ch)
		addr += 1
	return outStr
	
def readLenStr(VM, addr, len):
	outStr = ""
	for i in range(len): outStr += chr(VM.memory.read_b8(addr + i, signed=False))
	return outStr

