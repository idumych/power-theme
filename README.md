# Power Theme

![Power Theme on Haiku](docs/screenshots/power-theme.png)

**Power Theme** is a theme manager for [Renku OS](https://github.com/RenkuOS/Source)
and Haiku. It switches the whole look of the desktop between themes, and
back: window borders, controls, colours, fonts, wallpaper, the Deskbar's
place and freedesktop icon themes. It reads the themes made for Theme
Manager (and BeOS and ZETA before it) too.

It comes with its own looks:

- **Reactionary ME**, a Windows ME style look after the KDE Plasma theme
  of that name by Die4Ever ([store.kde.org/p/2331482](https://store.kde.org/p/2331482)),
  with the [ClassicOS 2000](https://github.com/ClassicOS-Themes/ClassicOS-2000-Icons)
  icons;
- **Smoke** window borders and controls in the style of ZETA's;
- **Renku**, in the colours of the Renku OS website;
- and the Haiku, Gonx and four Flat themes.

The Reactionary ME look is an independent, unofficial port. It is not
made by, affiliated with or endorsed by Die4Ever, the developers of
Reactionary, Reactionary Plus or the themes they build on, or the
ClassicOS project. It is a new implementation drawn to match the KDE
theme. Colours and the title bar button glyphs come from the theme's
files; the code shares nothing with them.

## Screenshots

All on Renku OS, with a window of controls and a Tracker window (and, for
Reactionary ME, a folder of different file types, to show its icons). The
picture at the top is Power Theme itself, under the stock Haiku theme.

| | |
|---|---|
| ![Reactionary ME](docs/screenshots/reactionary-me.png) | ![Renku](docs/screenshots/renku.png) |
| **Reactionary ME**: Windows ME window borders and controls, and the ClassicOS 2000 icons on folders and file types | **Renku**: Smoke window borders and flat controls in the Renku OS website's colours |
| ![Smoke](docs/screenshots/smoke.png) | ![Gonx](docs/screenshots/gonx.png) |
| **Smoke** window borders and controls, in the style of ZETA's, with the colours of the "smoke" Theme Manager theme | **Gonx**: Gonx's window borders and controls |
| ![DarkFlat](docs/screenshots/darkflat.png) | ![The Power Theme window](docs/screenshots/themes-window.png) |
| **DarkFlat**, from haiku_darkstyle, with Haiku's Flat window borders and controls | **Power Theme** under Renku: the theme list with a preview, the icon theme, and the parts to apply |

![Build add-ons](docs/screenshots/build-add-ons.png)

**Build add-ons…** compiles the window borders and controls for the running
system, here with the bundled headers, which match this Renku build.

## Install

Power Theme comes as a package for Haiku R1/beta6 and later, and Renku OS
(x86_64), on the [releases page](https://github.com/idumych/power-theme/releases):

```sh
pkgman install ./powertheme-0.3-1-x86_64.hpkg
```

Then open **Power Theme** (Deskbar → Applications), click **Build
add-ons…** once and apply a theme. The window borders and controls are
built on your own system, because they use private parts of app_server
and libbe; build them again after a system update. Power Theme offers to
install the compiler if it is missing.

On systems whose running applications can follow a theme, new controls and
fonts appear straight away, although an application that keeps fonts or sizes of
its own needs a restart. On others, including Haiku R1/beta6, restart running
applications or log out and back in to see them; Power Theme tells you which
applies.

```sh
powertheme --revert           # your previous decorator, controls, colours, fonts and icons
pkgman uninstall powertheme   # then remove Power Theme
```

If the desktop ever fails to start (for example after a system update),
hold Space while booting, open **Select safe mode options** and choose
**Disable user add-ons**. Once you're on the desktop, open Power Theme and
click **Build add-ons…** again.

## What's here

- **Window decorator** (`add-ons/decorators/Reactionary`), from the
  theme's "Windows ME" Aurorae decoration:
  - A four pixel grey frame, lit from the top left.
  - A title bar shading from navy to light blue (grey when inactive), with
    the title in bold white.
  - Bevelled 16 × 14 minimize, zoom and close buttons. The glyphs are the
    decoration's own bitmaps, and the buttons press in when clicked.
  - Every edge resizes, and so do the ends of each edge from the corner.
    Document windows get Windows' diagonal size grip.
  - Tool windows get a lower title bar. Menus and tool tips get the outer
    half of a raised edge.
- **Control look** (`add-ons/control_look/Reactionary`), after Qt's
  "Windows" style, which the theme uses:
  - Push buttons with a two pixel raised bevel. Pressed buttons sink and
    their label moves down and right. The default button gets a black
    frame.
  - White sunken fields, check boxes and radio buttons. The check box has
    Windows' tick and the radio buttons are drawn pixel by pixel.
  - Drop-down lists with a raised arrow button.
  - Scroll bars with a dithered track and raised arrows and thumb. A bar
    that can't scroll has no thumb.
  - Raised menus with a navy highlight. An open menu bar title sinks into
    the bar.
  - Classic tabs, etched group boxes, a progress bar made of blocks, and
    sliders with a sunken channel.
  - Dotted focus rectangles and engraved disabled text.
- **Smoke** (`add-ons/decorators/Smoke`, `add-ons/control_look/Smoke`), a
  window decorator and control look in the style of ZETA's "Smoke"
  decorator and ZETA's controls. They were redrawn from screenshots of ZETA
  1.21, measured pixel by pixel; no ZETA code or artwork is used. Theme
  Manager themes that name the Smoke decorator (such as the "smoke" sample
  theme) get both.
  - A sliding BeOS tab shading down from the window tab colour into a four
    pixel frame (a black line, a light one, the tab colour, black again),
    with a white cross at its left end and two overlapping squares at its
    right. The bottom right corner of a resizable window is lighter;
    document windows have a grip of six dots there.
  - Rounded buttons, check boxes and fields with black outlines and a soft
    shadow, faces shading downwards, a green tick (the control highlight
    colour) reaching out of its box, and radio buttons to match.
  - Menu fields with a pop-up section in the menu selection colour. ZETA
    has it at the left end; it is at the right here, where Haiku's menu
    fields keep room for it.
  - Scroll bars with a light track, double chevrons and a shaded thumb;
    outlined, rounded tabs on a strip; a shaded menu bar whose open title
    is a sunken box; single line rounded group boxes.
  - Radio buttons, pressed buttons and selected menu items weren't in the
    screenshots, and are drawn to match the rest.
- **The Haiku theme** (`data/themes/Haiku`): Haiku's own look, with its
  default decorator, built-in controls, default colours, desktop colour and
  fonts (from Haiku's source) and Haiku's icons. It switches back to stock
  Haiku whatever you had before; **Revert** instead goes back to *your*
  look from before the first theme, customisations included.
- **The Gonx theme** (`data/themes/Gonx`): the look of
  [Gonx](https://github.com/idumych/gonx), with its colours, desktop colour
  and fonts over Haiku's defaults, and Haiku's icons. Power Theme builds
  Gonx's window borders and controls (below);
  the sources are in `src/third_party/gonx`.
- **The Flat themes** (`data/themes/DarkFlat`, `LightFlat`, `MoonFlat`,
  `OrangeFlat`), from unarix's
  [haiku_darkstyle](https://github.com/unarix/haiku_darkstyle): flat
  window borders and controls in dark, light, moonlit and orange colours,
  the first three over its night sky wallpaper. They use Haiku's own
  FlatDecorator and FlatControlLook, which Haiku ships in its haiku_extras
  package; Renku's repositories don't have it, so Power Theme builds them
  from Renku's source (`src/third_party/haiku-flat`) unless the system
  already has them. Converted from Theme Manager's format by
  `tools/import-uitheme.py`, with Noto Sans standing in for Noto Sans
  Display where that isn't installed; their Terminal, screensaver
  and sound settings are left out.
- **The Renku theme** (`data/themes/Renku`): Smoke window borders and
  DarkFlat's flat controls in the colours of the
  [Renku OS website](https://idumych.github.io/renkuos-website-draft):
  night blue and indigo panels and lists, warm white text, fire orange
  window tabs, menu highlights, focus and scroll bar thumbs, and gold and
  seal red for links and failures. IBM Plex Sans and Mono where they are
  installed, otherwise Noto Sans. Its wallpaper follows the site's front
  page, with 連句 in outline; `tools/make-renku-wallpaper.py` draws it.
- **The Reactionary ME theme** (`data/themes/Reactionary ME`): the
  decorator and control look, the theme's "Windows ME" colour scheme, the
  3A6EA5 desktop colour, Noto Sans 10 pt fonts and the ClassicOS 2000
  icons.
- **Power Theme** (`apps/PowerTheme`, also the `powertheme` command), the
  theme manager. A theme is
  a folder with a `theme.settings` file (see `src/themes/Theme.h`);
  `control look = haiku` and `icons = haiku` stand for Haiku's own. A
  theme can set a wallpaper (`wallpaper = image.png`, scaled, centered,
  tiled or placed; `none`; or `default`, the one Tracker sets up on a new
  system), as the Backgrounds preferences would.
  - Lists the installed themes with a preview, and the icon themes with a
    few of their icons.
  - Applies a whole theme or only some parts of it (window borders,
    controls, colours, fonts, wallpaper, Deskbar position, icons), and any
    icon theme with
    any theme. Parts whose add-ons or files aren't installed are shown as
    such and left out.
  - Shows a theme's light and dark versions as one, with a **Mode** menu, and
    switches between them with the parts you applied (see below).
  - Saves your look and icons the first time, so **Revert** always gets
    back to them, whatever was applied in between.
  - **Build add-ons…** compiles the window decorators and control looks
    (Reactionary, Smoke, Gonx, and Haiku's Flat ones if the system lacks
    them) on the system they will run on, with a progress bar and a log.
    Decorators run inside app_server and use its private classes, and
    control looks subclass Haiku's private HaikuControlLook, so ones built
    for another system build can fail there. The headers that come with
    Power Theme are from one system revision (`build-headers/REVISION`); on any
    other, it finds the system's revision (`hrev60072+339`, from its haiku
    package) and fetches that revision's headers from RenkuOS/Source, or
    haiku/haiku, with git: the commits back to the hrev tag only, the
    commit as many past it as the revision says, then just the 42 header
    files. If the compiler isn't installed, it offers to install it (gcc
    and haiku_devel). A decorator in use is switched to its new build, and
    set aside if app_server doesn't take it. The same from a Terminal:
    `powertheme --build-decorators
    [--install-tools]` (`src/themes/build-decorators.sh`).
  - `powertheme --list`, `--apply=NAME [--parts=…] [--mode=light|dark]`,
    `--mode=light|dark|toggle`, `--icons=ID|haiku`,
    `--light-icons=ID|haiku|keep`, `--dark-icons=ID|haiku|keep`,
    `--revert`, `--status`, `--show=NAME`, `--restart-tracker` and
    `--build-decorators` do the same from a Terminal.
- **Light and dark versions**: two themes are versions of the same one if their
  `theme.settings` say so, with `variant of = Flat` and `mode = dark` (or
  `light`). Each stays a complete theme in its own folder. **Flat** (LightFlat
  and DarkFlat), **Haiku** (Haiku and Haiku Dark) and **Reactionary ME**
  (with Reactionary ME Dark) come this way; other themes stay as they are until
  they have both. `powertheme --mode=dark` switches the theme in use, keeping
  the parts it was applied with.
- **Light and dark icons**: each version of a theme has its own icons: what its
  `theme.settings` names, or what you choose with **Light icons** and **Dark
  icons** in the window (which menu you see follows **Mode**, and **Apply** keeps
  both), or with `--light-icons` and `--dark-icons`. Switching modes then changes
  the icons too. **Keep the current icons** leaves them alone, and so do icons
  set apart from a theme with `--icons`.
- **Haiku icons (dark)**: dark versions of Haiku's own icons, and what Haiku Dark
  uses (`icons = haiku-dark`). Power Theme recolours the icons of the running
  system when it applies them, so new file types and system updates are covered:
  a chosen palette of deeper colours and a thin soft white outline, on the same
  shapes. Icon themes with dark versions, such as Breeze Dark or Papirus-Dark,
  can go in `~/config/non-packaged/data/icons`; they mostly differ in their
  one-colour icons.
- **Switching by the time of day**: `powertheme --schedule=07:00,19:00` switches
  the theme in use to its light version at 07:00 and its dark one at 19:00,
  every day, until `--schedule=off`; **Switch mode every day** in the window does
  the same. It runs in the background, from a job in
  `~/config/settings/launch` at each login. A system started at night comes up
  dark, and a theme switched by hand stays until the next scheduled time.
- **Theme Manager themes**: themes made for
  [Theme Manager](https://github.com/HaikuArchives/HaikuThemeManager)
  (and BeOS and ZETA before it), such as the
  [sample themes](https://github.com/mmuman/HaikuThemes), are listed and
  applied too, from the `UIThemes` folders Theme Manager uses (for example
  `~/config/settings/UIThemes`). Taken over: colours (the ones a theme
  leaves out are Haiku's light or dark defaults, so a theme looks the same
  whatever came before), fonts (Noto Sans where theirs isn't installed),
  title colours, the Deskbar's place, and backgrounds and desktop colours
  per workspace, with images found in the theme's folder wherever it was
  installed. Many
  themes name ZETA decorators, which don't exist on Haiku; they get the
  nearest installed one (Smoke: Smoke, with its controls; Win2k and
  Redmond: Reactionary; GONX: Gonx; BeOS-like ones: BeDecorator; others:
  Default) and Haiku's controls. Terminal, sound and screensaver settings,
  and fonts that come with a theme, are left out.
- **Icon themes**: any [freedesktop icon theme](https://specifications.freedesktop.org/icon-theme-spec/latest/)
  (an `index.theme` with folders of PNG or SVG icons, including the themes
  it inherits from). Power Theme turns each icon into a Haiku vector icon (HVIF)
  that holds the theme's own 16, 24, 32 and 48 pixel designs, pixel for
  pixel, each drawn at its own size. It sets them on over 100 file types (as
  icons of the types, in your MIME database) and on your home, mail and
  queries folders. `data/PowerTheme/icon-map` says which icon goes where, and
  can be extended. The ClassicOS 2000 icons ship as PNG, rendered from the
  theme's SVG with librsvg (`tools/build-icon-theme.py`).

## Limitations

- The light "Windows ME" and dark "WinME-Dark" variants of Reactionary are
  ported. The theme's dim
  colour schemes, cursors and wallpaper are not.
- Icons change for file types and some folders only. Applications keep
  their own icons, which are part of them. The Trash keeps Tracker's icon
  (Tracker resets it whenever the Trash fills or empties), disks keep the
  icons their drivers give them, and alerts keep Haiku's.
- Tracker shows the new folder icons only once it restarts: Power Theme
  offers to restart it (`powertheme --restart-tracker`).
- Icons with many colours lose a few to fit Haiku's vector icon format
  (255 colours and shapes in all); designs that still don't fit are left
  out, largest first. SVG icons are drawn with nanosvg, which leaves out
  elements with filters (usually blurred shadows).
- Dark icons can't reach application icons, which stay as they are.
- Tracker shows a scheduled change of folder icons at its next restart, as it
  does any other; Power Theme doesn't restart it behind your back.
- Windows have no icon in the title bar: app_server decorators have no
  access to application icons.
- Haiku makes default buttons bigger than the others; Reactionary draws
  them only one pixel bigger on each side, but the layout keeps the room.
- Slider fill colours don't show, because the channel is as thin as
  Windows'.

## Building

On Renku or Haiku, `make` builds everything against the running system.
It uses the private app_server and FreeType headers bundled in
`build-headers/`. `make demo` builds `build/controls-demo`, a window with
every kind of control.

To cross-build on Linux against a Renku source tree, run
`sh tools/sync-sysroot.sh <ssh-host>` once per Renku build, then `make`.

`make icon-test` builds `build/icon-test`, which encodes an icon's PNG
designs and compares Haiku's rendering of the result with them, pixel by
pixel.

`python3 tools/build-icon-theme.py <ClassicOS-2000-Icons checkout> build/icons`
renders the ClassicOS 2000 icons (needs `rsvg-convert`).

`sh packaging/hpkg/build-hpkg.sh <build-dir> <icons-dir>` makes the
package on a Haiku or Renku system, from `make build/PowerTheme
decorator-source` and the rendered icons. Build it on the oldest system it
should run on (Haiku R1/beta6), against that system's own headers.

## Credits

- Reactionary ME global theme, "Windows ME" window decoration and "Windows ME"
  and "WinME-Dark" colour schemes by Die4Ever (GPL 3+), forked from Reactionary Plus (phob1an),
  Windows Classic hidpi (pixelocdguy) and Windows Classic colours.
- ClassicOS 2000 icons by the ClassicOS contributors
  ([licence](https://github.com/ClassicOS-Themes/ClassicOS-2000-Icons/blob/main/LICENSE):
  redistribution permitted). Brand icons in the set are trademarks of
  their owners.
- DarkFlat, LightFlat, MoonFlat, OrangeFlat themes and their wallpaper
  from [haiku_darkstyle](https://github.com/unarix/haiku_darkstyle) by
  nhtello (unarix), MIT licence (in each theme's folder).
  Haiku Dark uses DarkFlat's colours.
- FlatDecorator and FlatControlLook from Haiku (as in Renku OS's source),
  MIT licence, Haiku, Inc.; FlatControlLook by nhtello.
- Smoke's look follows ZETA's Smoke decorator and controls (yellowTAB,
  later magnussoft). ZETA is not affiliated with this project. Smoke is
  drawn from scratch after screenshots; nothing from ZETA is included.
- [nanosvg](https://github.com/memononen/nanosvg) by Mikko Mononen (zlib
  licence), in `src/themes/third_party`.
- Gonx's window decorator and control look (`src/third_party/gonx`), MIT
  licence, from [Gonx](https://github.com/idumych/gonx) 0.2.
- Project layout, scripts and the start of the theme tool from
  [Gonx](https://github.com/idumych/gonx).

Power Theme's own code is under the MIT licence (see `LICENSE`).
