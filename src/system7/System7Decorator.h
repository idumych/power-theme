/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Window decorator in the style of Mac OS 7.5 with its standard window colour:
 * a light grey title bar striped every other row, with a light lavender edge
 * along its top and left and a darker one along its bottom and right, a close
 * box at its left end, a zoom box at its right and the title in the middle; a
 * black line round the window with a drop shadow a pixel wide, and a grow box
 * of two overlapping squares on document windows. An inactive window's bar is
 * plain white, with a grey title and frame. There is no minimize box, as System
 * 7.5 had none; a double click on the bar minimizes.
 *
 * With Power Theme's BeOS-style tabs on, the bar is a tab as wide as its title, on
 * top of the frame, with close on the left and zoom on the right.
 *
 * Drawn from scratch after screenshots of Mac OS 7.5.5; nothing of Apple's is
 * used. Its structure follows Power Theme's NeXT decorator.
 */
#ifndef SYSTEM7_DECORATOR_H
#define SYSTEM7_DECORATOR_H


#include "Decorator.h"
#include "DecorManager.h"


class Desktop;


class System7DecorAddOn : public DecorAddOn {
public:
								System7DecorAddOn(image_id id, const char* name);

protected:
	virtual	Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, Desktop* desktop);

private:
			bool				fBeOSTabs;
};


class System7Decorator : public Decorator {
public:
								System7Decorator(DesktopSettings& settings,
									BRect frame, Desktop* desktop,
									bool beosTabs);
	virtual						~System7Decorator();

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
	enum glyph {
		GLYPH_NONE,
		GLYPH_CLOSE,
		GLYPH_MINIMIZE,
		GLYPH_ZOOM
	};

			bool				_HasTitle() const;
			bool				_HasFullTitleBar() const;
			float				_TabWidth(const Decorator::Tab* tab) const;
			bool				_IsSmall() const;
			float				_Scale() const;
			float				_TitleHeight() const;
			float				_ButtonSize() const;
			float				_ButtonEdge() const;
			float				_ButtonGap() const;
			BRect				_ButtonSlot(const Decorator::Tab* tab,
									int32 slot) const;
			void				_LayoutTabs();
			bool				_HasResizeCorner() const;
			float				_GlyphSpan() const;

			BRect				_TitleRect(const Decorator::Tab* tab) const;
			void				_GetShadow(BRegion* region) const;
			void				_DrawShadow(BRect invalid);
			void				_DrawSquare(BRect rect, rgb_color face);
			void				_DrawResizeCorner();
			void				_DrawBox(Decorator::Tab* tab, bool direct,
									BRect rect, bool pressed, glyph glyph,
									bool dim);

			bool				fBeOSTabs;

			rgb_color			fFocusTextColor;
			rgb_color			fNonFocusTextColor;
			rgb_color			fFocusFrameColor;
			rgb_color			fNonFocusFrameColor;
			rgb_color			fFocusTabColor;
			rgb_color			fNonFocusTabColor;
};


#endif	// SYSTEM7_DECORATOR_H
