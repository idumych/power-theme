/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Window decorator in the style of QNX 6's Photon microGUI: a blue title bar
 * shaded from top to bottom, a light line along its top and left and a dark
 * one along its bottom, a window menu box sunk into its left end, the title in
 * the middle in dark blue, and at its right end a grey panel with minimize,
 * maximize and close boxes; a grey frame six pixels wide round the window, a
 * raised ridge between black and dark grey lines outside and a dark line
 * inside, with a blue square in the bottom right corner of a resizable window.
 * An inactive window's bar is a pale greyish blue. The bar's shades are worked
 * out from the window tab colour, as Photon's colour schemes changed them.
 *
 * With Power Theme's BeOS-style tabs on, the bar is a tab as wide as its title,
 * on top of the frame.
 *
 * Drawn from scratch after screenshots of QNX 6.2.1; nothing of QNX's is
 * used. Its structure follows Power Theme's System 7 decorator.
 */
#ifndef QNX_DECORATOR_H
#define QNX_DECORATOR_H


#include "Decorator.h"
#include "DecorManager.h"


class Desktop;


class QNXDecorAddOn : public DecorAddOn {
public:
								QNXDecorAddOn(image_id id, const char* name);

protected:
	virtual	Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, Desktop* desktop);

private:
			bool				fBeOSTabs;
};


class QNXDecorator : public Decorator {
public:
								QNXDecorator(DesktopSettings& settings,
									BRect frame, Desktop* desktop,
									bool beosTabs);
	virtual						~QNXDecorator();

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
			float				_GadgetsWidth(const Decorator::Tab* tab) const;
			void				_DrawMenuBox(const Decorator::Tab* tab,
									rgb_color face);
			void				_DrawGadgets(Decorator::Tab* tab,
									const BRect& invalid);
			void				_DrawResizeCorner();
			void				_DrawBox(Decorator::Tab* tab, bool direct,
									BRect rect, bool pressed, glyph glyph);

			bool				fBeOSTabs;

			rgb_color			fFocusTextColor;
			rgb_color			fNonFocusTextColor;
			rgb_color			fFocusFrameColor;
			rgb_color			fNonFocusFrameColor;
			rgb_color			fFocusTabColor;
			rgb_color			fNonFocusTabColor;
};


#endif	// QNX_DECORATOR_H
