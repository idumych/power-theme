/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the GNU General Public License, version 3 or
 * later (see ../LICENSE).
 *
 * Window decorator in the style of the Amiga's Workbench 1.3, as Arthur Choung's
 * HOTDOG draws it: a white title bar with two blue stripes across its middle, a
 * close gadget at its left, the title in blue after it, and two depth gadgets at
 * its right, which here minimize and zoom; a white frame two pixels wide, and a
 * resize gadget at its bottom right. An inactive window's stripes are dotted
 * with white. A gadget held down turns black and orange. The gadgets' pixels and
 * the palette are HOTDOG's (objects/object-AmigaWindow.m, GPL 3 or later).
 *
 * With Power Theme's BeOS-style tabs on, the bar is a tab as wide as its title, on
 * top of the frame.
 */
#ifndef AMIGA_DECORATOR_H
#define AMIGA_DECORATOR_H


#include "Decorator.h"
#include "DecorManager.h"


class Desktop;


class AmigaDecorAddOn : public DecorAddOn {
public:
								AmigaDecorAddOn(image_id id, const char* name);

protected:
	virtual	Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, Desktop* desktop);

private:
			bool				fBeOSTabs;
};


class AmigaDecorator : public Decorator {
public:
								AmigaDecorator(DesktopSettings& settings,
									BRect frame, Desktop* desktop,
									bool beosTabs);
	virtual						~AmigaDecorator();

	virtual	void				Draw(BRect updateRect);
	virtual	void				Draw();

	virtual	Region				RegionAt(BPoint where, int32& tab) const;

	virtual	void				UpdateColors(DesktopSettings& settings);

protected:
	virtual	void				_DoLayout();
	virtual	void				_DoOutlineLayout();

	virtual	void				_DrawFrame(BRect rect);
	virtual	void				_DrawOutlineFrame(BRect rect);

	virtual	void				_DrawTab(Decorator::Tab* tab, BRect rect);
	virtual	void				_DrawTitle(Decorator::Tab* tab, BRect rect);
	virtual	void				_DrawButtons(Decorator::Tab* tab,
									const BRect& invalid);
	virtual	void				_DrawClose(Decorator::Tab* tab, bool direct,
									BRect rect);
	virtual	void				_DrawMinimize(Decorator::Tab* tab,
									bool direct, BRect rect);
	virtual	void				_DrawZoom(Decorator::Tab* tab, bool direct,
									BRect rect);

	virtual	void				_SetTitle(Decorator::Tab* tab,
									const char* string,
									BRegion* updateRegion = NULL);
	virtual	void				_SetFocus(Decorator::Tab* tab);
	virtual	bool				_SetTabLocation(Decorator::Tab* tab,
									float location, bool isShifting,
									BRegion* updateRegion = NULL);
	virtual	bool				_SetSettings(const BMessage& settings,
									BRegion* updateRegion = NULL);

	virtual	void				_UpdateFont(DesktopSettings& settings);

	virtual	void				_MoveBy(BPoint offset);
	virtual	void				_ResizeBy(BPoint offset, BRegion* dirty);

	virtual	bool				_AddTab(DesktopSettings& settings,
									int32 index = -1,
									BRegion* updateRegion = NULL);
	virtual	bool				_RemoveTab(int32 index,
									BRegion* updateRegion = NULL);
	virtual	bool				_MoveTab(int32 from, int32 to, bool isMoving,
									BRegion* updateRegion = NULL);

	virtual	void				_GetFootprint(BRegion* region);

private:
			bool				_HasTitle() const;
			bool				_HasFullTitleBar() const;
			float				_TabWidth(const Decorator::Tab* tab) const;
			bool				_IsSmall() const;
			float				_Scale() const;
			float				_TitleHeight() const;
			void				_LayoutTabs();
			bool				_HasResizeCorner() const;

			BRect				_TitleRect(const Decorator::Tab* tab) const;
			void				_DrawResizeCorner();
			void				_DrawGadget(bool direct, BRect rect,
									const char* const* rows, int32 width,
									int32 height, bool pressed);

			bool				fBeOSTabs;

			rgb_color			fFocusTextColor;
			rgb_color			fNonFocusTextColor;
			rgb_color			fFocusFrameColor;
			rgb_color			fNonFocusFrameColor;
			rgb_color			fFocusTabColor;
			rgb_color			fNonFocusTabColor;
};


#endif	// AMIGA_DECORATOR_H
