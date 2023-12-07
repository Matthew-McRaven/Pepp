import p4.utils
from p4.words import extract, core, io, math, stack, var
import p4.bootstrap, p4.vm, p4.dictionary

from p4.vm.dictionary import DictAccessor
words = [fn for item in [core, stack, io, var, math] for fn in extract(item)]
VM = p4.vm.vm()
p4.bootstrap.bootstrap(VM, words)
p4.dictionary.dump(VM)

# Helper to look up a WORD and get its CWA, used to implement interpretted words for now.
f = lambda s: p4.dictionary.addr_from_name(VM, s)["cwa"]
tokens = [f("LIT"), 0xFEED, f("."), f("HALT")]
VM.intWord("doAll", tokens)
VM.tcb.nextWord = f("doAll"); VM.next()
VM.run()
