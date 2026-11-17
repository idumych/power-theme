/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Control look after Hideaki Kamada's Notebook skin for WindowBlinds (1999),
 * drawn from its pictures (NotebookControlArt.h, from ../art): buttons outlined
 * in pencil, blue while pressed, check boxes ticked in red, radio buttons with a
 * blue dot, and a menu bar on paper ruled in navy. The rest is the default look
 * in the theme's colours.
 */
#ifndef NOTEBOOK_CONTROL_LOOK_H
#define NOTEBOOK_CONTROL_LOOK_H


#include "ForwardingControlLook.h"


namespace BPrivate {


class NotebookControlLook : public ForwardingControlLook {
public:
								NotebookControlLook();
	virtual						~NotebookControlLook();

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

	virtual	void				DrawMenuBarBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawCheckBox(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0);
	virtual	void				DrawRadioButton(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0);
};


}	// namespace BPrivate


#endif	// NOTEBOOK_CONTROL_LOOK_H
