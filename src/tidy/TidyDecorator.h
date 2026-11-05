/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Window decorator after the "Tidy" mockups by anon40228759 on Haiku's forum
 * ("UI adjustments that stay out of the way", 2025): a sliding tab with rounded
 * top corners and its title in the middle, a thin frame with rounded corners
 * that is little more than an outline along the top, so that a menu bar seems
 * to run right up to it, and a grip of six soft dots on document windows. It
 * follows the window
 * colours, so one decorator draws the light and the dark version.
 *
 * Without compositing, the corners are cut out of the window pixel by pixel
 * rather than anti-aliased.
 */
#ifndef TIDY_DECORATOR_H
#define TIDY_DECORATOR_H


#include "TabDecorator.h"
#include "DecorManager.h"


class Desktop;


class TidyDecorAddOn : public DecorAddOn {
public:
								TidyDecorAddOn(image_id id, const char* name);

protected:
	virtual	Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, Desktop* desktop);
};


class TidyDecorator : public TabDecorator {
public:
								TidyDecorator(DesktopSettings& settings,
									BRect frame, Desktop* desktop);
	virtual						~TidyDecorator();

protected:
	virtual	void				_DoLayout();
	virtual	void				_DoTabLayout();

	virtual	void				_DrawFrame(BRect rect);
	virtual	void				_DrawTab(Decorator::Tab* tab, BRect rect);
	virtual	void				_DrawTitle(Decorator::Tab* tab, BRect rect);
	virtual	void				_DrawClose(Decorator::Tab* tab, bool direct,
									BRect rect);
	virtual	void				_DrawZoom(Decorator::Tab* tab, bool direct,
									BRect rect);
	virtual	void				_DrawMinimize(Decorator::Tab* tab, bool direct,
									BRect rect);

	virtual	void				_GetButtonSizeAndOffset(const BRect& tabRect,
									float* offset, float* size,
									float* inset) const;

	virtual	void				_GetFootprint(BRegion* region);

	virtual	void				_FontsChanged(DesktopSettings& settings,
									BRegion* updateRegion = NULL);

private:
			void				_UpdateMenuBarHeight(
									const DesktopSettings& settings);
			bool				_HasThinTop() const;
			float				_Scale() const;
			float				_TabHeight() const;
			bool				_TabFocus(Decorator::Tab* tab) const;
			rgb_color			_Outline(bool focus) const;

			void				_ExcludeCorner(BRegion* region, float x,
									float y, int32 xDirection,
									int32 yDirection) const;
			void				_StrokeCorner(float x, float y,
									int32 xDirection, int32 yDirection,
									const rgb_color& color);
			bool				_TopLeftIsRound() const;
			bool				_TopRightIsRound() const;

			void				_DrawTabBody(Decorator::Tab* tab);
			void				_FillTabShading(Decorator::Tab* tab,
									BRect rect);
			void				_DrawResizeGrip(BRect invalid);
			void				_DrawButtonBox(BRect box,
									const rgb_color& base, bool pressed);

			float				fMenuBarHeight;
};


#endif	// TIDY_DECORATOR_H
