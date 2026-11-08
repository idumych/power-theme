/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Window decorator in the style of Swizcore Studio's NeXT, a ShapeShifter theme
 * for Mac OS X after NeXTSTEP: a black title bar across the whole window, with a
 * light edge along its top and left and a grey one along its bottom and right,
 * three bevelled grey boxes at its left end (close, minimize and zoom) and its
 * title in white in the middle. The frame is a black line round the window. An
 * inactive window's bar is grey, its boxes darker and its title black.
 *
 * With Power Theme's BeOS-style tabs on, the bar is a tab as wide as its title, on
 * top of the frame, with close on the left and zoom on the right.
 *
 * Drawn from scratch after the theme's pictures; none of its artwork is used. It
 * derives from Decorator rather than TabDecorator, which keeps moving the buttons
 * back to the tab's ends, and its structure follows Power Theme's Gershwix
 * decorator.
 */
#ifndef NEXT_DECORATOR_H
#define NEXT_DECORATOR_H


#include "Decorator.h"
#include "DecorManager.h"


class Desktop;


class NextDecorAddOn : public DecorAddOn {
public:
								NextDecorAddOn(image_id id, const char* name);

protected:
	virtual	Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, Desktop* desktop);

private:
			bool				fBeOSTabs;
};


class NextDecorator : public Decorator {
public:
								NextDecorator(DesktopSettings& settings,
									BRect frame, Desktop* desktop,
									bool beosTabs);
	virtual						~NextDecorator();

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


#endif	// NEXT_DECORATOR_H
