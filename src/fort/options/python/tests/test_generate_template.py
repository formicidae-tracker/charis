import unittest
import os

from . import generate_template

from .source_parser import CppSource


class GenerateTemplateTest(unittest.TestCase):
    EXAMPLE_PATH = os.path.join(os.path.dirname(os.path.dirname(__file__)), "examples")

    def setUp(self):
        self.definitions = CppSource.Parse(
            os.path.join(self.EXAMPLE_PATH, "nested.hpp")
        )

    def test_fail(self):
        with self.assertRaisesRegex(RuntimeError, "could not found 'foo'"):
            generate_template(self.definitions, "foo")

    def test_work(self):
        self.assertEqual(
            generate_template(self.definitions, "Options"),
            """// this file was automatically generated with fort_charis_options, do not modify it
#include <fort/charis/Options.hpp>

fort::charis::options::OptionsParser CreateOptionsParser(Options & opts) {

}
""",
        )
