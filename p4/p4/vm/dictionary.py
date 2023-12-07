import binascii
from ..strings import readStr
class DictAccessor:
	IMMEDIATE = 0x80
	HIDDEN = 0x20
	LEN = 0x1f
	
	def __init__(self, VM):
		self.VM = VM
		
	@staticmethod
	def header(VM, name, immediate=False, hidden=False):
		# u16 (link)
		VM.memory.write_b16(VM.herePP(2), VM.tcb.latest, signed=False);
		VM.tcb.latest = VM.tcb.here - 2
		# u8 (number of token bytes)
		VM.memory.write_b8(VM.herePP(1), 0, signed=False)
		# u8 (flags | string length)
		VM.memory.write_b8(VM.herePP(1), 
			(DictAccessor.IMMEDIATE if immediate else 0)
			| (DictAccessor.HIDDEN if hidden else 0)
			| (len(name) & DictAccessor.LEN), signed=False)
		# n * u8 name string; plus 1 or 2 u8 of null
		for letter in bytearray(name, "utf-8"): VM.memory.write_b8(VM.herePP(1), letter, signed=False)
		# Always null terminate strings, and place next words on a 16b boundary
		# So, pad evens with 2*null, odds with 1.
		if VM.tcb.here % 2 == 0: VM.memory.write_b8(VM.herePP(1), 0, signed=False)
		VM.memory.write_b8(VM.herePP(1), 0, signed=False)
		# Helper to dump the bytes of the entry
		#print(binascii.hexlify(VM.memory[VM.latest:VM.here]).decode("utf-8"))
		
	@staticmethod
	def defcode(VM, name, tokens, immediate=False):
		DictAccessor.header(VM, name, immediate=immediate)
		# Needed to return head of code field
		cwa = VM.tcb.here
		# n*u16code list
		for token in tokens: VM.memory.write_b16(VM.herePP(2), token, signed=True if token<0 else False);
		# Update code length field
		VM.memory.write_b8(VM.tcb.latest+2, 2*len(tokens), signed=False)
		#print(f"Defined {(10*' '+name)[-10:]}, from {(2*'0'+hex(old)[2:])[-2:]:2}..{(2*'0'+hex(VM.here)[2:])[-2:]:2}; strlen {len(name):2}, memlen is {VM.here-old:2}")
		return cwa
		
	# Yields a pointer into the dict
	def find(self, name): pass
	def entry(self, address):
		ret = {}
		ret["link"] = self.VM.memory.read_b16(address, signed=False); address += 2
		ret["codelen"] = self.VM.memory.read_b8(address, signed=False); address += 1
		ret["strlen"] = self.VM.memory.read_b8(address, signed=False) & DictAccessor.LEN; address += 1
		ret["str"] = address
		# Add 2 and mask out low bit to get actual length of string
		# EVEN: 2 => 4 => 4, which is right because we pad evens with 2 nulls
		# ODD: 1 =>3 => 2, which is right because we pad odds with 1 null
		ret["cwa"] = address + ((ret["strlen"] + 2) & 0xFE)
		return ret
		
	def dump(self): 	
		current, prev = self.VM.tcb.latest, 0
		while	current != 0:
			entry = self.entry(current)
			cwa = entry["cwa"]
			exec_token = self.VM.memory.read_b16(cwa, signed=True)
		
			strs = []
			for i in range(entry["codelen"]//2): strs.append((4*"0" + hex(self.VM.memory.read_b16(cwa+2*i,signed=False))[2:])[-4:])
			#print(' '.join(a+b for a,b in zip(s[::2], s[1::2])))
			print(f"{hex(entry['link']):4} <= {hex(entry['str']-4):4} {entry['strlen']:2}{readStr(self.VM,entry['str']):10} *[{hex(cwa):4}]={' '.join(strs)}")
			# Dump the dict entry in binary
			#s=binascii.hexlify(VM.memory[entry['str']-4:cwa + 2*entry["flag"]]).decode("utf-8")
			#print(' '.join(a+b for a,b in zip(s[::2], s[1::2])))
			prev, current = current, entry["link"]
