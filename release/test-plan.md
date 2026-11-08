# Test plan: Power Theme

Run on an installed Haiku R1/beta6 on a large disk, not a live image, with a
fresh home folder. Commands go over SSH; the screenshots come from the VM.

## Fixtures

None.

## Fresh state

`rm -rf ~/config/settings/PowerTheme ~/config/non-packaged/add-ons/decorators
~/config/non-packaged/add-ons/control_look`

## Feature checks

### themes
Steps: `powertheme --list > {p}-themes.txt`.
PASS when: the list has the Renku pair, "Renku Light (light version of Renku)".

### addons
Steps: `powertheme --build-decorators --install-tools`, then `powertheme --list`.
PASS when: the build exits 0 and no theme says "not installed":
"themes missing add-ons: 0" in {p}-addons.txt. Reboot once afterwards, and the
desktop must come back with the theme applied and no crash in the system log.

### modes
Steps: `powertheme --apply=Haiku`, `--mode=dark`, `--status`, `--mode=light`,
`--status`, `--schedule=07:00,19:00`, `--status`, `--schedule=off`.
PASS when: {p}-modes.txt shows mode: dark, mode: light and the schedule.

### dark-icons
Steps: `powertheme --apply="Haiku Dark"`, and read the `META:ICON` of
`~/config/settings/mime_db/text/plain`. `powertheme --apply=Haiku`, and read it
again. Compare both with the system's own icon.
PASS when: {p}-icons.txt says the dark icon differs and the light one matches.
Look at a Tracker window in a screenshot too: the dark palette must be the
approved one (dark folders, without an outline), not inverted.

### beos-tabs
Steps: `powertheme --apply="Reactionary ME"`, `--beos-tabs=on`, open a window and
take a screenshot; `--beos-tabs=off` and take another.
Then the same with "Expose", "Platinum", "Leopard", "Gershwix French Blue" and
"NeXT", shift-dragging the tab along the window.
PASS when: with the option on, each theme's title bar is a tab narrower than the
window with the close button on its left and the zoom button on its right, and no
minimize button, and it slides; off, a full-width title bar comes back. {p}-tabs.txt
records both measured widths for each.

### schedule
Steps: with a theme that has both versions in use, `powertheme --schedule=` with
the light time a minute or two away; wait past it; `--schedule=off`.
PASS when: {p}-schedule.txt shows turning it on printed one line, the process
isn't in the roster, the mode switched at the set time and the process has gone.
The Deskbar shows no entry for it in a screenshot.

### frame
Steps: "Reactionary ME", with Terminal in front of and overlapping StyledEdit;
screenshot; `--mode=dark`, `--mode=light`; screenshot again.
PASS when: {p}-frame.txt says "pixels changed by a round trip: 0", outside the
Deskbar. The back window's frame used to paint through the front one.

### window-size
Steps: in the Power Theme window, select "Reactionary ME" and click Apply, noting
the window's frame before and after.
PASS when: {p}-window.txt says "height changed by Apply: 0", and the whole window
is on a 1024x768 screen in a screenshot.

### labels
Steps: "Reactionary ME", restart Tracker, screenshot the desktop.
PASS when: {p}-labels.txt says "desktop labels white: yes", from the brightest
pixels under the first icon.

### dano
Steps: `powertheme --apply=Haiku --parts=decorator,controls`, `--apply=Dano`, start
StyledEdit, list its images; open a Tracker window and take a screenshot.
PASS when: {p}-dano.txt says "new application loads control look: Dano", and the
screenshot shows Dano's double scroll bar arrows and Haiku's window borders.

### tidy
Steps: `powertheme --apply=Haiku --parts=decorator,controls`, `--apply=Tidy`, start
StyledEdit, list its images and read `decorator_settings`; `--mode=dark`, `--status`;
`--mode=light`, `--status`; screenshots of a Tracker window in both.
PASS when: {p}-tidy.txt says "new application loads control look: Tidy" and "window
borders: Tidy", and shows the pair switching: "theme: Tidy Dark" with mode: dark, and
back with mode: light. The screenshots show the rounded tab with its title in the
middle, slim scroll bars, and in the dark one, marks that can be seen in check boxes.
Then `--apply="Tidy Square"`, read `decorator_settings`, `--mode=dark`, `--status`,
`--mode=light`: it also says "square window borders: TidySquare" and "square dark:
... theme: Tidy Square Dark", and its screenshot shows every corner of the tab and
frame square, and otherwise the same window as Tidy's. Then `--apply="Tidy Renku"`,
`--status`, `--mode=light`, `--status`: it says "renku dark: ... theme: Tidy Renku" and
"renku light: ... theme: Tidy Renku Light", and its screenshots show a fire orange tab,
text that can be read in both, and the Renku wallpapers. Then `--apply="Tidy Renku Square"`,
read `decorator_settings`, `--status`, `--mode=light`, `--status`: it says "renku square
window borders: TidySquare", "renku square dark: ... theme: Tidy Renku Square" and "renku
square light: ... theme: Tidy Renku Square Light".

### expose
Steps: `powertheme --apply=Haiku --parts=decorator,controls`, `--apply=Expose`,
read `decorator_settings`; start StyledEdit and list its images; `--mode=dark`, `--status`;
`--mode=light`, `--status`; screenshots of a Tracker window in both.
PASS when: {p}-expose.txt says "window borders: Expose" and "new application loads
control look: Expose", and shows the pair switching: "theme: Expose Dark" with mode:
dark, and back with mode: light. The screenshots show the dark split title bar with
its title on the left, buttons on the right and a red close button; dark glass
controls with slate selections in the dark one, and soft grey buttons, orange-topped
tabs and a round slider thumb in the light one.

### platinum
Steps: `powertheme --apply=Haiku --parts=decorator,controls`, `--apply=Platinum`,
start StyledEdit, list its images and read `decorator_settings`; screenshot of a
Tracker window and an inactive window.
PASS when: {p}-platinum.txt says "window borders: Platinum" and "new application
loads control look: Platinum". The screenshots show a wide bevelled frame with a
striped title bar, the close button on the left and two on the right, lavender
scroll bars, an inactive window flat with no stripes or buttons, and lilac folders.
Files with different types show different icons from NineIcons Redux.

### leopard
Steps: `powertheme --apply=Haiku --parts=decorator,controls`, `--apply=Leopard`,
start StyledEdit, list its images and read `decorator_settings`; screenshot of the
controls demo with a menu open, a Tracker window of different file types and an
inactive window. Then `--apply="Leopard Graphite"` and the same screenshot.
PASS when: {p}-leopard.txt says "window borders: Leopard", "new application loads
control look: Leopard" and "graphite window borders: Leopard". The screenshots show a
grey title bar with rounded top corners, red, yellow and green gel buttons at its
left and the title in the middle; white pill buttons with a blue default one, blue
gel pop-up ends, check boxes and scroll bar thumbs, and a blue menu selection; an
inactive window lighter with grey buttons; and elementary's blue-grey folders.
Under Graphite all that blue is steel blue, the window buttons too.

### gershwix
Steps: `powertheme --apply=Haiku --parts=decorator,controls`, `--apply="Gershwix
French Blue"`, start StyledEdit, list its images and read `decorator_settings`;
screenshot of the controls demo on its Media tab with a menu open and an inactive
Tracker window. Then apply each of the other six Gershwix themes and take a
screenshot of a small active Tracker window, so that its scroll bars have thumbs.
PASS when: {p}-gershwix.txt says "window borders: Gershwix" and "new application
loads control look: Gershwix", and "Gershwix themes listed: 7". The screenshots show
a light grey striped title bar with three square boxes at its left (minimize and
zoom with their marks) and the title in the middle, a dark line round the window,
light bevelled buttons with a dark ring round the default one, a ticked check box,
a progress bar shaded like a tube and ridged scroll and slider thumbs in French
blue, and NineIcons Redux; an inactive window's boxes flat grey. Each other theme
has its own colour in the thumbs: green, orange, red, purple, yellow and grey.

### next
Steps: `powertheme --apply=Haiku --parts=decorator,controls`, `--apply=NeXT`, start
StyledEdit, list its images and read `decorator_settings`; screenshot of the
controls demo on its Controls tab and on its Media tab with a menu open, an
inactive Tracker window, and a close box, a button and a check box held down.
PASS when: {p}-next.txt says "window borders: NeXT" and "new application loads
control look: NeXT". The screenshots show a black title bar with a light top and
left edge, three bevelled grey boxes at its left (a cross, a triangle in the top
left and a window) and the title in white in the middle; an inactive window's bar
grey with darker boxes and a black title; square grey buttons with a black line
round the default one, pop-up menus with a small raised bar at their right; a
ticked check box; stippled scroll bar tracks with dimpled thumbs; a stippled
slider groove with a square knob; a black progress bar in a stippled track; a
white menu selection; tan leather folders tied with a cord (a home badge on the
home folder) and a black display for the computer; and held down, the box, button
and check box are white with black along their top and left.

### system7
Steps: `powertheme --apply=Haiku --parts=decorator,controls`, `--apply="System 7"`,
start StyledEdit, list its images and read `decorator_settings`; screenshot of the
controls demo on both tabs with a menu open, and a Tracker window behind it.
PASS when: {p}-system7.txt says "window borders: System7" and "new application
loads control look: System7". The screenshots show a light grey title bar striped
every other row with a close box at its left, a zoom box at its right and the title
in the middle, a black frame with a drop shadow, an inactive window's bar plain
white with a grey title; white rounded buttons with a thick ring round the default
one, pop-ups with a shadow and a black triangle, check boxes with a cross, radio
buttons with a dot, hollow scroll arrows and a lavender thumb, a white menu with a
black outline, and the black and white dotted desktop.

### amiga
Steps: `powertheme --apply=Haiku --parts=decorator,controls`, `--apply=Amiga`,
start StyledEdit, list its images and read `decorator_settings`; read the
subpixel smoothing setting; screenshot of the controls demo with a menu open, a
Tracker window and the Deskbar; `--restart-tracker` and screenshot the Deskbar;
`--apply="System 7"` and read the smoothing setting again.
PASS when: {p}-amiga.txt says "window borders: Amiga", "new application loads
control look: Amiga", "subpixel smoothing with Amiga: off" and "subpixel smoothing
after System 7: on" (when it was on before). The screenshots show white title bars
with blue stripes, the close gadget at the left and two depth gadgets at the
right; all text in Topaz, its pixels sharp without coloured edges; orange and blue
buttons, pixel check boxes and radio buttons, blue lists with white text, drawers
for folders, and Deskbar labels that don't overlap after Tracker restarts.

### amigaos3
Steps: `powertheme --apply=Haiku --parts=decorator,controls`, `--apply="AmigaOS
3"`, start StyledEdit, list its images and read `decorator_settings`; screenshot of
the controls demo on both tabs with a menu open and a Tracker window behind it.
PASS when: {p}-amigaos3.txt says "window borders: AmigaOS3" and "new application
loads control look: AmigaOS3". The screenshots show a blue title bar with the close
gadget at its left and the zoom and depth gadgets at its right, the title in white
Topaz, an inactive window's bar grey; bevelled grey buttons, the GTK theme's check
boxes and radio buttons, a dotted scroll bar track with a raised thumb, a blue
progress bar and menu selection, and the resize grip where the scroll bars meet.

### flat-tab
Steps: build `tools/left-tab-test.cpp`, `powertheme --apply=DarkFlat`, start it,
take a screenshot and run `tools/left-tab-check.py` on it.
PASS when: {p}-flat.txt says "rows with lines across the left-titled tab: 0", and the
tab is one smooth gradient in the screenshot. The end of the gradient used to show
stripes of unrelated colours. It also says "decorators with rounding code: 0", from
the add-ons built here: this system has no compositor to round them.

### deskbar
Steps: go through "Haiku", "Tidy", "Platinum", "Expose Dark", "Reactionary ME",
"Tidy Square", "Haiku" and `--revert`, noting the Deskbar's team and how long each
apply takes, with a screenshot of the Deskbar after each.
PASS when: the Deskbar is running after every step, with a new team where the tab's
height changed and it is on the side of the screen, and the same team while it is
along the bottom. Its logo is whole in every screenshot, and no apply takes over
10 seconds because of it.

### revert
Steps: note every icon attribute in `mime_db` and the colours before any theme;
apply themes as above; `powertheme --revert`; compare.
PASS when: {p}-revert.txt says "differences from before any theme: 0".
