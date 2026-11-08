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
- **Renku** and **Renku Light**, in the colours of the Renku OS website, with
  dark and normal Haiku icons;
- **Haiku Dark**, a black and yellow version of Haiku's look, with dark versions
  of Haiku's own icons;
- **Dano**, the controls of BeOS R5.1 "Dano", with Haiku's window borders;
- **Tidy** and **Tidy Dark**, after mockups from Haiku's forum, and **Tidy Square**
  and **Tidy Square Dark**, with square corners for a compositor to round, and
  **Tidy Renku** and **Tidy Renku Light**, in Renku OS's colours, also square as
  **Tidy Renku Square** and **Tidy Renku Square Light**;
- **Expose** and **Expose Dark**, after Mark Whittaker's KDE themes;
- **Platinum**, in the style of Mac OS 9, after Niels's KDE theme;
- **Leopard** and **Leopard Graphite**, in the style of Mac OS X 10.5, with
  elementary's icons;
- **Gershwix**, after Mintar's Platinum theme for Mac OS X, in its seven colours
  (Ivy, French Blue, Poppy, Crimson, Plum, Sunny and Silver);
- **NeXT**, after Swizcore Studio's NeXTSTEP theme for Mac OS X, with its leather
  folders;
- **System 7**, in the style of Mac OS 7.5;
- **SuMa**, after the SuMa 1 GUI kit for Mac OS X, in Orange, Blue and Green;
- and the Haiku, Gonx and four Flat themes.

The Reactionary ME look is an independent, unofficial port. It is not
made by, affiliated with or endorsed by Die4Ever, the developers of
Reactionary, Reactionary Plus or the themes they build on, or the
ClassicOS project. It is a new implementation drawn to match the KDE
theme. Colours and the title bar button glyphs come from the theme's
files; the code shares nothing with them.

## Haiku Dark

| | |
|---|---|
| ![Haiku Dark](docs/screenshots/haiku-dark.png) | ![Haiku](docs/screenshots/haiku-light.png) |
| **Haiku Dark** | **Haiku**, one mode switch away |

Haiku Dark and Haiku are the light and dark versions of one theme, so they share
a row in Power Theme with a **Mode** menu, and can switch by the time of day.
Haiku Dark's icons are Haiku's own, recoloured when the theme is applied: the same
shapes, in deeper colours.

![Haiku's icons above their dark versions](docs/screenshots/dark-icons.png)

![Dark icons in Tracker under Haiku Dark](docs/screenshots/dark-tracker.png)

```
powertheme --apply=Haiku --mode=dark
powertheme --schedule=07:00,19:00
```

These pictures are from a Renku OS build with the changes below, which is
why the windows change in place and the Deskbar logo is light. The test app at the
top left of the first pictures shows how an application can follow the mode.

## Leopard

![Leopard](docs/screenshots/leopard.png)

**Leopard** looks like Mac OS X 10.5: a grey title bar with red, yellow and green
gel buttons, white pill buttons with a blue default, blue gel pop-ups, check boxes
and scroll bars, a vivid blue menu selection, the Deskbar across the top like a menu
bar, and elementary's blue-grey folders, over an aurora of its own. **Leopard
Graphite** is the same in Graphite's steel blue. Everything in them is drawn from
scratch after screenshots of Leopard; nothing of Apple's is included.

## BeOS-style tabs

![Reactionary ME with BeOS-style tabs](docs/screenshots/beos-tabs.png)

Window borders that can draw one can have a BeOS-style tab instead of a title bar
across the window, whatever the theme. It's as wide as the title, with close on the
left and zoom on the right, and shift-dragging it slides it along the window. That's
Reactionary's, Expose's, Platinum's, Leopard's, Gershwix's, NeXT's, System 7's and SuMa's borders; turn it on with **BeOS-style tabs** in the window, or
`powertheme --beos-tabs=on`. Run **Build add-ons…** after updating to get it.

## What needs Renku OS changes

Power Theme works without these changes, including Haiku Dark, its icons and the
schedule, as tested on Haiku R1/beta6. A few things only happen in place, or at
all, with changes to Renku OS. Some are merged, and come with Renku OS builds from
November 2026 on; the rest are still in review. Power Theme checks for the first
two and says what to do if they're missing. The
[Power Theme: Live Themes page](https://idumych.github.io/power-theme/) shows all of
this work, with what's finished, tested and still open.

| What | Without them | Needs |
|---|---|---|
| Running applications change their controls | Restart them | [#284](https://github.com/RenkuOS/Source/pull/284) (merged), [#285](https://github.com/RenkuOS/Source/pull/285) |
| Running applications change their fonts | Restart them | [#286](https://github.com/RenkuOS/Source/pull/286), [#287](https://github.com/RenkuOS/Source/pull/287) |
| Open Tracker windows show new file type icons | Some wait for Tracker to restart | [#292](https://github.com/RenkuOS/Source/pull/292) (merged) |
| Open Tracker windows fit a new font | Restart Tracker | [#293](https://github.com/RenkuOS/Source/pull/293), with #286 and #287 |
| The Deskbar logo is light on dark menus | It stays navy | [#288](https://github.com/RenkuOS/Source/pull/288) (merged) |
| Applications can ask whether the desktop is dark, such as to swap their artwork | They can't, with a standard call | [#289](https://github.com/RenkuOS/Source/pull/289) |
| Check boxes and radio buttons stay readable on dark panels, in applications that place controls by hand | Their labels can stay black | [#295](https://github.com/RenkuOS/Source/pull/295) |
| Menus join a submenu's selection to the item it opened from, as BeOS Dano's ZSnake did, as in the Expose and Platinum pictures | They don't | [#279](https://github.com/RenkuOS/Source/pull/279) (merged), with [#280](https://github.com/RenkuOS/Source/pull/280) to turn it on in Appearance and [#281](https://github.com/RenkuOS/Source/pull/281) for the Deskbar's menu button |
| Desktop labels stay readable on a plain pale desktop | They stay white, so Renku Light sets a darker system desktop colour to get black ones | [#296](https://github.com/RenkuOS/Source/pull/296) |
| A slid BeOS-style tab keeps its place when the window borders change | It goes back to the start | [#297](https://github.com/RenkuOS/Source/pull/297) |

## Screenshots

All on Renku OS, with a window of controls and a Tracker window (and, for
Reactionary ME, Platinum and Leopard, a folder of different file types, to show
their icons). The picture at the top is Power Theme itself, under the stock Haiku
theme.

| | |
|---|---|
| ![Reactionary ME](docs/screenshots/reactionary-me.png) | ![Renku](docs/screenshots/renku.png) |
| **Reactionary ME**: Windows ME window borders and controls, and the ClassicOS 2000 icons on folders and file types | **Renku**: Smoke window borders and flat controls in the Renku OS website's colours, with the dark Haiku icons |
| ![Renku Light](docs/screenshots/renku-light.png) | ![DarkFlat](docs/screenshots/darkflat.png) |
| **Renku Light**: the same on washi paper, with Haiku's own icons | **DarkFlat**, from haiku_darkstyle, with Haiku's Flat window borders and controls |
| ![Smoke](docs/screenshots/smoke.png) | ![Gonx](docs/screenshots/gonx.png) |
| **Smoke** window borders and controls, in the style of ZETA's, with the colours of the "smoke" Theme Manager theme | **Gonx**: Gonx's window borders and controls |
| ![Dano](docs/screenshots/dano.png) | ![Tidy](docs/screenshots/tidy.png) |
| **Dano**: the controls of BeOS R5.1 "Dano", with Haiku's window borders | **Tidy**: a thin frame, soft menu bars and slim scroll bars, after mockups from Haiku's forum |
| ![Tidy Dark](docs/screenshots/tidy-dark.png) | ![Expose](docs/screenshots/expose.png) |
| **Tidy Dark**: the same in dark | **Expose**: a dark glossy title bar over soft grey controls with orange-topped tabs, with ZSnake on in the menus |
| ![Expose Dark](docs/screenshots/expose-dark.png) | ![Platinum](docs/screenshots/platinum.png) |
| **Expose Dark**: the same title bar with dark glass controls and slate selections, with ZSnake on | **Platinum**: Mac OS 9's bevelled frame and striped title bar, lavender controls and NineIcons Redux, with ZSnake on |
| ![Leopard](docs/screenshots/leopard.png) | ![Leopard Graphite](docs/screenshots/leopard-graphite.png) |
| **Leopard**: Mac OS X 10.5's grey title bar and red, yellow and green gel buttons, white pill buttons, blue gel pop-ups, check boxes and scroll bars, elementary's blue-grey folders and an aurora of its own | **Leopard Graphite**: the same in Graphite's steel blue, window buttons too |
| ![Gershwix French Blue](docs/screenshots/gershwix.png) | ![NeXT](docs/screenshots/next.png) |
| **Gershwix French Blue**: a striped grey title bar with square boxes, light bevelled buttons, a ridged scroll thumb and slider knob and a tube-shaded progress bar in French blue, and NineIcons Redux; six other colours share the rest | **NeXT**: NeXTSTEP's black title bar with bevelled grey boxes, square grey buttons, stippled scroll bars with dimpled thumbs, a black progress bar and a slate desktop, with its leather folders |
| ![System 7](docs/screenshots/system7.png) | ![SuMa Blue](docs/screenshots/suma.png) |
| **System 7**: Mac OS 7.5's striped title bar with a close box and a zoom box, black frames with a drop shadow, white rounded buttons, a lavender scroll thumb and the dotted grey desktop | **SuMa Blue**: dark grey metal windows with small beads, shiny black glass buttons and controls marked in blue, dark menus, and gold tabs to set the blue off; Orange and Green share the rest |
| ![The Power Theme window](docs/screenshots/themes-window.png) | |
| **Power Theme** under Renku: the theme list with a preview, the icon theme, and the parts to apply | |

![Build add-ons](docs/screenshots/build-add-ons.png)

**Build add-ons…** compiles the window borders and controls for the running
system, here with the bundled headers, which match this Renku build.

## Install

Power Theme comes as a package for Haiku R1/beta6 and later, and Renku OS
(x86_64), on the [releases page](https://github.com/idumych/power-theme/releases):

```sh
pkgman install ./powertheme-0.8-1-x86_64.hpkg
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

### On Linux, with Hokku

[Hokku](https://github.com/idumych/hokku), the Haiku API on Linux, runs the
same window decorators and control looks, built from the same sources, in
its applications: it draws their window frames itself on Wayland, rounding
their corners and casting a shadow, or a glow on a dark desktop. Power
Theme themes Hokku's applications there. It needs Hokku installed with its
development files (headers, `libdecorator`, `rc`), and g++ and binutils to
build the add-ons:

```sh
make COSMOE_ROOT=/usr && sudo make COSMOE_ROOT=/usr install
PowerTheme                    # then Build add-ons… and apply a theme
```

The add-ons go in `~/cosmoe/addons`. A theme's window borders show in
applications started after it is applied; colours and fonts change in
running ones too. "Default", Haiku's window borders, is Haiku there. A
light/dark schedule starts at login from `~/.config/autostart`. Other Linux
applications, and Hokku applications run from an AppImage, keep the
desktop's own frames; on X11 the window manager draws them all.

On Arch and CachyOS with Hokku installed, `packaging/arch/PKGBUILD` (the same
as Hokku OS's `packages/power-theme`) makes a package instead. It comes with
the window borders and controls already built into Hokku's add-ons folder, so
themes work at once, and registers Power Theme with Hokku's file types and the
Deskbar's Preferences:

```sh
cd packaging/arch && makepkg -si
```

Rebuild the package when Hokku's decorator classes change; until then,
**Build add-ons…** builds them into `~/cosmoe/addons`, which comes first.

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
- **Dano** (`add-ons/control_look/Dano`, `data/themes/Dano`): the controls of
  BeOS R5.1 "Dano", redrawn from screenshots of a running system: white buttons
  with a soft shadow, a default button on a blue plate, menu fields with a blue
  tab on the left, an overflowing tick and radio bullet, sunken grey sliders and
  swept tabs. It also turns on double scroll bar arrows, as Dano has, until you
  leave the theme. Its window borders are Haiku's own for now.
- **Tidy** (`add-ons/decorators/Tidy`, `add-ons/control_look/Tidy`,
  `data/themes/Tidy`, `data/themes/Tidy Dark`): a light and dark pair after the
  ["Tidy" mockups](https://discuss.haiku-os.org/t/ui-adjustments-that-stay-out-of-the-way/16988)
  by anon40228759: a rounded yellow tab with its title in the middle, a thin
  rounded frame that only has an outline along the top, soft menu bars, pale
  column headers, and slim scroll bars that are blank when there's nothing to
  scroll. Without compositing, its rounded corners aren't anti-aliased. The
  mockups' changes inside applications, such as Tracker's folder path, list
  icon sizes and spacing and a new HaikuDepot, aren't something a theme can do.
- **Tidy Square** (`add-ons/decorators/TidySquare`, `data/themes/Tidy Square`,
  `data/themes/Tidy Square Dark`): Tidy with the corners of its tab and frame left
  square, for a compositor that rounds windows itself. Its colours and controls
  are Tidy's.
- **Tidy Renku** (`data/themes/Tidy Renku`, `data/themes/Tidy Renku Light`): Tidy's
  window borders, controls and fonts with the colours, icons and wallpapers of the
  Renku themes. **Tidy Renku Square** (`data/themes/Tidy Renku Square`,
  `data/themes/Tidy Renku Square Light`) is the same with Tidy Square's window borders.
- **Expose** (`add-ons/decorators/Expose`, `add-ons/control_look/Expose`,
  `data/themes/Expose`, `data/themes/Expose Dark`): a light and dark pair after
  [Expose Dark](https://store.kde.org/p/2347990) and its
  [Kvantum theme](https://store.kde.org/p/2348069), and
  [Expose Air Dark](https://store.kde.org/p/2357405), by Mark Whittaker: a dark
  glossy title bar across the window, split into a bright and a dark half, with
  its title on the left and glass buttons on the right, the close button red. The
  light theme has soft grey buttons with a white rim, orange-topped tabs and a
  round slider thumb, and the dark one dark glass controls with slate selections. Without compositing, its soft shadow is left out and its
  rounded corners aren't anti-aliased. Expose Dark's Kvantum theme has no dark
  palette, hence Expose Air Dark's controls in the dark theme. Its add-ons are under
  the GNU GPL, unlike the rest of Power Theme.
- **Platinum** (`add-ons/decorators/Platinum`, `add-ons/control_look/Platinum`,
  `data/themes/Platinum`): Mac OS 9's look, after
  [Platinum Retro OS9](https://store.kde.org/p/2361987) by Niels, with the
  [QPlatinum](https://store.kde.org/p/1287747) colours: a wide bevelled frame with a
  hard shadow, a striped title bar with its title in the middle and the close
  button on the left, bevelled grey buttons that go dark when pressed, lavender
  scroll bars and pop-up arrows, a tiled wallpaper, and the
  [NineIcons Redux](https://store.kde.org/p/1960539) icons on folders and file
  types. It has no dark version. Its add-ons are under the GNU GPL, unlike the rest of Power Theme.
- **Leopard** (`add-ons/decorators/Leopard`, `add-ons/control_look/Leopard`,
  `data/themes/Leopard`, `data/themes/Leopard Graphite`): in the style of Mac OS X
  10.5 Leopard, drawn from scratch after screenshots of it. A grey title bar across
  the window with rounded top corners, three gel buttons at its left (red close,
  yellow minimize and green zoom; a window without minimize or zoom shows them
  greyed out, as Leopard does) and its title in the middle over a light edge; a thin
  line round the content, and Leopard's grip of ridges on document windows. An
  inactive window's bar is lighter and its buttons grey, and a pressed button shows
  its glyph. The controls are Aqua gels: white pill buttons with a blue default and
  pressed one, pop-up menus with a blue end holding two arrows, check boxes and radio
  buttons that turn blue when on, blue capsules in shaded scroll bar tracks, round
  blue slider knobs, segmented tabs, a striped blue progress bar, a light shaded menu
  bar and a vivid blue menu selection. **Leopard Graphite** is Leopard's Graphite
  appearance, with all of that in steel blue; the add-ons tell from how saturated the
  control highlight colour is. Both put the Deskbar across the top of the screen,
  like Leopard's menu bar, use Lucida Grande and Monaco where they're installed (Noto
  otherwise), and have an aurora wallpaper of their own, drawn by
  `tools/make-leopard-wallpaper.py`. Their icons are
  [elementary's](https://github.com/elementary/icons) from 2017 (version 4.3.1, the
  last with its blue-grey folders), rendered by `tools/make-elementary-icons.py`.
  With a compositor, the top corners are rounded smoothly and the bottom ones stay
  square; without one, the top corners are cut pixel by pixel.
- **Gershwix** (`add-ons/decorators/Gershwix`, `add-ons/control_look/Gershwix`,
  and seven themes, `data/themes/Gershwix Ivy` to `data/themes/Gershwix Silver`):
  after [Gershwix](http://www.geocities.com/gershwix/) 2.5 by Mintar (2002-2007), a
  Platinum theme for Mac OS X's ShapeShifter, drawn from scratch after measuring its
  pictures. A light grey title bar across the window, striped across, with square
  etched close, minimize and zoom boxes at its left and its title in the middle; a
  dark line round the window, and a grip on document windows. An inactive window's
  boxes are flat grey squares and its title grey. The controls are Platinum's with
  softened corners: light bevelled push buttons and pop-up menus (with a second
  bevel holding two arrows), a dark ring round the default button, square check
  boxes whose tick reaches out of the top, shaded radio buttons, sunken text
  fields, a shaded scroll bar track, and in the accent colour a ridged scroll bar
  thumb, a ridged slider knob and a progress bar shaded like a tube, with a flat
  menu selection in the accent's dark shade. The seven themes differ only in that
  accent, which the control look takes from the scroll bar thumb colour: Gershwix's
  seven have the shades measured from it, and any other colour is shaded the same
  way. They put the Deskbar across the top, use Lucida Grande and Monaco where
  they're installed (Noto otherwise), have a plain desktop in a dark tone of their
  accent, as Gershwix has no wallpaper, and use the NineIcons Redux icons.
- **NeXT** (`add-ons/decorators/NeXT`, `add-ons/control_look/NeXT` and
  `data/themes/NeXT`): after NeXT 2.0 by Swizcore Studio (2004), a NeXTSTEP theme
  for Mac OS X's ShapeShifter, drawn from scratch after measuring its pictures. A
  black title bar across the window, with a light edge along its top and left and
  a grey one along its bottom and right, three bevelled grey boxes at its left
  (close with a cross, minimize with a triangle and zoom with a window) and its
  title in white bold in the middle; a black line round the window, and a ridged
  grip on document windows. An inactive window's bar is grey, its boxes darker and
  its title black. The controls are in NeXTSTEP's greys: square buttons and pop-up
  menus with a white edge along their top and left and grey and black ones along
  their bottom and right, which turn white with black along their top and left
  when pressed; a black line round the default button; a small raised bar as the
  pop-up mark; check boxes with a black and white tick; radio buttons with a dark
  rim; stippled scroll bar tracks with dimpled grey thumbs; a stippled slider
  groove with a square knob; a black progress bar in a stippled track; sunken text
  fields; and a white menu selection. The theme puts the Deskbar across the top,
  uses Helvetica and Courier where they're installed (Nimbus or Noto otherwise),
  has a plain slate desktop, as NeXTSTEP's was, and its own icons
  (`data/icons/NeXT`, drawn by `tools/make-next-icons.py`): NeXTSTEP's leather
  folder tied with a cord (with home, mail and query versions) and a black display
  for the computer, after the two icons the kit comes with, and NineIcons Redux
  for the rest.
- **System 7** (`add-ons/decorators/System7`, `add-ons/control_look/System7` and
  `data/themes/System 7`): in the style of Mac OS 7.5 with its standard window
  colour, drawn from scratch after screenshots of Mac OS 7.5.5. A light grey title
  bar striped every other row, with a light lavender edge along its top and left
  and a darker one along its bottom and right, a close box at its left and a zoom
  box at its right, sunk into the bar with a raised grey face, and the title in the
  middle; a black line round the window with a drop shadow a pixel wide, and a
  grow box of two overlapping squares. An inactive window's bar is plain white,
  with a grey title and frame. There's no minimize box, as System 7.5 had none: a
  double click on the bar minimizes. The controls are System 7's: white buttons
  with a black rounded outline that turn black when pressed, a thick ring round
  the default one, pop-up menus as white boxes with a shadow and a black triangle,
  check boxes with a cross, radio buttons with a dot, scroll bars with hollow
  arrows, a stippled track and a lavender thumb, black outlines round text fields
  and progress bars, and white menus with a black selection. The theme has the
  black and white dotted desktop, the Deskbar across the top, ChicagoFLF or Chicago
  where installed (Noto otherwise), and NineIcons Redux.
- **SuMa** (`add-ons/decorators/SuMa`, `add-ons/control_look/SuMa`, and three
  themes, `data/themes/SuMa Orange`, `SuMa Blue` and `SuMa Green`): after the SuMa 1
  GUI kit (2006), a ShapeShifter theme for Mac OS X "based off scrap materials from
  Susumu Yoshida", drawn from scratch after measuring its pictures, with its
  windows as its metal ones looked. A dark grey title bar with rounded top corners,
  three small dark beads in sunken sockets for close, minimize and zoom, which fill
  with the accent when pressed, and a white title. The controls are black glass:
  pill buttons with a ring in the accent round the default one and an accent glow
  when pressed, pop-up menus, check boxes with an accent tick, radio buttons with an
  accent dot, a black scroll thumb with an accent stripe in a light track, a black
  slider knob with an accent dot, and a glossy accent progress bar in a black
  track; menus and menu bars are dark, with a flat accent selection, and tabs are
  glossy in a colour that sets the accent off (blue for Orange, gold for Blue,
  purple for Green), as the kit had them. The three themes differ in that accent,
  which the add-ons take from the control highlight colour. They have a dark grey
  desktop, the Deskbar across the top, Lucida Grande where installed (Noto
  otherwise), and elementary's icons. The beads are drawn by
  `tools/make-suma-art.py`.
- **The Gonx theme** (`data/themes/Gonx`): the look of
  [Gonx](https://github.com/idumych/gonx), with its colours, desktop colour
  and fonts over Haiku's defaults, and Haiku's icons. Power Theme builds
  Gonx's window borders and controls (below);
  the sources are in `src/third_party/gonx`.
- **Rounded corners:** every window decorator here can have its corners rounded
  by a composited app_server, where Appearance has an option for it. They find
  out when they are built, from the system's headers. Everywhere else they build
  without this, and draw their corners as they always have.
- **The Flat themes** (`data/themes/DarkFlat`, `LightFlat`, `MoonFlat`,
  `OrangeFlat`), from unarix's
  [haiku_darkstyle](https://github.com/unarix/haiku_darkstyle): flat
  window borders and controls in dark, light, moonlit and orange colours,
  the first three over its night sky wallpaper. They use Haiku's own
  FlatDecorator and FlatControlLook, which Haiku ships in its haiku_extras
  package; Renku's repositories don't have it, so Power Theme builds them
  from Renku's source (`src/third_party/haiku-flat`, with two small changes
  listed in its README) unless the system already has them. Converted from Theme Manager's format by
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
    (Reactionary, Smoke, Dano, Tidy, Expose, Platinum, Leopard, Gershwix, NeXT, System 7, SuMa, Gonx, and
    Haiku's Flat ones if the system lacks them) on the system they will run on, with a progress bar and a log.
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
  and DarkFlat), **Haiku** (Haiku and Haiku Dark), **Renku** (Renku Light and Renku),
  **Reactionary ME** (with Reactionary ME Dark), **Tidy**, **Tidy Square**, **Tidy
  Renku**, **Tidy Renku Square** and **Expose** come this way; other themes stay as they are until
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
  a chosen palette of deeper colours, on the same shapes. Icon themes with dark versions, such as Breeze Dark or Papirus-Dark,
  can go in `~/config/non-packaged/data/icons`; they mostly differ in their
  one-colour icons.
- **Switching by the time of day**: `powertheme --schedule=07:00,19:00` switches
  the theme in use to its light version at 07:00 and its dark one at 19:00,
  every day, until `--schedule=off`; **Switch mode every day** in the window does
  the same. It runs in the background, from a job in
  `~/config/settings/launch` at each login. A system started at night comes up
  dark, and a theme switched by hand stays until the next scheduled time.
- **BeOS-style tabs**: **BeOS-style tabs** in the window, or `powertheme
  --beos-tabs=on`, gives window borders that can draw one a tab as wide as the
  title on top of the frame, with close on the left and zoom on the right, instead
  of a title bar across the window, and shift-dragging a tab slides it. It's for any theme using those borders, and so
  far that's Reactionary's, Expose's, Platinum's, Leopard's, Gershwix's, NeXT's, System 7's and SuMa's. It stays on through a change of mode.
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
  Redmond: Reactionary; GONX: Gonx; Baqua: Leopard; BeOS-like ones: BeDecorator; others:
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

- A Deskbar along the side of the screen, or collapsed, sizes its logo by the window
  tab's height, which it reads only as it starts. So Power Theme restarts the Deskbar
  when a theme changes that height. Its tray icons from add-ons come back; an icon put
  there by a running application may not until that application restarts.
- The light "Windows ME" and dark "WinME-Dark" variants of Reactionary are
  ported. The theme's dim
  colour schemes, cursors and wallpaper are not.
- Icons change for file types and some folders only. Applications keep
  their own icons, which are part of them. The Trash keeps Tracker's icon
  (Tracker resets it whenever the Trash fills or empties), disks keep the
  icons their drivers give them, and alerts keep Haiku's.
- Tracker shows the new folder icons only once it restarts: Power Theme
  offers to restart it (`powertheme --restart-tracker`). On Renku OS builds with
  [#292](https://github.com/RenkuOS/Source/pull/292), most of them show at once.
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
- Leopard's window buttons have no glyphs until they're pressed: a decorator
  isn't told when the pointer is over them. Its title bar can't run on into a
  toolbar as Leopard's unified windows do, as toolbars are part of applications, and
  its scroll arrows are at each end of the bar, where Haiku puts them. Without a
  compositor, its windows have no shadow. Desktop icons already placed where the
  Deskbar now runs across the top stay under it until they're moved.
- Gershwix's title bar has no toolbar button at its right end, as Haiku's
  windows have no toolbar to hide, and its scroll arrows are at each end of the
  bar rather than together. In Gershwix itself the theme named "Crimson" is purple
  and "Plum" red; here each is named for its colour.
- NeXT's scroll arrows are at each end of the bar, where Haiku puts them, rather
  than together as NeXTSTEP had them, and its menu bar is grey like its menus, as
  Haiku gives both one colour; in the original theme Mac OS X's menu bar is black.
- System 7's desktop icon labels aren't on white boxes as Mac OS's were: Haiku's
  Tracker no longer draws a background behind them, so they get a light glow on the
  dotted desktop instead. Its scroll thumb grows with the content, where Mac OS's
  was a fixed square.
- SuMa's menu bars are dark like its menus, where the kit's menu bar was light, as
  Haiku gives menus and menu bars one text colour; its window bar is the dark grey
  of the kit's metal windows, which its other windows didn't have. The empty part
  of Renku's horizontal Deskbar stays white, under every theme.

## Building

On Renku or Haiku, `make` builds everything against the running system.
It uses the private app_server and FreeType headers bundled in
`build-headers/`. `make demo` builds `build/controls-demo`, a window with
every kind of control.

To cross-build on Linux against a Renku source tree, run
`sh tools/sync-sysroot.sh <ssh-host>` once per Renku build, then `make`.

For Hokku on Linux, `make COSMOE_ROOT=<Hokku's prefix>` builds
`build/hokku/PowerTheme` and the add-ons' source folder (`hokku.mk`), and
`make COSMOE_ROOT=... install` installs them with the themes and icons.

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
- Dano's look follows the controls of BeOS R5.1 "Dano". Be Inc.'s successors are
  not affiliated with this project. It is drawn from scratch after screenshots of
  a running system; nothing from Dano is included.
- Tidy's look follows the "Tidy" mockups by anon40228759 on
  [Haiku's forum](https://discuss.haiku-os.org/t/ui-adjustments-that-stay-out-of-the-way/16988).
  They aren't affiliated with this project. It is drawn from scratch; the mockups
  aren't included.
- Expose's window decorator and control look (`src/third_party/expose`), GNU GPL
  version 3 or later, follow [Expose Dark](https://store.kde.org/p/2347990), its
  [Kvantum theme](https://store.kde.org/p/2348069) and
  [Expose Air Dark](https://store.kde.org/p/2357405) by Mark Whittaker (GPL), whose
  artwork is included and whose buttons and check marks they use. Mark Whittaker
  isn't affiliated with this project.
- Platinum's window decorator and control look (`src/third_party/platinum`), GNU
  GPL version 3 or later, follow
  [Platinum Retro OS9](https://store.kde.org/p/2361987) by Niels (GPL), whose
  window decoration artwork and wallpaper are included. Its colours follow
  [QPlatinum](https://store.kde.org/p/1287747) by zinjanthr0pus. Its controls are
  drawn from scratch, measuring
  [Mac9Kvantum](https://store.kde.org/p/1766812) by drgordbord, which has no licence,
  so nothing of it is included. Its icons are from
  [NineIcons Redux](https://store.kde.org/p/1960539) by aitees (MIT licence, in
  `data/icons/NineIconsRedux`). Apple and these authors aren't affiliated with this
  project.
- Leopard's look follows Mac OS X 10.5 Leopard. It is drawn from scratch after
  screenshots ([512 Pixels' Aqua screenshot library](https://512pixels.net/projects/aqua-screenshot-library/mac-os-x-10-5-leopard/)),
  and its wallpaper is its own; nothing of Apple's is included. Its icons are from
  [elementary](https://github.com/elementary/icons) 4.3.1 by Daniel Foré and the
  elementary contributors (GNU GPL version 3 or later, in `data/icons/elementary`).
  Apple and elementary aren't affiliated with this project.
- Gershwix's look follows [Gershwix](http://www.geocities.com/gershwix/) 2.5 by
  Mintar (© 2002-2007), which states no licence. It is drawn from scratch after
  measuring the theme's pictures; none of its artwork is included. Its icons are
  NineIcons Redux (above). Apple and Mintar aren't affiliated with this project.
- NeXT's look follows NeXT 2.0 by Swizcore Studio (© 2004), which states no
  licence. It is drawn from scratch after measuring the theme's pictures; none of
  its artwork is included; so are its folder and computer icons, drawn from scratch
  after the kit's, with NineIcons Redux (above) for the rest. Apple, NeXT and
  Swizcore Studio aren't affiliated with this project.
- System 7's look follows Mac OS 7.5. It is drawn from scratch after measuring
  screenshots of Mac OS 7.5.5 from Nathan Lineback's
  [toastytech.com](http://toastytech.com/guis/mac755.html); nothing of Apple's is
  included. Apple isn't affiliated with this project.
- SuMa's look follows the SuMa 1 GUI kit (© 2006), which states no licence and is
  "based off scrap materials from Susumu Yoshida". It is drawn from scratch after
  measuring the kit's pictures; none of its artwork is included. Its icons are
  elementary's (above). Apple and the kit's authors aren't affiliated with this
  project.
- [nanosvg](https://github.com/memononen/nanosvg) by Mikko Mononen (zlib
  licence), in `src/themes/third_party`.
- Gonx's window decorator and control look (`src/third_party/gonx`), MIT
  licence, from [Gonx](https://github.com/idumych/gonx) 0.2.
- Project layout, scripts and the start of the theme tool from
  [Gonx](https://github.com/idumych/gonx).

Power Theme's own code is under the MIT licence (see `LICENSE`), Leopard's,
Gershwix's, NeXT's, System 7's and SuMa's add-ons included, apart from Expose's and Platinum's add-ons, which are under the
GNU GPL (`src/third_party/expose/LICENSE`, `src/third_party/platinum/LICENSE`), as
are the elementary icons (`data/icons/elementary/COPYING`).
