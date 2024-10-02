import re
import subprocess
from dataclasses import dataclass
from typing import Dict, Optional

from clang import cindex
from jinja2.defaults import DEFAULT_NAMESPACE


@dataclass
class CppField:
    Name: str
    Type: str
    Short: Optional[str]
    Long: str
    Description: str = ""
    Required: bool = False


@dataclass
class CppStruct:
    Name: str
    Fields: Dict[str, CppField]


tagRx = re.compile(r"""([a-zA-Z_]+):"(.*)"$""")


def _parseTags(tags: str):
    res = {}
    if tags is None:
        return res
    for t in tags.strip().split(" "):
        match = tagRx.match(t)
        if match is None:
            continue
        res[match[1]] = match[2]
    return res


class CppSource:
    _BaseArgs = []

    @staticmethod
    def BaseArgs():
        if len(CppSource._BaseArgs) > 0:
            return CppSource._BaseArgs
        CppSource._BaseArgs = "-std=c++17 -Wno-pragma-once-outside-header".split()
        out = subprocess.check_output(
            "c++ -E -x c++ - -v".split(),
            stdin=subprocess.DEVNULL,
            stderr=subprocess.STDOUT,
        )

        CppSource._BaseArgs += [
            "-I" + d.group(0)[1:]
            for d in re.finditer("^ /.*$", out.decode("utf-8"), re.MULTILINE)
        ]

        return CppSource._BaseArgs

    @staticmethod
    def Parse(filename):
        source = CppSource(filename)
        return source.parseDeclaration()

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
            self.declarations[cursor.get_usr()] = cursor

    def parseDeclaration(self) -> Dict[str, CppStruct]:
        res = {}
        for name, cursor in self.declarations.items():
            res[name] = CppStruct(Name=cursor.displayname, Fields={})
            for child in cursor.get_children():
                if child.kind != cindex.CursorKind.FIELD_DECL:
                    continue

                tags = _parseTags(child.brief_comment)

                res[name].Fields[child.displayname] = CppField(
                    Name=child.displayname,
                    Type=child.type.spelling,
                    Short=tags.get("short", None),
                    Long=tags.get("long", ""),
                    Description=tags.get("description", ""),
                    Required=tags.get("required", "false") == "true",
                )

        return res
