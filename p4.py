@NEXT	
def wd_tail(VM):
	VM.pStack.push_b16(VM.latest, 2, signed=False)
					
def bootstrap(VM):
	VM.pStack.push_bytes([6, 7])
	(cwa_printstr, _) = VM.nativeWord("prntstr", printstr)
	(cwa_latest, _) = VM.nativeWord("wd.tail", wd_tail)
	
	cwa_wdelink = VM.intWord("wde.link", [token_docol, cwa_q, cwa_dot, cwa_exit])
	cwa_wdename = VM.intWord("wde.name", [token_docol, cwa_plus3, cwa_printstr, cwa_exit])
	# Need to know code len first.
	#VM.intWord("wde.code", [""])
	#VM.intWord("wde.dump", ["DUP", "wde.link", "DUP", "wde.name"])
	VM.pStack.push_bytes([0x04, 0x00])
	#tokens = [cwa_latest, cwa_dup, cwa_wdelink, cwa_dup, cwa_wdename, cwa_halt]

	token_exc = VM.intWord("doAll", tokens)#token_q, token_dot, token_printstr, token_halt])
	ac = DictAccessor(VM)
	ac.dump()
	
	VM.tcb.nextWord = VM.addr_from_name("doAll")["cwa"]; VM.next()
	VM.run()
	
VM = vm()
bootstrap(VM)
