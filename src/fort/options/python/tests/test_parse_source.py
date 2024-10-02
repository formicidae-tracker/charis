import unittest
import os

from fort_charis_options.source_parser import CppField, CppSource, CppStruct


class StructureParserTest(unittest.TestCase):
    EXAMPLE_PATH = os.path.join(os.path.dirname(os.path.dirname(__file__)), "examples")

    def test_can_parse_flat_struct(self):
        self.maxDiff = None
        res = CppSource.Parse(os.path.join(self.EXAMPLE_PATH, "flat.hpp"))
        self.assertDictEqual(
            res,
            {
                "c:@S@Options": CppStruct(
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
            },
        )

    def test_can_parse_nested_struct(self):
        self.maxDiff = None
        res = CppSource.Parse(os.path.join(self.EXAMPLE_PATH, "nested.hpp"))
        self.assertDictEqual(
            res,
            {
                "c:@S@Resolution": CppStruct(
                    Name="Resolution",
                    Fields={
                        "Width": CppField(Name="Width", Type="int"),
                        "Height": CppField(Name="Height", Type="int"),
                    },
                ),
                "c:@S@Options": CppStruct(
                    Name="Options",
                    Fields={
                        "Resolution": CppField(Name="Resolution", Type="Resolution"),
                    },
                ),
            },
        )
