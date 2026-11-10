/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * What Power Theme's decorator styles share: the structures of DecorStyle.h
 * seen through BRect and BPoint, so that a style's drawing reads as a
 * decorator's did. Only libbe's public, stable classes are used, so a style
 * built once keeps working on later systems.
 */
#ifndef STYLE_KIT_H
#define STYLE_KIT_H


#include <algorithm>
#include <math.h>
#include <string.h>

#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <Point.h>
#include <Rect.h>
#include <Window.h>

#include <decorator/DecorStyle.h>


namespace StyleKit {


// app_server's look for a window with its tab on the left
static const window_look kLeftTitledWindowLook = window_look(25);


inline BRect
R(const decor_rect& rect)
{
	return BRect(rect.left, rect.top, rect.right, rect.bottom);
}


inline decor_rect
D(const BRect& rect)
{
	decor_rect result = { rect.left, rect.top, rect.right, rect.bottom };
	return result;
}


inline decor_point
P(const BPoint& point)
{
	decor_point result = { point.x, point.y };
	return result;
}


inline decor_rect
Invalid()
{
	return D(BRect(0, 0, -1, -1));
}


//!	\a color, each of its channels moved by \a delta.
inline rgb_color
Shift(rgb_color color, int delta)
{
	color.red = (uint8)std::max(0, std::min(255, color.red + delta));
	color.green = (uint8)std::max(0, std::min(255, color.green + delta));
	color.blue = (uint8)std::max(0, std::min(255, color.blue + delta));
	return color;
}


inline rgb_color
Mix(rgb_color a, rgb_color b, float amount)
{
	rgb_color color;
	color.red = (uint8)roundf(a.red + (b.red - a.red) * amount);
	color.green = (uint8)roundf(a.green + (b.green - a.green) * amount);
	color.blue = (uint8)roundf(a.blue + (b.blue - a.blue) * amount);
	color.alpha = 255;
	return color;
}


inline rgb_color
Grey(int level)
{
	const uint8 value = (uint8)std::max(0, std::min(255, level));
	rgb_color color = { value, value, value, 255 };
	return color;
}


/*!	app_server's canvas, with the calls a decorator's drawing engine had. */
class Canvas {
public:
	Canvas(decor_canvas* canvas)
		:
		fCanvas(canvas)
	{
	}

	rgb_color UIColor(color_which which) const
		{ return fCanvas->ui_color(fCanvas, which); }

	void SetDrawingMode(drawing_mode mode)
		{ fCanvas->set_drawing_mode(fCanvas, mode); }
	void SetBlendingMode(source_alpha srcAlpha, alpha_function function)
		{ fCanvas->set_blending_mode(fCanvas, srcAlpha, function); }
	void SetHighColor(rgb_color color)
		{ fCanvas->set_high_color(fCanvas, color); }
	void SetLowColor(rgb_color color)
		{ fCanvas->set_low_color(fCanvas, color); }
	void SetPattern(const pattern& value)
		{ fCanvas->set_pattern(fCanvas, &value); }

	void StrokeLine(BPoint start, BPoint end, rgb_color color)
		{ fCanvas->stroke_line(fCanvas, P(start), P(end), color); }
	void StrokePoint(BPoint point, rgb_color color)
		{ fCanvas->stroke_point(fCanvas, P(point), color); }
	void StrokeRect(BRect rect, rgb_color color)
		{ fCanvas->stroke_rect(fCanvas, D(rect), color); }
	void FillRect(BRect rect, rgb_color color)
		{
			if (rect.IsValid())
				fCanvas->fill_rect(fCanvas, D(rect), color);
		}
	void FillRect(BRect rect, const decor_gradient& gradient)
		{
			if (rect.IsValid())
				fCanvas->fill_rect_gradient(fCanvas, D(rect), &gradient);
		}
	void StrokeRect(BRect rect)
		{ fCanvas->stroke_rect_pattern(fCanvas, D(rect)); }
	void FillRect(BRect rect)
		{
			if (rect.IsValid())
				fCanvas->fill_rect_pattern(fCanvas, D(rect));
		}

	void DrawBitmap(const void* bits, int32 width, int32 height,
		int32 bytesPerRow, BRect source, BRect destination)
		{
			fCanvas->draw_bitmap(fCanvas, bits, width, height, bytesPerRow,
				D(source), D(destination));
		}

	void SetFont(int32 which, float size, uint32 flags)
		{ fCanvas->set_font(fCanvas, which, size, flags); }
	float FontSize(int32 which) const
		{ return fCanvas->font_size(fCanvas, which); }
	void GetFontHeight(font_height& height) const
		{
			fCanvas->font_height(fCanvas, &height.ascent, &height.descent,
				&height.leading);
		}
	float StringWidth(const char* string, int32 length = -1) const
		{ return fCanvas->string_width(fCanvas, string, length); }
	void DrawString(const char* string, int32 length, BPoint baseline,
		rgb_color color)
		{ fCanvas->draw_string(fCanvas, string, length, P(baseline), color); }

	bool CopyToFrontEnabled() const
		{ return fCanvas->copy_to_front(fCanvas); }
	void SetCopyToFrontEnabled(bool enabled)
		{ fCanvas->set_copy_to_front(fCanvas, enabled); }

private:
	decor_canvas*	fCanvas;
};


/*!	A linear gradient to fill with. */
class Gradient : public decor_gradient {
public:
	Gradient()
	{
		start.x = start.y = end.x = end.y = 0;
		stop_count = 0;
	}

	void SetStart(BPoint point) { start = P(point); }
	void SetEnd(BPoint point) { end = P(point); }
	void AddColor(rgb_color color, uint8 offset)
	{
		if (stop_count >= DECOR_GRADIENT_STOPS)
			return;
		colors[stop_count] = color;
		offsets[stop_count] = offset;
		stop_count++;
	}
};


/*!	A window's frame, as a decorator saw itself. */
class Window {
public:
	Window(decor_window* window)
		:
		fWindow(window)
	{
	}

	decor_window* Get() const { return fWindow; }

	int32 CountTabs() const { return fWindow->tab_count; }
	decor_tab& TabAt(int32 index) const { return fWindow->tabs[index]; }
	decor_tab& TopTab() const
		{
			return fWindow->tabs[fWindow->top_tab >= 0
				? fWindow->top_tab : 0];
		}
	window_look Look() const { return (window_look)TopTab().look; }
	uint32 Flags() const { return TopTab().flags; }

	BRect Frame() const { return R(fWindow->frame); }
	float BorderWidth() const { return fWindow->border_width; }
	BRect LeftBorder() const { return R(fWindow->left_border); }
	BRect TopBorder() const { return R(fWindow->top_border); }
	BRect RightBorder() const { return R(fWindow->right_border); }
	BRect BottomBorder() const { return R(fWindow->bottom_border); }
	BRect BorderRect() const
		{
			return fWindow->border_width > 0
				? BRect(TopBorder().LeftTop(), BottomBorder().RightBottom())
				: Frame();
		}
	BRect TitleBarRect() const { return R(fWindow->title_bar); }
	BRect ResizeRect() const { return R(fWindow->resize); }

	uint8 Highlight(int32 region) const
		{
			return region >= 0 && region < DECOR_REGION_COUNT
				? fWindow->highlights[region] : 0;
		}

	void SetBorders(float width, BRect left, BRect top, BRect right,
		BRect bottom)
		{
			fWindow->border_width = width;
			fWindow->left_border = D(left);
			fWindow->top_border = D(top);
			fWindow->right_border = D(right);
			fWindow->bottom_border = D(bottom);
		}
	void SetTitleBarRect(BRect rect) { fWindow->title_bar = D(rect); }
	void SetResizeRect(BRect rect) { fWindow->resize = D(rect); }
	void SetTabsSlide(bool slide) { fWindow->tabs_slide = slide; }

private:
	decor_window*	fWindow;
};


inline BRect TabRect(const decor_tab& tab) { return R(tab.frame); }
inline BRect ButtonRect(const decor_tab& tab, int32 button)
	{ return R(tab.buttons[button]); }


}	// namespace StyleKit


#endif	// STYLE_KIT_H
