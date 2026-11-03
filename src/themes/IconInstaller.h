/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Puts an icon theme's icons on Haiku's file types and folders, following
 * the icon map (data/Themes/icon-map), and takes them off again.
 *
 * File type icons are set through BMimeType, in the user's MIME database,
 * so Tracker shows them at once; folder icons are the folders' BEOS:ICON
 * attributes. What each one was before is kept in
 * ~/config/settings/Themes/icons the first time it is changed, so that
 * "revert" puts back Haiku's own icons whatever themes came in between.
 */
#ifndef ICON_INSTALLER_H
#define ICON_INSTALLER_H


#include <String.h>

#include <functional>


class IconTheme;


typedef std::function<void(float progress, const char* what)> ProgressHook;


// Sets \a theme's icons. Targets it has no icon for get back what they
// had before. Returns how many were set in \a changed.
status_t	apply_icon_theme(const IconTheme& theme, BString& log,
				int32& changed, const ProgressHook& progress = ProgressHook());

// Puts back every icon changed by any icon theme.
status_t	revert_icons(BString& log);

bool		has_saved_icons();


#endif	// ICON_INSTALLER_H
