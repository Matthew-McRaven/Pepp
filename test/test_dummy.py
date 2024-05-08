import itertools
import logging
import unittest
import p4.vm, p4.utils, p4.dictionary
import p4.io.void
import p4.bootstrap
import p4.vocabs.boot, p4.vocabs.debug, p4.vocabs.boot.compile

class TestStringMethods(unittest.TestCase):
    def test_magic(self):
        words = [x for x in itertools.chain.from_iterable([p4.vocabs.boot.ALL, p4.vocabs.debug.ALL])]
        VM = p4.vm.vm()
        VM.io.output = p4.io.void.Output()
        VM.io.log.setLevel(logging.DEBUG)
        p4.bootstrap.initialize(VM, words)

        # Helper to look up a WORD and get its CWA, used to implement interpreted words for now.
        e = lambda s: p4.dictionary.addr_from_name(VM, s)
        f = lambda s: e(s)["cwa"]

        p4.dictionary.defforth(VM, ("doAll", 0, ["'", ".", "CR", "EXIT"]))
        p4.dictionary.defforth(VM, ("testReword", 0, "LIT 1 . HALT".split()))

        with open("p4/bootstrap/dummy.f") as x: VM.io.buffer_text(" ".join(x.readlines()))

        VM.tcb.nextWord(f("testReword")); VM.next()
        VM.run()
