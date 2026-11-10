/*
 * Copyright 2026, idumych.
 * Copyright 2026, Gonx contributors.
 * Distributed under the terms of the MIT License.
 *
 * Control look after BeOS R5.1 "Dano": white pill-cornered buttons with a one
 * pixel black outline and a soft two-step shadow, a default button on a blue
 * plate, menu fields with a blue arrow tab on the left, a tick and a bullet that
 * overflow their frames, a sunken grey slider, and swept tabs. Drawn from
 * screenshots of a running system, and based on the structure of Gonx's look.
 *
 * Every frame and background spends exactly as many pixels as Haiku's own look
 * does, so layouts are unchanged; only what is drawn in them differs.
 */
#ifndef DANO_CONTROL_LOOK_H
#define DANO_CONTROL_LOOK_H


#include "ForwardingControlLook.h"


class BMenuBar;


namespace BPrivate {


class DanoControlLook : public ForwardingControlLook {
public:
								DanoControlLook();
	virtual						~DanoControlLook();

	// buttons
	virtual	void				DrawButtonFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);
	virtual	void				DrawButtonFrame(BView* view, BRect& rect,
									const BRect& updateRect, float radius,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);
	virtual	void				DrawButtonFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawButtonBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);
	virtual	void				DrawButtonBackground(BView* view, BRect& rect,
									const BRect& updateRect, float radius,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);
	virtual	void				DrawButtonBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);

	virtual	void				DrawButtonWithPopUpBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);
	virtual	void				DrawButtonWithPopUpBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									float radius, const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);
	virtual	void				DrawButtonWithPopUpBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);

	// menu fields and menus
	virtual	void				DrawMenuFieldFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);
	virtual	void				DrawMenuFieldFrame(BView* view, BRect& rect,
									const BRect& updateRect, float radius,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);
	virtual	void				DrawMenuFieldFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawMenuFieldBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, bool popupIndicator,
									uint32 flags = 0);
	virtual	void				DrawMenuFieldBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									float radius, const rgb_color& base,
									bool popupIndicator, uint32 flags = 0);
	virtual	void				DrawMenuFieldBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									bool popupIndicator, uint32 flags = 0);
	virtual	void				DrawMenuFieldBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawMenuBarBackground(BView* view, BRect& rect,
									const BRect& updateRect,
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

	// text, check boxes, radio buttons, groups
	virtual	void				DrawTextControlBorder(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);
	virtual	void				DrawCheckBox(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0);
	virtual	void				DrawRadioButton(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0);
	virtual	void				DrawGroupFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 borders = B_ALL_BORDERS);

	// scroll bars
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

	virtual	void				DrawArrowShape(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 direction,
									uint32 flags = 0,
									float tint = B_DARKEN_MAX_TINT);

	// sliders
	virtual	rgb_color			SliderBarColor(const rgb_color& base);
	virtual	void				DrawSliderBar(BView* view, BRect rect,
									const BRect& updateRect,
									const rgb_color& base,
									rgb_color leftFillColor,
									rgb_color rightFillColor,
									float sliderScale, uint32 flags,
									orientation orientation);
	virtual	void				DrawSliderBar(BView* view, BRect rect,
									const BRect& updateRect,
									const rgb_color& base, rgb_color fillColor,
									uint32 flags, orientation orientation);
	virtual	void				DrawSliderThumb(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation);
	virtual	void				DrawSliderTriangle(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation);
	virtual	void				DrawSliderTriangle(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& fill, uint32 flags,
									orientation orientation);

	// tabs
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

	virtual	void				DrawStatusBar(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& barColor,
									float progressPosition);

	virtual	void				DrawSliderHashMarks(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, int32 count,
									hash_mark_location location,
									uint32 flags, orientation orientation);

	virtual	void				DrawBorder(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									border_style borderStyle, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	// a pressed button greys its label
	using ForwardingControlLook::DrawLabel;
	virtual	void				DrawLabel(BView* view, const char* label,
									const BBitmap* icon, BRect rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									const BAlignment& alignment,
									const rgb_color* textColor = NULL);

	virtual	void				DrawScrollViewFrame(BView* view,
									BRect& rect, const BRect& updateRect,
									BRect verticalScrollBarFrame,
									BRect horizontalScrollBarFrame,
									const rgb_color& base,
									border_style borderStyle,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

private:
			void				_DrawDanoButtonFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags, uint32 borders);
			void				_FitButtonHeight(BView* view, BRect& rect,
									uint32 flags, uint32 borders);
			BRect				_DrawMenuFieldBox(BView* view,
									BMenuBar* menuBar, BRect bar,
									const rgb_color& background,
									uint32 flags);
			void				_DrawFace(BView* view, BRect& rect,
									const rgb_color& base, uint32 flags,
									uint32 borders);
			void				_DrawTriangle(BView* view, BRect rect,
									uint32 direction, const rgb_color& outline,
									const rgb_color& fill);
			void				_DrawTab(BView* view, BRect& rect,
									const rgb_color& base, uint32 flags,
									bool active);
};


}	// namespace BPrivate


#endif	// DANO_CONTROL_LOOK_H
