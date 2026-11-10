/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * app_server's decorator classes, as far as Power Theme's decorators use
 * them, rebuilt on the stable decorator style interface (DecorStyle.h).
 * Compiled against these instead of app_server's own headers, a decorator's
 * source becomes a style without changes: one build of it keeps working on
 * later systems. StyleShim.cpp turns the style's calls into the decorator's,
 * as app_server would make them.
 *
 * Only what the decorators use is here; the names, signatures and behaviour
 * follow app_server's Decorator, DrawingEngine, DrawState, ServerFont and
 * DesktopSettings.
 */
#ifndef STYLE_SHIM_H
#define STYLE_SHIM_H


#include <Font.h>
#include <GradientLinear.h>
#include <InterfaceDefs.h>
#include <Menu.h>
#include <Message.h>
#include <Point.h>
#include <Rect.h>
#include <Region.h>
#include <String.h>
#include <Window.h>

#include <decorator/DecorStyle.h>


class Desktop;
class StyleShim;


//	#pragma mark - fonts


class ServerFont {
public:
								ServerFont();

			float				Size() const;
			void				SetSize(float size);
			uint32				Flags() const { return fFont.flags; }
			void				SetFlags(uint32 flags) { fFont.flags = flags; }
			void				SetSpacing(uint8 spacing) {}

			void				GetHeight(font_height& height) const;
			float				StringWidth(const char* string,
									int32 length) const;
			float				StringWidth(const char* string) const
									{ return StringWidth(string, -1); }
			void				TruncateString(BString* string, uint32 mode,
									float width) const;

private:
	friend class DesktopSettings;
	friend class DrawState;
	friend class DrawingEngine;
	friend class StyleShim;

			decor_canvas*		fCanvas;
			decor_font			fFont;
};


class DrawState {
public:
								DrawState();

			const ServerFont&	Font() const { return fFont; }
			void				SetFont(const ServerFont& font);

private:
	friend class StyleShim;

			decor_canvas*		fCanvas;
			ServerFont			fFont;
};


class DesktopSettings {
public:
								DesktopSettings(decor_canvas* canvas);

			rgb_color			UIColor(color_which which) const;
			void				GetDefaultPlainFont(ServerFont& font) const;
			void				GetDefaultBoldFont(ServerFont& font) const;
			void				GetDefaultFixedFont(ServerFont& font) const;
			void				GetMenuInfo(menu_info& info) const;

private:
			void				_Font(int32 which, ServerFont& font) const;

			decor_canvas*		fCanvas;
};


//	#pragma mark - drawing


class DrawingEngine {
public:
								DrawingEngine();

			void				SetDrawState(const DrawState* state,
									int32 xOffset = 0, int32 yOffset = 0);

			void				SetHighColor(const rgb_color& color);
			void				SetLowColor(const rgb_color& color);
			void				SetPattern(const struct pattern& pattern);
			void				SetDrawingMode(drawing_mode mode);
			void				SetDrawingMode(drawing_mode mode,
									drawing_mode& oldMode);
			void				SetBlendingMode(source_alpha srcAlpha,
									alpha_function alphaFunction);
			void				SetFont(const ServerFont& font);
			void				SetFont(const DrawState* state);

			bool				CopyToFrontEnabled() const;
			void				SetCopyToFrontEnabled(bool enable);

			void				StrokeLine(const BPoint& start,
									const BPoint& end, const rgb_color& color);
			void				StrokeLine(const BPoint& start,
									const BPoint& end);
			void				StrokePoint(const BPoint& point,
									const rgb_color& color);
			void				StrokeRect(BRect rect, const rgb_color& color);
			void				StrokeRect(BRect rect);
			void				FillRect(BRect rect, const rgb_color& color);
			void				FillRect(BRect rect);
			void				FillRect(BRect rect, const BGradient& gradient);
			void				FillRegion(BRegion& region,
									const rgb_color& color);

			BPoint				DrawString(const char* string, int32 length,
									const BPoint& point,
									escapement_delta* delta = NULL);
			float				StringWidth(const char* string, int32 length,
									escapement_delta* delta = NULL);
			float				StringWidth(const char* string, int32 length,
									const ServerFont& font,
									escapement_delta* delta = NULL);

private:
	friend class Decorator;
	friend class StyleShim;

			decor_canvas*		fCanvas;
			rgb_color			fHighColor;
			drawing_mode		fDrawingMode;
};


//	#pragma mark - the decorator


/*!	Stands in for app_server's BObjectList of tabs. */
template<class T>
class ShimList {
public:
			int32				CountItems() const { return fCount; }
			T*					ItemAt(int32 index) const
									{
										return index >= 0 && index < fCount
											? fItems[index] : NULL;
									}
			int32				IndexOf(const T* item) const
									{
										for (int32 i = 0; i < fCount; i++) {
											if (fItems[i] == item)
												return i;
										}
										return -1;
									}

private:
	friend class StyleShim;

	static	const int32			kMaxItems = 64;
			T*					fItems[kMaxItems];
			int32				fCount = 0;
};


class Decorator {
public:
	struct Tab {
								Tab();
		virtual					~Tab() {}

		BRect					tabRect;

		BRect					zoomRect;
		BRect					closeRect;
		BRect					minimizeRect;

		bool					closePressed : 1;
		bool					zoomPressed : 1;
		bool					minimizePressed : 1;

		window_look				look;
		uint32					flags;
		bool					isFocused : 1;

		BString					title;

		uint32					tabOffset;
		float					tabLocation;
		float					textOffset;

		BString					truncatedTitle;
		int32					truncatedTitleLength;

		bool					buttonFocus : 1;

		bool					isHighlighted : 1;

		float					minTabSize;
		float					maxTabSize;
	};

	enum Region {
		REGION_NONE,

		REGION_TAB,

		REGION_CLOSE_BUTTON,
		REGION_ZOOM_BUTTON,
		REGION_MINIMIZE_BUTTON,

		REGION_LEFT_BORDER,
		REGION_RIGHT_BORDER,
		REGION_TOP_BORDER,
		REGION_BOTTOM_BORDER,

		REGION_LEFT_TOP_CORNER,
		REGION_LEFT_BOTTOM_CORNER,
		REGION_RIGHT_TOP_CORNER,
		REGION_RIGHT_BOTTOM_CORNER,

		REGION_COUNT
	};

	enum {
		HIGHLIGHT_NONE,
		HIGHLIGHT_RESIZE_BORDER,

		HIGHLIGHT_USER_DEFINED
	};

								Decorator(DesktopSettings& settings,
									BRect frame, Desktop* desktop);
	virtual						~Decorator();

			int32				CountTabs() const
									{ return fTabList.CountItems(); }
			Decorator::Tab*		TabAt(int32 index) const
									{ return fTabList.ItemAt(index); }

	virtual	void				UpdateColors(DesktopSettings& settings) = 0;

			window_look			Look(int32 tab) const;
			uint32				Flags(int32 tab) const;

			BRect				BorderRect() const { return fBorderRect; }
			BRect				TitleBarRect() const { return fTitleBarRect; }
			BRect				TabRect(int32 tab) const;
			BRect				TabRect(Decorator::Tab* tab) const
									{ return tab->tabRect; }

			const char*			Title(int32 tab) const;
			const char*			Title(Decorator::Tab* tab) const
									{ return tab->title.String(); }

			bool				IsFocus(int32 tab) const;
			bool				IsFocus(Decorator::Tab* tab) const
									{ return tab->isFocused; }

	virtual	float				TabLocation(int32 tab) const;
			bool				SetTabLocation(int32 tab, float location,
									bool isShifting,
									BRegion* updateRegion = NULL);

	virtual	Region				RegionAt(BPoint where, int32& tab) const;

			const BRegion&		GetFootprint();
			::Desktop*			GetDesktop() { return NULL; }

			bool				IsOutlineResizing() const
									{ return fOutlinesDelta != BPoint(0, 0); }

	virtual	bool				SetRegionHighlight(Region region,
									uint8 highlight, BRegion* dirty,
									int32 tab = -1);
			uint8				RegionHighlight(Region region,
									int32 tab = -1) const;

	virtual	bool				GetSettings(BMessage* settings) const;

	virtual	void				GetSizeLimits(int32* minWidth,
									int32* minHeight, int32* maxWidth,
									int32* maxHeight) const;
	virtual	void				ExtendDirtyRegion(Region region,
									BRegion& dirty);

	virtual	void				Draw(BRect updateRect) = 0;
	virtual	void				Draw() = 0;

			rgb_color			UIColor(color_which which);

			float				BorderWidth() { return fBorderWidth; }
			float				TabHeight();

protected:
	virtual	Decorator::Tab*		_AllocateNewTab();

	virtual	void				_DoLayout() = 0;
	virtual	void				_DoOutlineLayout() = 0;

	virtual	void				_DrawFrame(BRect rect) = 0;
	virtual	void				_DrawOutlineFrame(BRect rect) = 0;
	virtual	void				_DrawTabs(BRect rect);

	virtual	void				_DrawTab(Decorator::Tab* tab, BRect rect) = 0;
	virtual	void				_DrawTitle(Decorator::Tab* tab,
									BRect rect) = 0;

	virtual	void				_DrawButtons(Decorator::Tab* tab,
									const BRect& invalid) = 0;
	virtual	void				_DrawClose(Decorator::Tab* tab, bool direct,
									BRect rect) = 0;
	virtual	void				_DrawMinimize(Decorator::Tab* tab, bool direct,
									BRect rect) = 0;
	virtual	void				_DrawZoom(Decorator::Tab* tab, bool direct,
									BRect rect) = 0;

	virtual	void				_SetTitle(Decorator::Tab* tab,
									const char* string,
									BRegion* updateRegion = NULL) = 0;

	virtual	void				_SetFocus(Decorator::Tab* tab);
	virtual	bool				_SetTabLocation(Decorator::Tab* tab,
									float location, bool isShifting,
									BRegion* updateRegion = NULL);

	virtual	Decorator::Tab*		_TabAt(int32 index) const;

	virtual	void				_FontsChanged(DesktopSettings& settings,
									BRegion* updateRegion = NULL);
	virtual	void				_UpdateFont(DesktopSettings& settings) = 0;

	virtual	void				_SetLook(Decorator::Tab* tab,
									DesktopSettings& settings,
									window_look look,
									BRegion* updateRegion = NULL);
	virtual	void				_SetFlags(Decorator::Tab* tab, uint32 flags,
									BRegion* updateRegion = NULL);

	virtual	void				_MoveBy(BPoint offset);
	virtual	void				_ResizeBy(BPoint offset, BRegion* dirty) = 0;

	virtual	void				_MoveOutlineBy(BPoint offset);
	virtual	void				_ResizeOutlineBy(BPoint offset,
									BRegion* dirty);
	virtual	void				_SetOutlinesDelta(BPoint delta,
									BRegion* dirty);

	virtual	bool				_SetSettings(const BMessage& settings,
									BRegion* updateRegion = NULL);

	virtual	bool				_AddTab(DesktopSettings& settings,
									int32 index = -1,
									BRegion* updateRegion = NULL) = 0;
	virtual	bool				_RemoveTab(int32 index,
									BRegion* updateRegion = NULL) = 0;
	virtual	bool				_MoveTab(int32 from, int32 to, bool isMoving,
									BRegion* updateRegion = NULL) = 0;

	virtual	void				_GetFootprint(BRegion* region);
	virtual	void				_GetOutlineFootprint(BRegion* region);
			void				_InvalidateFootprint()
									{ fFootprintValid = false; }
			void				_InvalidateBitmaps() {}

protected:
			DrawingEngine*		fDrawingEngine;
			DrawState			fDrawState;

			BPoint				fOutlinesDelta;

			BRect				fTitleBarRect;
			BRect				fFrame;
			BRect				fResizeRect;
			BRect				fBorderRect;
			BRect				fOutlineBorderRect;

			BRect				fLeftBorder;
			BRect				fTopBorder;
			BRect				fBottomBorder;
			BRect				fRightBorder;

			BRect				fLeftOutlineBorder;
			BRect				fTopOutlineBorder;
			BRect				fBottomOutlineBorder;
			BRect				fRightOutlineBorder;

			int32				fBorderWidth;
			int32				fOutlineBorderWidth;

			Decorator::Tab*		fTopTab;
			ShimList<Decorator::Tab> fTabList;

private:
	friend class StyleShim;

			BRegion				fFootprint;
			bool				fFootprintValid;
			uint8				fRegionHighlights[REGION_COUNT - 1];
};


//	#pragma mark - the add-on


class DecorAddOn {
public:
								DecorAddOn(image_id id, const char* name);
	virtual						~DecorAddOn();

	virtual	status_t			InitCheck() const { return B_OK; }
			image_id			ImageID() const { return fImageID; }

protected:
	virtual	Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, Desktop* desktop);

private:
	friend class StyleShim;

			image_id			fImageID;
};


#endif	// STYLE_SHIM_H
