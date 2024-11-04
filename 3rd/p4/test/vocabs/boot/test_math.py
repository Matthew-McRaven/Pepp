import unittest
import test.utils

from test.parameterized import parameterized
import p4.vocabs.boot, p4.dictionary

class TestMath(unittest.TestCase):
    @parameterized.expand([
        [">0", 0, 0, ">0, equals 0"],
        [">0", 1, 1, ">0, greater than 0"],
        [">0", 0xFFFF, 0, ">0, less than 0"],
    ])
    def test_unary_op(self, op, first, result, message):
        words = p4.vocabs.boot.ALL
        vm = test.utils.create_vm(words, [op])
        test.utils.pushall_b16(vm,[first])
        test.utils.step(vm, op)
        self.assertEqual(vm.pStack.pop_b16(signed=False), result, message)

    @parameterized.expand([
        ["+", 0, 0, 0, "0 is the identity, ensure we don't hallucinate values"],
        ["+", 10, 20, 30, "unsigned addition, no overflow"],
        ["+", 0xffff, 11, 10, "addition, signed+unsigned overflow"],
        ["+", 0x7fff, 1, 0x8000, "addition, signed overflow"],
        ["-",0, 0, 0, "0 is the identity, ensure we don't hallucinate values"],
        ["-", 30, 10, 20, "unsigned sub, no overflow"],
        ["-",0x8000, 1, 0x7FFF, "sub, signed overflow"],
        ["OR", 30, 0, 30, "OR, identity"],
        ["OR", 0xFFFF, 0, 0xFFFF, "OR"],
        ["AND", 30, 0xFFFF, 30, "AND, identity"],
        ["AND", 0xFFFF, 0, 0, "AND, zero"],
        ["=", 30, 30, 1, "=, equal"],
        ["=", 0xFFFF, 0, 0, "=, not equal"],
    ])
    def test_binary_op(self, op, first, second, result, message):
        words = p4.vocabs.boot.ALL
        vm = test.utils.create_vm(words, [op])
        test.utils.pushall_b16(vm,[first, second])
        test.utils.step(vm, op)
        self.assertEqual(vm.pStack.pop_b16(signed=False), result, message)