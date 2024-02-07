import unittest
import os

from fort_charis_options.source_parser import parse_source_file


class StructureParserTest(unittest.TestCase):
    EXAMPLE_PATH = os.path.join(os.path.dirname(os.path.dirname(__file__)), "examples")

    def test_can_parse_file(self):
        res = parse_source_file(os.path.join(self.EXAMPLE_PATH, "flat.hpp"))
        self.assertEqual(res, {})
