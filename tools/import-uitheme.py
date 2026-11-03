#!/usr/bin/env python3
"""Convert a Theme Manager theme (HaikuThemeManager's "UIThemes" folders:
a "Theme" file holding a BMessage in text form, and a screenshot) into a
Themes theme folder with theme.settings and preview.png.

    python3 -I tools/import-uitheme.py <Theme file> <output folder>
        [--name=NAME] [--description=TEXT] [--control-look=NAME]
        [--wallpaper=IMAGE] [--wallpaper-mode=scaled|centered|tiled|manual]
        [--icons=ID|haiku] [--credit=TEXT]
        [--haiku-source=<Haiku or Renku source tree>]

Taken over: the system colours, the desktop colour (of the first
workspace), the fonts, the window decorator and the Deskbar's place. Fonts other than Noto Sans
get Noto Sans as an alternative, for systems without them. Theme Manager
files don't name a control look, and seldom the wallpaper image, so those
can be given. With --haiku-source, the colours the theme leaves out are
filled in from Haiku's defaults (src/kits/interface/InterfaceDefs.cpp):
the dark set for a dark theme, the light one otherwise. Terminal, screensaver and sound settings are left out: Themes
doesn't change those.
"""

import argparse
import os
import re
import shutil
import sys

# Theme Manager's colour names -> the names of Haiku's colour settings
COLORS = {
    "PanBg": "B_PANEL_BACKGROUND_COLOR", "PanTx": "B_PANEL_TEXT_COLOR",
    "DocBg": "B_DOCUMENT_BACKGROUND_COLOR", "DocTx": "B_DOCUMENT_TEXT_COLOR",
    "CtlBg": "B_CONTROL_BACKGROUND_COLOR", "CtlTx": "B_CONTROL_TEXT_COLOR",
    "CtlBr": "B_CONTROL_BORDER_COLOR", "CtlHg": "B_CONTROL_HIGHLIGHT_COLOR",
    "CtlMk": "B_CONTROL_MARK_COLOR",
    "NavBs": "B_KEYBOARD_NAVIGATION_COLOR", "NavPl": "B_NAVIGATION_PULSE_COLOR",
    "Shine": "B_SHINE_COLOR", "Shadow": "B_SHADOW_COLOR",
    "TipBg": "B_TOOLTIP_BACKGROUND_COLOR", "TipTx": "B_TOOLTIP_TEXT_COLOR",
    "MenBg": "B_MENU_BACKGROUND_COLOR",
    "MenSBg": "B_MENU_SELECTED_BACKGROUND_COLOR",
    "MenTx": "B_MENU_ITEM_TEXT_COLOR",
    "MenSTx": "B_MENU_SELECTED_ITEM_TEXT_COLOR",
    "MenSBr": "B_MENU_SELECTED_BORDER_COLOR",
    "Success": "B_SUCCESS_COLOR", "Failure": "B_FAILURE_COLOR",
    "WinTabBg": "B_WINDOW_TAB_COLOR", "WinTabTx": "B_WINDOW_TEXT_COLOR",
    "InWinTabBg": "B_WINDOW_INACTIVE_TAB_COLOR",
    "InWinTabTx": "B_WINDOW_INACTIVE_TEXT_COLOR",
    "WinBr": "B_WINDOW_BORDER_COLOR", "InWinBr": "B_WINDOW_INACTIVE_BORDER_COLOR",
    "LstBg": "B_LIST_BACKGROUND_COLOR", "LstSBg": "B_LIST_SELECTED_BACKGROUND_COLOR",
    "LstTx": "B_LIST_ITEM_TEXT_COLOR", "LstSTx": "B_LIST_SELECTED_ITEM_TEXT_COLOR",
    "SbThb": "B_SCROLL_BAR_THUMB_COLOR",
    "LnkTx": "B_LINK_TEXT_COLOR", "LnkHv": "B_LINK_HOVER_COLOR",
    "LnkVs": "B_LINK_VISITED_COLOR", "LnkAc": "B_LINK_ACTIVE_COLOR",
    "StaBar": "B_STATUS_BAR_COLOR",
}

FONTS = {
    "be:f:be_plain_font": "plain", "be:f:be_bold_font": "bold",
    "be:f:be_fixed_font": "fixed", "be:f:MenTx": "menu",
}

COLOR = re.compile(r"^\s*(\S+?)(?:\[\d+\])? = rgb_color\((\d+),(\d+),(\d+),\d+\)")
FONT = re.compile(r"^\s*(\S+) = BFont\(([^/]+)/([^/]+)/([\d.]+)")
INT = re.compile(r"^\s*(\S+) = int32\((-?\d+)")
BOOL = re.compile(r"^\s*(\S+) = bool\((true|false)\)")
PLACES = ["top", "bottom", "left top", "right top", "left bottom",
    "right bottom"]
STRING = re.compile(r'^\s*(\S+) = string\("([^"]*)"')


def haiku_defaults(source, dark):
    """Haiku's default colours, from its source."""
    text = open(os.path.join(source, "src/kits/interface/InterfaceDefs.cpp")).read()
    start = text.index("static const rgb_color _kDefaultColors%s[kColorWhichCount]"
        % ("Dark" if dark else ""))
    block = text[start:text.index("};", start)]
    return [(name, "#%02X%02X%02X" % (int(r), int(g), int(b)))
        for r, g, b, name in re.findall(
            r"\{(\d+), (\d+), (\d+), \d+\},\s*// (B_[A-Z_]+)", block)]


def font_line(which, family, style, size):
    fallback = "Noto Sans Mono" if which == "fixed" else "Noto Sans"
    if family != fallback:
        family += " | " + fallback
    if style == "Medium":
        style += " | Bold"
    return "font %s = %s, %s, %g" % (which, family, style, float(size))


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("theme")
    parser.add_argument("output")
    parser.add_argument("--name")
    parser.add_argument("--description", default="")
    parser.add_argument("--control-look")
    parser.add_argument("--wallpaper")
    parser.add_argument("--wallpaper-mode", default="scaled")
    parser.add_argument("--icons")
    parser.add_argument("--credit", default="")
    parser.add_argument("--haiku-source")
    args = parser.parse_args()

    name = args.name
    desktop = None
    decorator = None
    wallpaper = None
    colors = {}
    fonts = {}
    deskbar = None
    expanded = True
    with open(args.theme) as f:
        for line in f:
            match = COLOR.match(line)
            if match:
                key, r, g, b = match.groups()
                short = key.split(":")[-1]
                hexcolor = "#%02X%02X%02X" % (int(r), int(g), int(b))
                if key == "be:desktop_color":
                    desktop = desktop or hexcolor
                elif short in COLORS and COLORS[short] not in colors:
                    colors[COLORS[short]] = hexcolor
                continue
            match = FONT.match(line)
            if match and match.group(1) in FONTS:
                fonts[FONTS[match.group(1)]] = match.groups()[1:]
                continue
            match = INT.match(line)
            if match and match.group(1) == "db:location":
                if 0 <= int(match.group(2)) < len(PLACES):
                    deskbar = PLACES[int(match.group(2))]
                continue
            match = BOOL.match(line)
            if match and match.group(1) == "db:expanded":
                expanded = match.group(2) == "true"
                continue
            match = STRING.match(line)
            if match:
                key, value = match.groups()
                if key == "z:theme:name" and not name:
                    name = value
                elif key == "window:decor" and value:
                    decorator = os.path.basename(value)
                elif key == "be:bgndimginfopath" and value:
                    wallpaper = value

    if not name:
        sys.exit("%s: no theme name" % args.theme)

    os.makedirs(args.output, exist_ok=True)
    defaults = []
    note = ""
    if args.haiku_source:
        panel = colors.get("B_PANEL_BACKGROUND_COLOR", "#D8D8D8")
        dark = sum(int(panel[i:i + 2], 16) for i in (1, 3, 5)) / 3 < 128
        defaults = haiku_defaults(args.haiku_source, dark)
        note = " Colours it leaves out are Haiku's %s defaults." % (
            "dark" if dark else "light")
    lines = ["# %s, converted from a Theme Manager theme by" % name,
        "# tools/import-uitheme.py." + note]
    if args.credit:
        lines.append("# " + args.credit)
    lines += ["", "name = " + name]
    if args.description:
        lines.append("description = " + args.description)
    if decorator:
        lines.append("decorator = " + decorator)
    if args.control_look:
        lines.append("control look = " + args.control_look)
    if args.icons:
        lines.append("icons = " + args.icons)
    if desktop:
        lines.append("desktop color = " + desktop)

    if deskbar:
        lines.append("deskbar = %s, %s"
            % (deskbar, "expanded" if expanded else "collapsed"))

    image = args.wallpaper or wallpaper
    if image and os.path.isfile(image):
        leaf = "wallpaper" + os.path.splitext(image)[1]
        shutil.copy(image, os.path.join(args.output, leaf))
        lines += ["wallpaper = " + leaf, "wallpaper mode = " + args.wallpaper_mode]
    else:
        lines.append("wallpaper = none")

    lines.append("")
    for which in ("plain", "bold", "fixed", "menu"):
        if which in fonts:
            lines.append(font_line(which, *fonts[which]))
    lines.append("")
    if defaults:
        for key, value in defaults:
            if key == "B_DESKTOP_COLOR" and desktop:
                value = desktop
            lines.append("color %s = %s" % (key, colors.get(key, value)))
    else:
        for key in sorted(colors):
            lines.append("color %s = %s" % (key, colors[key]))

    with open(os.path.join(args.output, "theme.settings"), "w") as f:
        f.write("\n".join(lines) + "\n")

    screenshot = os.path.join(os.path.dirname(args.theme), "screenshot.png")
    if os.path.isfile(screenshot):
        shutil.copy(screenshot, os.path.join(args.output, "preview.png"))
    print("%s: %d colours, %d fonts" % (args.output, len(colors), len(fonts)))


if __name__ == "__main__":
    main()
