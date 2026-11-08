/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Window decorator in the style of the Amiga's Workbench 3.x, from the pictures
 * of lordwolfchild's public domain Amiga3.x theme for xfwm4: a bar in the
 * Workbench's blue (grey when inactive), a close gadget at its left, the title in
 * white after a groove, and zoom and depth gadgets at its right, the depth
 * gadget here minimizing; a bevelled frame four pixels wide whose bottom corners
 * are the resize handles.
 *
 * With Power Theme's BeOS-style tabs on, the bar is a tab as wide as its title, on
 * top of the frame.
 */
#ifndef AMIGAOS3_DECORATOR_H
#define AMIGAOS3_DECORATOR_H


#include "Decorator.h"
#include "DecorManager.h"


struct Art;
class Desktop;


class AmigaOS3DecorAddOn : public DecorAddOn {
public:
								AmigaOS3DecorAddOn(image_id id, const char* name);

protected:
	virtual	Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, Desktop* desktop);

private:
			bool				fBeOSTabs;
};


class AmigaOS3Decorator : public Decorator {
public:
								AmigaOS3Decorator(DesktopSettings& settings,
									BRect frame, Desktop* desktop,
									bool beosTabs);
	virtual						~AmigaOS3Decorator();

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
									const Art& art);
			void				_Draw(const Art& art, BPoint where,
									const BRect& clip);
			void				_Repeat(const Art& art, const BRect& area,
									bool across, const BRect& clip);

			bool				fBeOSTabs;

			rgb_color			fFocusTextColor;
			rgb_color			fNonFocusTextColor;
			rgb_color			fFocusFrameColor;
			rgb_color			fNonFocusFrameColor;
			rgb_color			fFocusTabColor;
			rgb_color			fNonFocusTabColor;
};


#endif	// AMIGAOS3_DECORATOR_H
