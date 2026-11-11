/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the GNU General Public License, version 3 or
 * later (see ../LICENSE).
 *
 * Control look in the style of the Amiga's Workbench 1.3, after Arthur Choung's
 * HOTDOG: buttons with an orange outline round a blue one, black when pressed
 * with an orange label; check boxes with a cross and radio buttons with a dot;
 * pop-up menus framed like buttons; blue outlines round text fields, groups and
 * progress bars; scroll bars with a blue knob and hollow blue arrows in a white
 * track; and white menus with a blue edge.
 */
#ifndef AMIGA_CONTROL_LOOK_H
#define AMIGA_CONTROL_LOOK_H


#include "ForwardingControlLook.h"


namespace BPrivate {


class AmigaControlLook : public ForwardingControlLook {
public:
								AmigaControlLook();
	virtual						~AmigaControlLook();

	using ForwardingControlLook::DrawButtonFrame;
	virtual	void				DrawButtonFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	using ForwardingControlLook::DrawButtonBackground;
	virtual	void				DrawButtonBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);

	using ForwardingControlLook::DrawButtonWithPopUpBackground;
	virtual	void				DrawButtonWithPopUpBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);

	using ForwardingControlLook::DrawLabel;
	virtual	void				DrawLabel(BView* view, const char* label,
									const BBitmap* icon, BRect rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									const BAlignment& alignment,
									const rgb_color* textColor = NULL);

	virtual	void				DrawMenuBarBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	using ForwardingControlLook::DrawMenuFieldFrame;
	virtual	void				DrawMenuFieldFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	using ForwardingControlLook::DrawMenuFieldBackground;
	virtual	void				DrawMenuFieldBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, bool popupIndicator,
									uint32 flags = 0);
	virtual	void				DrawMenuFieldBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawMenuBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);
	virtual	void				DrawMenuItemBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawStatusBar(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& barColor,
									float progressPosition);

	virtual	void				DrawCheckBox(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0);
	virtual	void				DrawRadioButton(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0);

	virtual	void				DrawScrollBarBorder(BView* view,
									BRect rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation);
	virtual	void				DrawScrollBarButton(BView* view,
									BRect rect, const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& text, uint32 flags,
									int32 direction, orientation orientation,
									bool down = false);
	virtual	void				DrawScrollBarBackground(BView* view,
									BRect& rect1, BRect& rect2,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation);
	virtual	void				DrawScrollBarBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation);
	virtual	void				DrawScrollBarThumb(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation,
									uint32 knobStyle = B_KNOB_NONE);

	virtual	float				GetScrollBarWidth(
									orientation orientation = B_VERTICAL);

	using ForwardingControlLook::DrawSliderBar;
	virtual	void				DrawSliderBar(BView* view, BRect rect,
									const BRect& updateRect,
									const rgb_color& base,
									rgb_color fillColor, uint32 flags,
									orientation orientation);
	virtual	void				DrawSliderThumb(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation);
	using ForwardingControlLook::DrawSliderTriangle;
	virtual	void				DrawSliderTriangle(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& fill, uint32 flags,
									orientation orientation);

	virtual	void				DrawTextControlBorder(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawGroupFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawTabFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									border_style borderStyle = B_FANCY_BORDER,
									uint32 side = B_TOP_BORDER);
	virtual	void				DrawActiveTab(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									uint32 side = B_TOP_BORDER,
									int32 index = 0, int32 selected = -1,
									int32 first = 0, int32 last = 0);
	virtual	void				DrawInactiveTab(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									uint32 side = B_TOP_BORDER,
									int32 index = 0, int32 selected = -1,
									int32 first = 0, int32 last = 0);

};


}	// namespace BPrivate


#endif	// AMIGA_CONTROL_LOOK_H
