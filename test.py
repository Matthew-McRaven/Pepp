import itertools
import p4.bootstrap, p4.vm, p4.dictionary
from p4.io import open_file
import p4.vocabs.boot, p4.vocabs.debug

words = [x for x in itertools.chain.from_iterable([p4.vocabs.boot.ALL, p4.vocabs.debug.ALL])]

VM = p4.vm.vm()
p4.bootstrap.bootstrap(VM, words)

# Helper to look up a WORD and get its CWA, used to implement interpretted words for now.
e = lambda s: p4.dictionary.addr_from_name(VM, s)
f = lambda s: e(s)["cwa"]

p4.dictionary.defforth(VM, ("doAll", 0, ["'", ".", "CR", "EXIT"]))
p4.dictionary.defforth(VM, ("testReword", 0, "LIT 1 COREINT BRANCH -4".split()))

with open("p4/bootstrap/dummy.f") as x: open_file(" ".join(x.readlines()))

VM.tcb.nextWord(f("testReword")); VM.next()
VM.run()
