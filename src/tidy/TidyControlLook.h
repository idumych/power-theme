/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Control look after the "Tidy" mockups by anon40228759 on Haiku's forum
 * ("UI adjustments that stay out of the way", 2025): Haiku's own controls, with
 * softer menu bars, pale column headers, slim scroll bars with a capsule for a
 * thumb and no bezels, blank when there is nothing to scroll, plainer tabs and
 * a shaded status bar. It follows the system colours, so one look draws the
 * light and the dark version.
 */
#ifndef TIDY_CONTROL_LOOK_H
#define TIDY_CONTROL_LOOK_H


#include "ForwardingControlLook.h"


namespace BPrivate {


class TidyControlLook : public ForwardingControlLook {
public:
								TidyControlLook();
	virtual						~TidyControlLook();

	virtual	void				DrawMenuBarBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawBorder(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									border_style borderStyle, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	using ForwardingControlLook::DrawButtonBackground;
	virtual	void				DrawButtonBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);

	virtual	void				DrawStatusBar(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& barColor,
									float progressPosition);

	virtual	void				DrawScrollViewFrame(BView* view,
									BRect& rect, const BRect& updateRect,
									BRect verticalScrollBarFrame,
									BRect horizontalScrollBarFrame,
									const rgb_color& base,
									border_style borderStyle,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);
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
			void				_DrawArrow(BView* view, BRect rect,
									int32 direction, const rgb_color& line,
									const rgb_color& fill);
};


}	// namespace BPrivate


#endif	// TIDY_CONTROL_LOOK_H
