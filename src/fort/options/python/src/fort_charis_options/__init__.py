import argparse
from typing import Dict

from .source_parser import CppSource, CppStruct


def parse_options():
    parser = argparse.ArgumentParser(
        prog="fort_charis_options",
        description="Parse header file to extract structure and build parser around fort-charis::options",
    )

    parser.add_argument("filename")
    parser.add_argument("-C", "--classname", type=str, default="Options")

    return parser.parse_args()


def get_usr(structname: str) -> str:
    return f"c:@S@{structname}"


def generate_template(definitions: Dict[str, CppStruct], structname: str) -> str:
    if get_usr(structname) not in definitions:
        raise RuntimeError(f"could not found '{structname}'")
    raise NotImplementedError()


def main():
    opts = parse_options()
    definitions = CppSource.Parse(opts.filename)
    with open(f"{opts.classname}Parser.hpp") as f:
        f.write(generate_template(definitions, opts.classname))
