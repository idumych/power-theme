/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the GNU General Public License, version 3 or
 * later (see ../LICENSE).
 *
 * Control look after two Kvantum themes by Mark Whittaker. The system's panel
 * colour says which it draws, so one look follows a theme's light and dark mode.
 *
 * Expose Dark, on light panels: soft grey buttons with a white rim that turn amber
 * by default, white fields with a small arrow box, tabs with an orange top, shaded
 * scroll bars and a round slider thumb.
 *
 * Expose Air Dark, on dark panels: dark glass buttons split into a bright upper
 * half and a darker lower one, that turn slate when pressed, a black glossy menu
 * bar, slate selections, shaded scroll bars, thin sliders and flat tabs.
 */
#ifndef EXPOSE_CONTROL_LOOK_H
#define EXPOSE_CONTROL_LOOK_H


#include "ForwardingControlLook.h"


class BBitmap;


namespace BPrivate {


class ExposeControlLook : public ForwardingControlLook {
public:
								ExposeControlLook();
	virtual						~ExposeControlLook();

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
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);

	using ForwardingControlLook::DrawButtonWithPopUpBackground;
	virtual	void				DrawButtonWithPopUpBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);

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

	virtual	void				DrawMenuBackground(BView* view,
									BRect& rect, const BRect& updateRect,
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
									const rgb_color& base,
									uint32 flags = 0);
	virtual	void				DrawRadioButton(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0);

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

	using ForwardingControlLook::DrawSliderBar;
	virtual	void				DrawSliderBar(BView* view, BRect rect,
									const BRect& updateRect,
									const rgb_color& base, rgb_color fillColor,
									uint32 flags, orientation orientation);
	virtual	void				DrawSliderThumb(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
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

private:
			void				_DrawMark(BView* view, const BRect& rect,
									const uint32* pixels, int32 index,
									bool disabled);

			BBitmap*			fMarks[44];
};


}	// namespace BPrivate


#endif	// EXPOSE_CONTROL_LOOK_H
