#!/bin/sh
# Remove Power Theme as install.sh put it in: put the previous look and
# icons back, then delete the decorators, the control looks, Power Theme, the
# Reactionary ME, Haiku and Gonx themes and the ClassicOS 2000 icons from
# ~/config/non-packaged.

nonpackaged=$(finddir B_USER_NONPACKAGED_DIRECTORY)
addons=$(finddir B_USER_NONPACKAGED_ADDONS_DIRECTORY)
bin=$(finddir B_USER_NONPACKAGED_BIN_DIRECTORY)
data=$(finddir B_USER_NONPACKAGED_DATA_DIRECTORY)
apps=$nonpackaged/apps
settings=$(finddir B_USER_SETTINGS_DIRECTORY)

for app in "$apps/PowerTheme" "$apps/Themes"; do
	if [ -x "$app" ]; then
		"$app" --revert || true
		"$app" --restart-tracker || true
		break
	fi
done
if setdecor -l 2>/dev/null | grep -q -E '^\*(Reactionary|FlatDecorator|Smoke)'; then
	setdecor Default >/dev/null
fi

rm -f "$addons/decorators/Reactionary" "$addons/control_look/Reactionary" \
	"$addons/decorators/FlatDecorator" "$addons/control_look/FlatControlLook" \
	"$addons/decorators/Smoke" "$addons/control_look/Smoke" \
	"$apps/PowerTheme" "$bin/powertheme" \
	"$settings/deskbar/menu/Applications/Power Theme" \
	"$apps/Themes" "$bin/Themes" "$bin/reactionary-theme" \
	"$settings/deskbar/menu/Applications/Themes"
rm -rf "$data/themes/Reactionary ME" "$data/themes/Haiku" "$data/themes/Gonx" \
	"$data/themes/DarkFlat" "$data/themes/LightFlat" "$data/themes/MoonFlat" \
	"$data/themes/OrangeFlat" "$data/themes/Renku" "$data/themes/Renku Light" \
	"$data/themes/Haiku Dark" "$data/themes/Reactionary ME Dark" \
	"$data/icons/ClassicOS2000" "$data/icons/breeze-dark" \
	"$data/PowerTheme" "$settings/PowerTheme" "$data/Themes" "$settings/Themes"

echo "Power Theme is removed. Restart running applications to get your"
echo "previous controls and fonts back everywhere."
