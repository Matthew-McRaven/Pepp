import p4.utils
from p4.words import extract, core, io, math, stack, var, dict
import p4.bootstrap, p4.vm, p4.dictionary

words = [fn for item in [core, stack, io, var, math, dict] for fn in extract(item)]
VM = p4.vm.vm()
p4.bootstrap.bootstrap(VM, words)
p4.dictionary.dump(VM)

# Helper to look up a WORD and get its CWA, used to implement interpretted words for now.
e = lambda s: p4.dictionary.addr_from_name(VM, s)
f = lambda s: e(s)["cwa"]
#tokens = [f("LIT"), e(".")["head"], f("NAME"), f("FIND"), f("."), f("HALT")]
tokens = [f("KEY"), f("KEY"), f("EMIT"), f("EMIT"), f("."), f("HALT")]
VM.intWord("doAll", tokens)
VM.tcb.nextWord = f("doAll"); VM.next()
VM.run()
