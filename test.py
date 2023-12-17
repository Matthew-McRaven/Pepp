from p4.words import extract, core, io, math, stack, var, dict as _dict
import p4.bootstrap, p4.vm, p4.dictionary

words = [fn for item in [core, stack, io, var, math, _dict] for fn in extract(item)]
VM = p4.vm.vm()
p4.bootstrap.bootstrap(VM, words)

# Helper to look up a WORD and get its CWA, used to implement interpretted words for now.
e = lambda s: p4.dictionary.addr_from_name(VM, s)
f = lambda s: e(s)["cwa"]

docol_token = VM.memory.read_b16(f("DOCOL"), False)

p4.dictionary.defforth(VM, ("doAll", 0, ["'", ".", "HALT"]))
VM.tcb.nextWord(f("doAll")); VM.next()
p4.dictionary.dump(VM)
VM.run()
