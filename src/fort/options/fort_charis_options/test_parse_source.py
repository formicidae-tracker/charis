import unittest
import os

from fort_charis_options.source_parser import CppField, CppSource, CppStruct


class StructureParserTest(unittest.TestCase):
    EXAMPLE_PATH = os.path.join(os.path.dirname(os.path.dirname(__file__)), "examples")

    def test_can_parse_file(self):
        res = CppSource.Parse(os.path.join(self.EXAMPLE_PATH, "flat.hpp"))
        self.assertEqual(
            res,
            CppStruct(
                Name="Options",
                Fields={
                    "AnInt": CppField(Name="AnInt", Type="int"),
                    "ADouble": CppField(Name="ADouble", Type="double"),
                    "AString": CppField(Name="AString", Type="std::string"),
                    "AVectorOfInt": CppField(
                        Name="AVectorOfInt", Type="std::vector<int>"
                    ),
                },
            ),
        )
