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
Then the same with "Expose" and "Platinum", shift-dragging the tab along the window.
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
