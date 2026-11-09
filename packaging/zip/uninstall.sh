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
if setdecor -l 2>/dev/null | grep -q -E '^\*(Reactionary|FlatDecorator|Smoke|Tidy|TidySquare|Expose|Platinum|Leopard|Gershwix|NeXT|System7|Amiga|AmigaOS3|QNX)'; then
	setdecor Default >/dev/null
fi

rm -f "$addons/decorators/Reactionary" "$addons/control_look/Reactionary" \
	"$addons/decorators/FlatDecorator" "$addons/control_look/FlatControlLook" \
	"$addons/decorators/Smoke" "$addons/control_look/Smoke" \
	"$addons/control_look/Dano" \
	"$addons/decorators/Tidy" "$addons/decorators/TidySquare" \
	"$addons/control_look/Tidy" \
	"$addons/decorators/Expose" "$addons/control_look/Expose" \
	"$addons/decorators/Platinum" "$addons/control_look/Platinum" \
	"$addons/decorators/Leopard" "$addons/control_look/Leopard" \
	"$addons/decorators/Gershwix" "$addons/control_look/Gershwix" \
	"$addons/decorators/NeXT" "$addons/control_look/NeXT" \
	"$addons/decorators/System7" "$addons/control_look/System7" \
	"$addons/decorators/QNX" "$addons/control_look/QNX" \
	"$addons/decorators/Amiga" "$addons/control_look/Amiga" \
	"$addons/decorators/AmigaOS3" "$addons/control_look/AmigaOS3" \
	"$data/fonts/ttfonts/HOTDOGTopaz.ttf" "$data/fonts/ttfonts/HOTDOGTopaz-Bold.ttf" \
	"$apps/PowerTheme" "$apps/PowerShelf" "$bin/powertheme" \
	"$settings/launch/powertheme-shelf" \
	"$settings/deskbar/menu/Applications/Power Theme" \
	"$apps/Themes" "$bin/Themes" "$bin/reactionary-theme" \
	"$settings/deskbar/menu/Applications/Themes"
rm -rf "$data/themes/Reactionary ME" "$data/themes/Haiku" "$data/themes/Gonx" \
	"$data/themes/DarkFlat" "$data/themes/LightFlat" "$data/themes/MoonFlat" \
	"$data/themes/OrangeFlat" "$data/themes/Renku" "$data/themes/Renku Light" \
	"$data/themes/Haiku Dark" "$data/themes/Reactionary ME Dark" \
	"$data/themes/Expose" "$data/themes/Expose Dark" "$data/themes/Platinum" \
	"$data/themes/Dano" "$data/themes/Tidy" "$data/themes/Tidy Dark" \
	"$data/themes/Tidy Square" "$data/themes/Tidy Square Dark" \
	"$data/themes/Tidy Renku" "$data/themes/Tidy Renku Light" \
	"$data/themes/Tidy Renku Square" "$data/themes/Tidy Renku Square Light" \
	"$data/icons/ClassicOS2000" "$data/icons/breeze-dark" \
	"$data/icons/NineIconsRedux" "$data/icons/elementary" "$data/icons/NeXT" \
	"$data/themes/Leopard" "$data/themes/Leopard Graphite" \
	"$data/themes/Gershwix Ivy" "$data/themes/Gershwix French Blue" \
	"$data/themes/Gershwix Poppy" "$data/themes/Gershwix Crimson" \
	"$data/themes/Gershwix Plum" "$data/themes/Gershwix Sunny" \
	"$data/themes/Gershwix Silver" "$data/themes/NeXT" \
	"$data/themes/System 7" "$data/themes/Amiga" "$data/icons/Workbench" \
	"$data/themes/AmigaOS 3" "$data/themes/QNX" \
	"$data/PowerTheme" "$settings/PowerTheme" "$data/Themes" "$settings/Themes"

echo "Power Theme is removed. Restart running applications to get your"
echo "previous controls and fonts back everywhere."
