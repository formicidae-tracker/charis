import unittest
import os

from fort_charis_options.source_parser import CppField, CppSource, CppStruct


class StructureParserTest(unittest.TestCase):
    EXAMPLE_PATH = os.path.join(
        os.path.dirname(os.path.dirname(os.path.dirname(__file__))), "examples"
    )

    def test_can_parse_flat_struct(self):
        self.maxDiff = None
        res = CppSource.Parse(os.path.join(self.EXAMPLE_PATH, "flat.hpp"))
        self.assertDictEqual(
            res,
            {
                "c:@S@Options": CppStruct(
                    Name="Options",
                    Fields={
                        "AnInt": CppField(
                            Name="AnInt",
                            Type="int",
                            Short="I",
                            Long="integer",
                            Description="An Integer",
                            Required=False,
                        ),
                        "ADouble": CppField(
                            Name="ADouble",
                            Type="double",
                            Short="d",
                            Long="double",
                            Description="A double",
                            Required=True,
                        ),
                        "AString": CppField(
                            Name="AString",
                            Type="std::string",
                            Short=None,
                            Long="string",
                            Description="A String",
                            Required=False,
                        ),
                        "AVectorOfInt": CppField(
                            Name="AVectorOfInt",
                            Type="std::vector<int>",
                            Short="R",
                            Long="repeat",
                            Description="a repeatable",
                            Required=False,
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
