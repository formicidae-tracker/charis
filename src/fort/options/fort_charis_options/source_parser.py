import re
from typing import Dict
from clang import cindex
from dataclasses import dataclass

import ccsyspath


@dataclass
class CppField:
    Name: str
    Type: str


@dataclass
class CppStruct:
    Name: str
    Fields: Dict[str, CppField]


class CppSource:
    _BaseArgs = []

    @staticmethod
    def BaseArgs():
        if len(CppSource._BaseArgs) > 0:
            return CppSource._BaseArgs
        CppSource._BaseArgs = (
            "-x c++ -std=c++11 -Wno-pragma-once-outside-header".split()
        )
        CppSource._BaseArgs += [
            "-I" + d.decode("utf-8") for d in ccsyspath.system_include_paths("clang++")
        ]
        return CppSource._BaseArgs

    @staticmethod
    def Parse(filename, name="Options"):
        source = CppSource(filename)
        return source.parseDeclaration(name)

    def __init__(self, filename):
        self.enumerate_declaration(filename)

    def enumerate_declaration(self, filename):
        index = cindex.Index.create()
        self.declarations = {}
        options = cindex.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD

        tu = index.parse(filename, args=self.BaseArgs(), options=options)
        for d in tu.diagnostics:

            if d.severity > d.Warning:
                raise RuntimeError(f"Could not parse '{filename}': {d}")
            else:
                print(d)

        for cursor in tu.cursor.get_children():
            if cursor.kind != cindex.CursorKind.STRUCT_DECL or (
                cursor.location.file and cursor.location.file.name != filename
            ):
                continue
            self.declarations[cursor.displayname] = cursor

    def parseDeclaration(self, name: str):
        ref = self.declarations[name]
        if ref is None:
            raise RuntimeError(f"Could not find structure '{name}'")

        res = CppStruct(Name=name, Fields={})
        for cursor in ref.get_children():
            if cursor.kind != cindex.CursorKind.FIELD_DECL:
                continue
            res.Fields[cursor.displayname] = CppField(
                Name=cursor.displayname,
                Type=cursor.type.spelling,
            )

        return res
