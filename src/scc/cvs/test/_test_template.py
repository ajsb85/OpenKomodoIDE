"""
    Test cvslib.py's interface to the 'cvs foo' command.
"""

import os, sys, unittest
import testsupport


class FooTestCase(unittest.TestCase):
    pass


def suite():
    """Return a unittest.TestSuite to be used by test.py."""
    return unittest.makeSuite(FooTestCase)

