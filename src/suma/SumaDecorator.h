/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Window decorator in the style of the SuMa 1 GUI kit, a ShapeShifter theme for
 * Mac OS X after Susumu Yoshida's materials, as its metal windows looked: a dark
 * grey title bar with rounded top corners, three small dark beads in sunken
 * sockets at its left for close, minimize and zoom, and the title in white in the
 * middle. A bead fills with the accent of the kit's version whose accent the
 * highlight colour is (orange, blue or green) when pressed.
 *
 * With Power Theme's BeOS-style tabs on, the bar is a tab as wide as its title, on
 * top of the frame, with close on the left and zoom on the right.
 *
 * Drawn from scratch after the kit's pictures; none of its artwork is used. It is
 * Power Theme's Leopard decorator with SuMa's colours and beads.
 */
#ifndef SUMA_DECORATOR_H
#define SUMA_DECORATOR_H


#include "Decorator.h"
#include "DecorManager.h"

// Only a composited app_server has this; without it the corners are cut by hand.
#if __has_include("RoundedDecorator.h")
#	include "RoundedDecorator.h"
#	define LEOPARD_ROUNDED_CORNERS
#endif


class Desktop;


class SumaDecorAddOn : public DecorAddOn {
public:
								SumaDecorAddOn(image_id id, const char* name);

protected:
	virtual	Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, Desktop* desktop);

private:
			bool				fBeOSTabs;
};


class SumaDecorator : public Decorator
#ifdef LEOPARD_ROUNDED_CORNERS
	, public RoundedDecorator
#endif
{
public:
#ifdef LEOPARD_ROUNDED_CORNERS
	virtual	void				GetRoundedShape(
									std::vector<ShapeRect>& shape) const;
#endif

								SumaDecorator(DesktopSettings& settings,
									BRect frame, Desktop* desktop,
									bool beosTabs);
	virtual						~SumaDecorator();

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
#ifdef LEOPARD_ROUNDED_CORNERS
			void				_StrokeRoundedFrame();
#endif
			bool				_HasTitle() const;
			bool				_HasFullTitleBar() const;
			bool				_RoundedByCompositor() const;
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

			rgb_color			_BarColor(const Decorator::Tab* tab,
									float y) const;
			void				_ExcludeCorner(BRegion* region, float x,
									float y, int32 xDirection) const;
			void				_DrawResizeCorner();
			void				_DrawBead(Decorator::Tab* tab, bool direct,
									BRect rect, int32 which, bool pressed);
			void				_DrawButton(Decorator::Tab* tab, bool direct,
									BRect rect, const uint32* large,
									const uint32* small);
			void				_DrawDisabledSlots(Decorator::Tab* tab,
									const BRect& invalid);

			bool				fBeOSTabs;
			int32				fVersion;

			rgb_color			fFocusTextColor;
			rgb_color			fNonFocusTextColor;
};


#endif	// SUMA_DECORATOR_H
