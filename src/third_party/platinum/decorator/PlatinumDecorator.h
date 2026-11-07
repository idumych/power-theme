/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the GNU General Public License, version 3 or
 * later (see ../LICENSE). Its structure comes from Gonx's decorator, under the
 * MIT licence.
 *
 * Window decorator after Niels's Platinum window decoration for KDE, itself after
 * Mac OS 9: a wide bevelled grey frame with a recess round the content and a hard
 * shadow along the right and bottom, a title bar across the whole window with
 * stripes either side of a centred title, the close button on the left, and zoom
 * and minimize on the right. An inactive window is flat, with no stripes or
 * buttons.
 *
 * With Power Theme's BeOS-style tabs on, the bar is a tab as wide as its title and
 * a little of its stripes, on top of a frame with a top edge, with close on the
 * left and zoom on the right.
 *
 * It derives from Decorator rather than TabDecorator, which keeps moving the
 * buttons back to the tab's ends. Its colours are the artwork's own, whatever the
 * system's colours are; only the title's follow them.
 */
#ifndef PLATINUM_DECORATOR_H
#define PLATINUM_DECORATOR_H


#include "Decorator.h"
#include "DecorManager.h"

// Only a composited app_server has this; without it the corners stay square.
#if __has_include("RoundedDecorator.h")
#	include "RoundedDecorator.h"
#	define PLATINUM_ROUNDED_CORNERS
#endif


class Desktop;


class PlatinumDecorAddOn : public DecorAddOn {
public:
								PlatinumDecorAddOn(image_id id, const char* name);

protected:
	virtual	Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, Desktop* desktop);

private:
			bool				fBeOSTabs;
};


class PlatinumDecorator : public Decorator
#ifdef PLATINUM_ROUNDED_CORNERS
	, public RoundedDecorator
#endif
{
public:
#ifdef PLATINUM_ROUNDED_CORNERS
	virtual	void				GetRoundedShape(
									std::vector<ShapeRect>& shape) const;
#endif

								PlatinumDecorator(DesktopSettings& settings,
									BRect frame, Desktop* desktop,
									bool beosTabs);
	virtual						~PlatinumDecorator();

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
#ifdef PLATINUM_ROUNDED_CORNERS
			void				_StrokeRoundedFrame();
			void				_StrokeRoundedTab(Decorator::Tab* tab);
#endif
			bool				_HasTitle() const;
			bool				_HasFullTitleBar() const;
			bool				_TopRightIsRound() const;
			float				_TitleSpace(float height) const;
			float				_TabWidth(const Decorator::Tab* tab) const;
			bool				_IsSmall() const;
			float				_Scale() const;
			float				_TitleHeight() const;
			void				_LayoutTabs();
			bool				_HasResizeCorner() const;

			void				_DrawStripes(float left, float right,
									const BRect& tabRect);
			void				_DrawResizeCorner();
			void				_DrawButton(Decorator::Tab* tab, bool direct,
									BRect rect, const uint32* large,
									const uint32* small);

			bool				fBeOSTabs;

			rgb_color			fFocusTextColor;
			rgb_color			fNonFocusTextColor;
};


#endif	// PLATINUM_DECORATOR_H
