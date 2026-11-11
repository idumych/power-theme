/*
 * Copyright 2026, idumych.
 * SPDX-License-Identifier: MIT
 *
 * The item margins of a menu, which BMenu keeps protected, reached through a
 * pointer to the inherited member instead of libbe's private MenuPrivate, so a
 * look built once doesn't need private headers.
 */
#ifndef MENU_MARGINS_H
#define MENU_MARGINS_H


#include <Menu.h>


class MenuMargins {
public:
	MenuMargins(BMenu* menu)
		:
		fMenu(menu)
	{
	}

	void GetItemMargins(float* left, float* top, float* right,
		float* bottom) const
	{
		(fMenu->*(&Access::GetItemMargins))(left, top, right, bottom);
	}

	void SetItemMargins(float left, float top, float right, float bottom)
	{
		(fMenu->*(&Access::SetItemMargins))(left, top, right, bottom);
	}

private:
	struct Access : BMenu {
		using BMenu::GetItemMargins;
		using BMenu::SetItemMargins;
	};

	BMenu* fMenu;
};


#endif	// MENU_MARGINS_H
