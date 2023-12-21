import itertools
import unittest

loader = unittest.TestLoader()
tests = loader.discover("test", "test*.py", top_level_dir=".")
testRunner = unittest.runner.TextTestRunner()
testRunner.run(tests)