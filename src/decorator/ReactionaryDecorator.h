/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Window decorator after the "Reactionary ME" KDE Plasma theme (Die4Ever,
 * store.kde.org/p/2331482) and its Windows ME Aurorae decoration: a four
 * pixel bevelled grey frame, a title bar shading from navy to light blue,
 * and square bevelled minimize, zoom and close buttons at its right end.
 */
#ifndef REACTIONARY_DECORATOR_H
#define REACTIONARY_DECORATOR_H


#include <Region.h>

#include "Decorator.h"
#include "DecorManager.h"


class Desktop;


class ReactionaryDecorAddOn : public DecorAddOn {
public:
								ReactionaryDecorAddOn(image_id id,
									const char* name);

protected:
	virtual	Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, Desktop* desktop);
};


class ReactionaryDecorator : public Decorator {
public:
	struct Geometry {
		float					border;
		float					titleHeight;
		float					buttonWidth;
		float					buttonHeight;
		float					buttonMargin;
			// between the buttons and the title bar's edges
		float					closeGap;
			// between the close button and the others
		float					titleInset;
		float					cornerLength;
			// how far a corner's resize area reaches along each border
	};

								ReactionaryDecorator(
									DesktopSettings& settings, BRect frame,
									Desktop* desktop);
	virtual						~ReactionaryDecorator();

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
			bool				_HasTab() const;
			bool				_IsSmall() const;
			bool				_HasResizeGrip() const;
			Geometry			_Geometry() const;
			void				_LayoutTabs(const Geometry& geometry);

			void				_DrawBevel(BRect rect, rgb_color topLeft,
									rgb_color bottomRight);
			void				_DrawButton(Decorator::Tab* tab, bool direct,
									BRect rect, bool pressed,
									const char* const* glyph);
			void				_DrawResizeGrip(BRect invalid);

private:
			rgb_color			fFocusTabColor;
			rgb_color			fFocusTextColor;
			rgb_color			fNonFocusTabColor;
			rgb_color			fNonFocusTextColor;
};


#endif	// REACTIONARY_DECORATOR_H
