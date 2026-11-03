#!/bin/sh
# Remove Reactionary as install.sh put it in: put the previous look and
# icons back, then delete the decorators, the control looks, Themes, the
# Reactionary ME, Haiku and Gonx themes and the ClassicOS 2000 icons from
# ~/config/non-packaged.

nonpackaged=$(finddir B_USER_NONPACKAGED_DIRECTORY)
addons=$(finddir B_USER_NONPACKAGED_ADDONS_DIRECTORY)
bin=$(finddir B_USER_NONPACKAGED_BIN_DIRECTORY)
data=$(finddir B_USER_NONPACKAGED_DATA_DIRECTORY)
apps=$nonpackaged/apps
settings=$(finddir B_USER_SETTINGS_DIRECTORY)

if [ -x "$apps/Themes" ]; then
	"$apps/Themes" --revert || true
	"$apps/Themes" --restart-tracker || true
fi
if setdecor -l 2>/dev/null | grep -q -E '^\*(Reactionary|FlatDecorator|Smoke)'; then
	setdecor Default >/dev/null
fi

rm -f "$addons/decorators/Reactionary" "$addons/control_look/Reactionary" \
	"$addons/decorators/FlatDecorator" "$addons/control_look/FlatControlLook" \
	"$addons/decorators/Smoke" "$addons/control_look/Smoke" \
	"$apps/Themes" "$bin/Themes" "$bin/reactionary-theme" \
	"$settings/deskbar/menu/Applications/Themes"
rm -rf "$data/themes/Reactionary ME" "$data/themes/Haiku" "$data/themes/Gonx" \
	"$data/themes/DarkFlat" "$data/themes/LightFlat" "$data/themes/MoonFlat" \
	"$data/themes/OrangeFlat" "$data/themes/Renku" \
	"$data/icons/ClassicOS2000" \
	"$data/Themes" "$settings/Themes"

echo "Reactionary is removed. Restart running applications to get your"
echo "previous controls and fonts back everywhere."
