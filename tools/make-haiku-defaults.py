#!/usr/bin/env python3
"""Write src/themes/HaikuDefaults.h: Haiku's default system colours, light
and dark, from a Haiku or Renku source tree.

    python3 -I tools/make-haiku-defaults.py <source tree> > src/themes/HaikuDefaults.h
"""

import os
import re
import sys


def table(text, name):
    start = text.index("static const rgb_color %s[kColorWhichCount]" % name)
    block = text[start:text.index("};", start)]
    return re.findall(r"\{(\d+), (\d+), (\d+), \d+\},\s*// (B_[A-Z_]+)", block)


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__.strip())
    text = open(os.path.join(sys.argv[1],
        "src/kits/interface/InterfaceDefs.cpp")).read()
    print("/*\n * Haiku's default system colours, from its source\n"
        " * (src/kits/interface/InterfaceDefs.cpp), written by\n"
        " * tools/make-haiku-defaults.py. Haiku, Inc.: MIT licence.\n */")
    print("#ifndef HAIKU_DEFAULTS_H\n#define HAIKU_DEFAULTS_H\n")
    print("#include <InterfaceDefs.h>\n")
    print("struct DefaultColor {\n\tcolor_which\twhich;\n\trgb_color\tcolor;\n};\n")
    for array, name in (("kHaikuLightColors", "_kDefaultColors"),
            ("kHaikuDarkColors", "_kDefaultColorsDark")):
        print("static const DefaultColor %s[] = {" % array)
        for r, g, b, which in table(text, name):
            if which == "B_TOOLTIP_BACKGROUND_COLOR":
                which = "B_TOOL_TIP_BACKGROUND_COLOR"
            elif which == "B_TOOLTIP_TEXT_COLOR":
                which = "B_TOOL_TIP_TEXT_COLOR"
            print("\t{ %s, { %s, %s, %s, 255 } }," % (which, r, g, b))
        print("};\n")
    print("static const int32 kHaikuDefaultColorCount\n"
        "\t= sizeof(kHaikuLightColors) / sizeof(kHaikuLightColors[0]);\n")
    print("#endif\t// HAIKU_DEFAULTS_H")


if __name__ == "__main__":
    main()
