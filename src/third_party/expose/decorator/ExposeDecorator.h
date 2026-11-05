/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the GNU General Public License, version 3 or
 * later (see ../LICENSE). Its structure comes from Gonx's decorator, under the
 * MIT licence.
 *
 * Window decorator after Expose Dark, a KDE window decoration by Mark Whittaker:
 * a dark glossy title bar across the whole window, split into a bright upper half
 * and a darker lower one, its title on the left and glass buttons on the right
 * with a red close button, and a thin frame with a pale line inside its outline.
 * The inactive bar has no split, and gets lighter towards the bottom.
 *
 * With Power Theme's BeOS-style tabs on, the bar is a tab as wide as its title on
 * top of a frame with a top edge, with close on the left and zoom on the right.
 *
 * It derives from Decorator rather than TabDecorator, which keeps moving the
 * buttons back to the tab's ends. Its colours are the artwork's own, whatever the
 * system's colours are; only the title's follow them.
 *
 * Without compositing, its soft shadow is left out and its corners are cut out
 * pixel by pixel rather than anti-aliased.
 */
#ifndef EXPOSE_DECORATOR_H
#define EXPOSE_DECORATOR_H


#include "Decorator.h"
#include "DecorManager.h"


class Desktop;


class ExposeDecorAddOn : public DecorAddOn {
public:
								ExposeDecorAddOn(image_id id, const char* name);

protected:
	virtual	Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, Desktop* desktop);

private:
			bool				fBeOSTabs;
};


class ExposeDecorator : public Decorator {
public:
								ExposeDecorator(DesktopSettings& settings,
									BRect frame, Desktop* desktop,
									bool beosTabs);
	virtual						~ExposeDecorator();

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
			bool				_TopLeftIsRound() const;
			bool				_TopRightIsRound() const;
			float				_TabWidth(const Decorator::Tab* tab) const;
			bool				_IsSmall() const;
			float				_Scale() const;
			float				_TitleHeight() const;
			void				_LayoutTabs();
			bool				_HasResizeCorner() const;
			BRect				_OuterRect() const;

			rgb_color			_TitleColorAt(bool focus, float row,
									float height) const;
			void				_ExcludeCorner(BRegion* region, float x,
									float y, int32 xDirection,
									int32 yDirection) const;
			void				_DrawCorner(float x, float y, int32 xDirection,
									int32 yDirection, const rgb_color& line);
			void				_DrawResizeCorner();
			void				_DrawButton(Decorator::Tab* tab, bool direct,
									BRect rect, const uint32* large,
									const uint32* small);

			bool				fBeOSTabs;

			rgb_color			fFocusTextColor;
			rgb_color			fNonFocusTextColor;
};


#endif	// EXPOSE_DECORATOR_H
