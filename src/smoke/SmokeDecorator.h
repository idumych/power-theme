/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Window decorator in the style of ZETA's "Smoke" decorator, redrawn from
 * screenshots: a sliding BeOS tab shading down from the window tab colour,
 * with a white cross at its left end and two overlapping squares at its
 * right, flowing into a four pixel frame (a black line, a light one, the tab
 * colour and a black line again). The bottom right corner of a resizable
 * window is lighter, and document windows have a grip with six dots there.
 * No ZETA code or artwork is used.
 */
#ifndef SMOKE_DECORATOR_H
#define SMOKE_DECORATOR_H


#include "TabDecorator.h"
#include "DecorManager.h"

// Only a composited app_server has this; without it the corners stay square.
#if __has_include("RoundedDecorator.h")
#	include "RoundedDecorator.h"
#	define SMOKE_ROUNDED_CORNERS
#endif


class Desktop;


class SmokeDecorAddOn : public DecorAddOn {
public:
								SmokeDecorAddOn(image_id id, const char* name);

protected:
	virtual	Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, Desktop* desktop);
};


class SmokeDecorator : public TabDecorator
#ifdef SMOKE_ROUNDED_CORNERS
	, public RoundedDecorator
#endif
{
public:
#ifdef SMOKE_ROUNDED_CORNERS
	virtual	void				GetRoundedShape(
									std::vector<ShapeRect>& shape) const;
#endif

	// a tab's or frame's colours, worked out from the window tab colour
	struct Palette {
		rgb_color				outer;
		rgb_color				light;
		rgb_color				face;
		rgb_color				inner;
		rgb_color				gradientTop;
		rgb_color				gradientBottom;
		rgb_color				glyph;
		rgb_color				glyphShade;
	};

								SmokeDecorator(DesktopSettings& settings,
									BRect frame, Desktop* desktop);
	virtual						~SmokeDecorator();

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

private:
#ifdef SMOKE_ROUNDED_CORNERS
			void				_StrokeRoundedFrame();
			void				_StrokeRoundedTab(Decorator::Tab* tab);
#endif
			bool				_HasTab() const;
			bool				_HasResizeCorner() const;
			float				_Scale() const;
			float				_TextHeight() const;
			float				_SkirtBottom() const;
			BRect				_CornerRect() const;

			Palette				_Palette(bool focus) const;
			Palette				_CornerPalette(bool focus) const;
			bool				_TabFocus(Decorator::Tab* tab) const;

			void				_StrokeFrame(BRect rect, const Palette& palette,
									int32 lines);
			void				_DrawTabBody(Decorator::Tab* tab,
									bool skirtOnly);
			void				_FillTabShading(Decorator::Tab* tab,
									BRect rect);
			void				_DrawResizeCorner(BRect invalid);
			void				_DrawGlyph(const char* const* glyph,
									int32 width, int32 height, BPoint origin,
									const Palette& palette, bool pressed);
};


#endif	// SMOKE_DECORATOR_H
