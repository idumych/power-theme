#!/usr/bin/env python3
"""Draw the NeXT icon theme into data/icons/NeXT, for the NeXT theme.

    python3 -I tools/make-next-icons.py

Swizcore Studio's NeXT GUI kit (2004) comes with two icons: NeXTSTEP's folder, a
taupe leather wallet with a flap and a cord round it, and a computer, a black
MegaPixel-style display on a black base. They are drawn here from scratch as SVG,
after their shape and colours; none of the kit's pictures are used. Home, mail and
query folders are the same folder with a badge. Everything else is inherited from
NineIcons Redux.

Each icon is rendered with librsvg's rsvg-convert into PNG files at Power
Theme's sizes: <context>/<size>/<name>.png.
"""

import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
OUT = os.path.join(ROOT, "data", "icons", "NeXT")
SIZES = (16, 24, 32, 48, 64)

# the folder's colours, after the kit's
GUSSET = ("#4b3028", "#7b5f53")
FRONT = ("#7c685a", "#b39a82")
BACK = ("#cbbba5", "#e4dccb")
FLAP = ("#9a8673", "#ddcbad")
CORD = "#d7d4c6"
CORD_EDGE = "#6b675c"
KNOT = "#1a1a16"


def folder(badge=""):
	return f"""<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 128 128">
<defs>
 <linearGradient id="gusset" x1="0" y1="0" x2="1" y2="0">
  <stop offset="0" stop-color="{GUSSET[0]}"/><stop offset="1" stop-color="{GUSSET[1]}"/>
 </linearGradient>
 <linearGradient id="front" x1="0" y1="0" x2="1" y2="0.4">
  <stop offset="0" stop-color="{FRONT[0]}"/><stop offset="1" stop-color="{FRONT[1]}"/>
 </linearGradient>
 <linearGradient id="back" x1="0" y1="0" x2="1" y2="0">
  <stop offset="0" stop-color="{BACK[0]}"/><stop offset="1" stop-color="{BACK[1]}"/>
 </linearGradient>
 <linearGradient id="flap" x1="0" y1="1" x2="0.6" y2="0">
  <stop offset="0" stop-color="{FLAP[0]}"/><stop offset="1" stop-color="{FLAP[1]}"/>
 </linearGradient>
</defs>
<!-- shadow on the ground -->
<path d="M18 116 L104 88 L116 92 L32 122 Z" fill="#000" opacity="0.18"/>
<!-- the back panel, standing up behind the flap -->
<path d="M34 46 L108 20 Q112 19 112 23 L112 76 L34 104 Z" fill="url(#back)"
 stroke="#6e5c4c" stroke-width="1.2" stroke-linejoin="round"/>
<!-- the front panel -->
<path d="M24 58 L104 30 L104 86 L24 114 Z" fill="url(#front)"
 stroke="#4f3b30" stroke-width="1.2" stroke-linejoin="round"/>
<!-- the gusset at its left end -->
<path d="M18 60 Q18 56 22 55 L34 50 L34 104 L24 114 Q18 116 18 110 Z"
 fill="url(#gusset)" stroke="#32201a" stroke-width="1.2" stroke-linejoin="round"/>
<path d="M22 60 L22 110" stroke="#9a8070" stroke-width="1.5" opacity="0.7"/>
<!-- the flap, over from the back, curling up at its right end -->
<path d="M34 46 L108 20 Q116 18 114 26 L112 34 Q120 42 114 50
 L104 54 L28 82 Q22 84 22 78 L22 62 Q22 56 28 54 Z"
 fill="url(#flap)" stroke="#5a4636" stroke-width="1.2" stroke-linejoin="round"/>
<path d="M28 80 L104 53" stroke="#5a4636" stroke-width="1.6" opacity="0.6"/>
<path d="M36 50 L108 24" stroke="#f0e8d8" stroke-width="1.4" opacity="0.8"/>
<!-- the cord round it, and its knot at the flap's edge -->
<path d="M66 41 L66 28 Q66 24 70 23 Q74 22 74 26 L74 92" fill="none"
 stroke="{CORD_EDGE}" stroke-width="4" stroke-linecap="round"/>
<path d="M66 41 L66 28 Q66 24 70 23 Q74 22 74 26 L74 92" fill="none"
 stroke="{CORD}" stroke-width="2.2" stroke-linecap="round"/>
<ellipse cx="72" cy="69" rx="4.5" ry="3.5" fill="{KNOT}"/>
<ellipse cx="71" cy="68" rx="1.6" ry="1.1" fill="#8a8678"/>
{badge}
</svg>"""


def badge(content, scale):
	"""A round white badge with a black ring in the bottom right corner, larger
	at small sizes so that it can still be made out."""
	x = 128 - 26 * scale
	y = 128 - 26 * scale
	return f"""<g transform="translate({x} {y}) scale({scale})">
<circle r="22" fill="#fff" stroke="#000" stroke-width="3"/>{content}</g>"""


HOME = ("""<path d="M-13 1 L0 -12 L13 1 L9 1 L9 12 L-9 12 L-9 1 Z" fill="#000"/>
<rect x="-3" y="4" width="6" height="8" fill="#fff"/>""")
MAIL = ("""<rect x="-14" y="-9" width="28" height="19" fill="#fff" stroke="#000"
 stroke-width="3" stroke-linejoin="round"/>
<path d="M-14 -9 L0 3 L14 -9" fill="none" stroke="#000" stroke-width="3"
 stroke-linejoin="round"/>""")
QUERY = ("""<circle cx="-3" cy="-3" r="8" fill="none" stroke="#000" stroke-width="4"/>
<path d="M3 3 L12 12" stroke="#000" stroke-width="5" stroke-linecap="round"/>""")


COMPUTER = """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 128 128">
<defs>
 <linearGradient id="bezel" x1="0" y1="0" x2="1" y2="1">
  <stop offset="0" stop-color="#5a5752"/><stop offset="0.25" stop-color="#2d2b27"/>
  <stop offset="1" stop-color="#1b1a15"/>
 </linearGradient>
 <linearGradient id="screen" x1="0" y1="0" x2="1" y2="1">
  <stop offset="0" stop-color="#23261d"/><stop offset="0.4" stop-color="#0a0c06"/>
  <stop offset="1" stop-color="#090b05"/>
 </linearGradient>
 <linearGradient id="base" x1="0" y1="0" x2="0" y2="1">
  <stop offset="0" stop-color="#3b3a35"/><stop offset="1" stop-color="#111210"/>
 </linearGradient>
 <linearGradient id="top" x1="0" y1="0" x2="1" y2="0">
  <stop offset="0" stop-color="#7b7b75"/><stop offset="1" stop-color="#2c2a27"/>
 </linearGradient>
</defs>
<ellipse cx="64" cy="121" rx="46" ry="4" fill="#000" opacity="0.2"/>
<!-- the base, its lit top and its front with a button -->
<path d="M26 98 L102 98 L108 104 L20 104 Z" fill="url(#top)"/>
<rect x="20" y="104" width="88" height="14" rx="1.5" fill="url(#base)"
 stroke="#000" stroke-width="1"/>
<path d="M22 110 L106 110" stroke="#000" stroke-width="1" opacity="0.6"/>
<circle cx="64" cy="113" r="1.8" fill="#c9a24a"/>
<!-- the neck -->
<path d="M50 80 L78 80 L80 98 L48 98 Z" fill="#1d1d18"/>
<path d="M50 80 L54 98" stroke="#3b3833" stroke-width="2"/>
<!-- the display -->
<rect x="20" y="10" width="88" height="74" rx="4" fill="url(#bezel)"
 stroke="#000" stroke-width="1"/>
<rect x="27" y="16" width="74" height="58" rx="1.5" fill="url(#screen)"
 stroke="#000" stroke-width="1"/>
<path d="M28 17 L60 17 L28 40 Z" fill="#fff" opacity="0.06"/>
<circle cx="64" cy="79" r="1.6" fill="#c9a24a"/>
<circle cx="34" cy="79" r="1" fill="#6b6a62"/>
</svg>"""


def badge_scale(size):
	return 1.45 if size <= 16 else 1.25 if size <= 24 else 1


ICONS = {
	("places", "folder"): lambda size: folder(),
	("places", "user-home"): lambda size: folder(badge(HOME, badge_scale(size))),
	("places", "folder-mail"): lambda size: folder(badge(MAIL, badge_scale(size))),
	("places", "folder-saved-search"):
		lambda size: folder(badge(QUERY, badge_scale(size))),
	("devices", "computer"): lambda size: COMPUTER,
}

CONTEXTS = {"places": "Places", "devices": "Devices"}


def main():
	if shutil.which("rsvg-convert") is None:
		sys.exit("make-next-icons.py: rsvg-convert (librsvg) is needed")

	shutil.rmtree(OUT, ignore_errors=True)
	directories = []
	with tempfile.TemporaryDirectory() as work:
		for (context, name), draw in ICONS.items():
			for size in SIZES:
				source = os.path.join(work, f"{name}-{size}.svg")
				with open(source, "w") as f:
					f.write(draw(size))
				folder_name = f"{context}/{size}"
				if folder_name not in directories:
					directories.append(folder_name)
				os.makedirs(os.path.join(OUT, folder_name), exist_ok=True)
				subprocess.run(["rsvg-convert", "-w", str(size), "-h", str(size),
					"-o", os.path.join(OUT, folder_name, name + ".png"), source],
					check=True)

	directories.sort(key=lambda d: (d.split("/")[0], int(d.split("/")[1])))
	with open(os.path.join(OUT, "index.theme"), "w") as f:
		f.write("[Icon Theme]\n")
		f.write("Name=NeXT\n")
		f.write("Comment=NeXTSTEP's leather folder and a black display, after "
			"Swizcore Studio's NeXT; the rest from NineIcons Redux\n")
		f.write("Inherits=NineIconsRedux\n")
		f.write("Directories=" + ",".join(directories) + "\n")
		for directory in directories:
			context, size = directory.split("/")
			f.write(f"\n[{directory}]\nContext={CONTEXTS[context]}\n"
				f"Size={size}\nType=Fixed\n")


if __name__ == "__main__":
	main()
