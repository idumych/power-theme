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
approved one (dark folders with a thin soft-white outline), not inverted.

### beos-tabs
Steps: `powertheme --apply="Reactionary ME"`, `--beos-tabs=on`, open a window and
take a screenshot; `--beos-tabs=off` and take another.
PASS when: with the option on, the title bar is a tab narrower than the window
with the close button on its left and the zoom button on its right, and no
minimize button; off, a full-width title bar comes back. {p}-tabs.txt records
both measured widths.

### revert
Steps: note every icon attribute in `mime_db` and the colours before any theme;
apply themes as above; `powertheme --revert`; compare.
PASS when: {p}-revert.txt says "differences from before any theme: 0".
