from p4.bootstrap import prune_graph
import unittest

class TestGraphPrune(unittest.TestCase):
    def test_keep_none(self):
        graph = {'a': set('b'), 'b': set('c'), 'c': set()}
        pruned = prune_graph(graph, [])
        self.assertEqual(len(pruned), 0)

    def test_keep_tail(self):
        graph = {'a': set('b'), 'b': set('c'), 'c': set()}
        pruned = prune_graph(graph, ['c'])
        self.assertEqual(len(pruned), 1)
        self.assertTrue("c" in pruned)

    def test_keep_transitive(self):
        graph = {'a': set('b'), 'b': set('c'), 'c': set()}
        pruned = prune_graph(graph, ['a'])
        self.assertEqual(len(pruned), 3)
        self.assertTrue("a" in pruned)
        self.assertTrue("b" in pruned)
        self.assertTrue("c" in pruned)

    def test_keep_recurse_branch(self):
        graph = {'removed': {':', 'word', ';'}, 'word': {':', 'base', ';'},
                 'base': {}, ':': {}, ';': {}}
        pruned = prune_graph(graph, ['word'])
        self.assertEqual(len(pruned), 4)
        self.assertTrue("word" in pruned)
        self.assertTrue("base" in pruned)
        self.assertTrue(":" in pruned)
        self.assertTrue(";" in pruned)
        self.assertFalse("removed" in pruned)
