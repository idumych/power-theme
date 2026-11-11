/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * See StyleShim.h. This makes a decorator, built against the shim, into a
 * style: app_server's style engine calls the style, and the style calls the
 * decorator as app_server would have.
 *
 * The decorator's own add-on entry, instantiate_decor_addon(), is renamed to
 * shim_instantiate_decor_addon() when it's compiled (-D), and is called once,
 * when the style is.
 */


#include "StyleShim.h"
#include "RoundedDecorator.h"
#include "WindowPrivate.h"

#include <stddef.h>

#include <algorithm>
#include <new>
#include <set>
#include <string.h>

#include <Font.h>


extern "C" DecorAddOn* shim_instantiate_decor_addon(image_id id,
	const char* name);


static inline BRect
to_rect(const decor_rect& rect)
{
	return BRect(rect.left, rect.top, rect.right, rect.bottom);
}


static inline decor_rect
to_decor_rect(const BRect& rect)
{
	decor_rect result = { rect.left, rect.top, rect.right, rect.bottom };
	return result;
}


static inline decor_point
to_decor_point(const BPoint& point)
{
	decor_point result = { point.x, point.y };
	return result;
}


//	#pragma mark - ServerFont


ServerFont::ServerFont()
	:
	fCanvas(NULL)
{
	fFont.which = DECOR_FONT_PLAIN;
	fFont.size = 0;
	fFont.flags = 0;
	fFont.rotation = 0;
}


float
ServerFont::Size() const
{
	if (fFont.size > 0 || fCanvas == NULL)
		return fFont.size;
	return fCanvas->font_size(fCanvas, fFont.which);
}


void
ServerFont::SetSize(float size)
{
	fFont.size = size;
}


void
ServerFont::GetHeight(font_height& height) const
{
	height.ascent = height.descent = height.leading = 0;
	if (fCanvas != NULL) {
		fCanvas->font_height(fCanvas, &fFont, &height.ascent, &height.descent,
			&height.leading);
	}
}


float
ServerFont::StringWidth(const char* string, int32 length) const
{
	if (fCanvas == NULL || string == NULL)
		return 0;
	return fCanvas->string_width(fCanvas, &fFont, string, length);
}


void
ServerFont::TruncateString(BString* string, uint32 mode, float width) const
{
	if (fCanvas == NULL || string == NULL)
		return;

	uint32 truncate;
	switch (mode) {
		case B_TRUNCATE_MIDDLE:
			truncate = DECOR_TRUNCATE_MIDDLE;
			break;
		case B_TRUNCATE_BEGINNING:
			truncate = DECOR_TRUNCATE_BEGINNING;
			break;
		default:
			truncate = DECOR_TRUNCATE_END;
			break;
	}

	const int32 size = string->Length() + 8;
	BString result;
	char* buffer = result.LockBuffer(size);
	const int32 length = fCanvas->truncate_string(fCanvas, &fFont,
		string->String(), truncate, width, buffer, size);
	result.UnlockBuffer(length);
	*string = result;
}


//	#pragma mark - DrawState


DrawState::DrawState()
	:
	fCanvas(NULL)
{
}


void
DrawState::SetFont(const ServerFont& font)
{
	fFont = font;
	if (fCanvas != NULL) {
		fFont.fCanvas = fCanvas;
		fCanvas->set_font(fCanvas, &fFont.fFont);
	}
}


//	#pragma mark - DesktopSettings


DesktopSettings::DesktopSettings(decor_canvas* canvas)
	:
	fCanvas(canvas)
{
}


rgb_color
DesktopSettings::UIColor(color_which which) const
{
	return fCanvas->ui_color(fCanvas, which);
}


void
DesktopSettings::GetDefaultPlainFont(ServerFont& font) const
{
	_Font(DECOR_FONT_PLAIN, font);
}


void
DesktopSettings::GetDefaultBoldFont(ServerFont& font) const
{
	_Font(DECOR_FONT_BOLD, font);
}


void
DesktopSettings::GetDefaultFixedFont(ServerFont& font) const
{
	_Font(DECOR_FONT_FIXED, font);
}


void
DesktopSettings::GetMenuInfo(menu_info& info) const
{
	// app_server's own settings, which libbe can't ask for from inside it
	memset(&info, 0, sizeof(info));
	info.font_size = fCanvas->font_size(fCanvas, DECOR_FONT_MENU);
	info.background_color = UIColor(B_MENU_BACKGROUND_COLOR);
	info.separator = 0;
	info.click_to_open = true;
	info.triggers_always_shown = false;
}


void
DesktopSettings::_Font(int32 which, ServerFont& font) const
{
	font.fCanvas = fCanvas;
	font.fFont.which = which;
	font.fFont.size = fCanvas->font_size(fCanvas, which);
	font.fFont.flags = 0;
	font.fFont.rotation = 0;
}


//	#pragma mark - DrawingEngine


DrawingEngine::DrawingEngine()
	:
	fCanvas(NULL),
	fDrawingMode(B_OP_COPY),
	fRecording(false),
	fDrawings(NULL),
	fCount(0),
	fCapacity(0)
{
	fHighColor = make_color(0, 0, 0);
}


bool
DrawingEngine::_Record(const ShimDrawing& drawing)
{
	if (!fRecording)
		return false;

	if (fCount == fCapacity) {
		const int32 capacity = fCapacity > 0 ? fCapacity * 2 : 16;
		ShimDrawing* drawings = new(std::nothrow) ShimDrawing[capacity];
		if (drawings == NULL)
			return true;
		for (int32 i = 0; i < fCount; i++)
			drawings[i] = fDrawings[i];
		delete[] fDrawings;
		fDrawings = drawings;
		fCapacity = capacity;
	}
	fDrawings[fCount++] = drawing;
	return true;
}


/*!	Draws again what was drawn into \a bitmap, moved to \a viewRect. Its
	transparent background is left out, as B_OP_OVER would.
*/
void
DrawingEngine::DrawBitmap(ServerBitmap* bitmap, const BRect& bitmapRect,
	const BRect& viewRect, uint32 options)
{
	if (bitmap == NULL || fCanvas == NULL)
		return;

	const BPoint offset = viewRect.LeftTop() - bitmapRect.LeftTop();
	for (int32 i = 0; i < bitmap->fCount; i++) {
		ShimDrawing drawing = bitmap->fDrawings[i];
		switch (drawing.what) {
			case ShimDrawing::FILL_RECT:
				if (drawing.color == B_TRANSPARENT_COLOR)
					break;
				FillRect(drawing.rect.OffsetByCopy(offset), drawing.color);
				break;
			case ShimDrawing::FILL_GRADIENT:
				drawing.gradient.start.x += offset.x;
				drawing.gradient.start.y += offset.y;
				drawing.gradient.end.x += offset.x;
				drawing.gradient.end.y += offset.y;
				fCanvas->fill_rect_gradient(fCanvas,
					to_decor_rect(drawing.rect.OffsetByCopy(offset)),
					&drawing.gradient);
				break;
			case ShimDrawing::STROKE_RECT:
				StrokeRect(drawing.rect.OffsetByCopy(offset), drawing.color);
				break;
			case ShimDrawing::STROKE_LINE:
				StrokeLine(drawing.start + offset, drawing.end + offset,
					drawing.color);
				break;
		}
	}
}


void
DrawingEngine::SetDrawState(const DrawState* state, int32 xOffset,
	int32 yOffset)
{
	// as app_server's: the state's own mode and colour, which a decorator's
	// is always left at
	SetDrawingMode(B_OP_COPY);
	SetHighColor(make_color(0, 0, 0));
	if (state != NULL)
		SetFont(state);
}


void
DrawingEngine::SetHighColor(const rgb_color& color)
{
	fHighColor = color;
	fCanvas->set_high_color(fCanvas, color);
}


void
DrawingEngine::SetLowColor(const rgb_color& color)
{
	fCanvas->set_low_color(fCanvas, color);
}


void
DrawingEngine::SetPattern(const struct pattern& pattern)
{
	fCanvas->set_pattern(fCanvas, &pattern);
}


void
DrawingEngine::SetDrawingMode(drawing_mode mode)
{
	fDrawingMode = mode;
	fCanvas->set_drawing_mode(fCanvas, mode);
}


void
DrawingEngine::SetDrawingMode(drawing_mode mode, drawing_mode& oldMode)
{
	oldMode = fDrawingMode;
	SetDrawingMode(mode);
}


void
DrawingEngine::SetBlendingMode(source_alpha srcAlpha,
	alpha_function alphaFunction)
{
	fCanvas->set_blending_mode(fCanvas, srcAlpha, alphaFunction);
}


void
DrawingEngine::SetFont(const ServerFont& font)
{
	fCanvas->set_font(fCanvas, &font.fFont);
}


void
DrawingEngine::SetFont(const DrawState* state)
{
	SetFont(state->Font());
}


bool
DrawingEngine::CopyToFrontEnabled() const
{
	return fCanvas->copy_to_front(fCanvas);
}


void
DrawingEngine::SetCopyToFrontEnabled(bool enable)
{
	fCanvas->set_copy_to_front(fCanvas, enable);
}


void
DrawingEngine::StrokeLine(const BPoint& start, const BPoint& end,
	const rgb_color& color)
{
	ShimDrawing drawing;
	drawing.what = ShimDrawing::STROKE_LINE;
	drawing.start = start;
	drawing.end = end;
	drawing.color = color;
	if (_Record(drawing))
		return;
	fCanvas->stroke_line(fCanvas, to_decor_point(start), to_decor_point(end),
		color);
}


void
DrawingEngine::StrokeLine(const BPoint& start, const BPoint& end)
{
	StrokeLine(start, end, fHighColor);
}


void
DrawingEngine::StrokePoint(const BPoint& point, const rgb_color& color)
{
	fCanvas->stroke_point(fCanvas, to_decor_point(point), color);
}


void
DrawingEngine::StrokeRect(BRect rect, const rgb_color& color)
{
	ShimDrawing drawing;
	drawing.what = ShimDrawing::STROKE_RECT;
	drawing.rect = rect;
	drawing.color = color;
	if (_Record(drawing))
		return;
	fCanvas->stroke_rect(fCanvas, to_decor_rect(rect), color);
}


void
DrawingEngine::StrokeRect(BRect rect)
{
	fCanvas->stroke_rect_pattern(fCanvas, to_decor_rect(rect));
}


void
DrawingEngine::FillRect(BRect rect, const rgb_color& color)
{
	ShimDrawing drawing;
	drawing.what = ShimDrawing::FILL_RECT;
	drawing.rect = rect;
	drawing.color = color;
	if (_Record(drawing))
		return;
	fCanvas->fill_rect(fCanvas, to_decor_rect(rect), color);
}


void
DrawingEngine::FillRect(BRect rect)
{
	fCanvas->fill_rect_pattern(fCanvas, to_decor_rect(rect));
}


void
DrawingEngine::FillRect(BRect rect, const BGradient& gradient)
{
	if (gradient.GetType() != BGradient::TYPE_LINEAR)
		return;
	const BGradientLinear& linear
		= static_cast<const BGradientLinear&>(gradient);

	decor_gradient styleGradient;
	styleGradient.start = to_decor_point(linear.Start());
	styleGradient.end = to_decor_point(linear.End());
	styleGradient.stop_count = std::min(
		(int32)gradient.CountColorStops(), (int32)DECOR_GRADIENT_STOPS);
	for (int32 i = 0; i < styleGradient.stop_count; i++) {
		const BGradient::ColorStop* stop = gradient.ColorStopAt(i);
		styleGradient.colors[i] = stop->color;
		styleGradient.offsets[i] = (uint8)stop->offset;
	}
	ShimDrawing drawing;
	drawing.what = ShimDrawing::FILL_GRADIENT;
	drawing.rect = rect;
	drawing.gradient = styleGradient;
	if (_Record(drawing))
		return;
	fCanvas->fill_rect_gradient(fCanvas, to_decor_rect(rect),
		&styleGradient);
}


void
DrawingEngine::FillRegion(BRegion& region, const rgb_color& color)
{
	for (int32 i = 0; i < region.CountRects(); i++)
		FillRect(region.RectAt(i), color);
}


BPoint
DrawingEngine::DrawString(const char* string, int32 length,
	const BPoint& point, escapement_delta* delta)
{
	fCanvas->draw_string(fCanvas, string, length, to_decor_point(point),
		fHighColor);
	return point + BPoint(StringWidth(string, length), 0);
}


float
DrawingEngine::StringWidth(const char* string, int32 length,
	escapement_delta* delta)
{
	return fCanvas->string_width(fCanvas, NULL, string, length);
}


float
DrawingEngine::StringWidth(const char* string, int32 length,
	const ServerFont& font, escapement_delta* delta)
{
	return fCanvas->string_width(fCanvas, &font.fFont, string, length);
}


//	#pragma mark - bitmaps


ServerBitmap::~ServerBitmap()
{
	delete[] fDrawings;
}


UtilityBitmap::UtilityBitmap(BRect bounds, color_space space, uint32 flags)
{
	fBounds = bounds;
}


BitmapDrawingEngine::BitmapDrawingEngine()
{
	fRecording = true;
}


BitmapDrawingEngine::~BitmapDrawingEngine()
{
	delete[] fDrawings;
}


status_t
BitmapDrawingEngine::SetSize(int32 width, int32 height)
{
	fBounds.Set(0, 0, width - 1, height - 1);
	fCount = 0;
	return B_OK;
}


UtilityBitmap*
BitmapDrawingEngine::ExportToBitmap(int32 width, int32 height,
	color_space space)
{
	UtilityBitmap* bitmap = new(std::nothrow) UtilityBitmap(
		BRect(0, 0, width - 1, height - 1), space, 0);
	if (bitmap == NULL)
		return NULL;
	bitmap->fDrawings = new(std::nothrow) ShimDrawing[fCount > 0 ? fCount : 1];
	if (bitmap->fDrawings == NULL) {
		delete bitmap;
		return NULL;
	}
	for (int32 i = 0; i < fCount; i++)
		bitmap->fDrawings[i] = fDrawings[i];
	bitmap->fCount = fCount;
	return bitmap;
}


//	#pragma mark - RoundedDecorator


// Whether app_server cuts off the corners of the window being drawn: set
// for each call from the engine, which can come from several threads.
static __thread bool sCornersCut = false;


/*static*/ bool
RoundedDecorator::IsActive(Desktop* desktop)
{
	return sCornersCut;
}


/*!	As app_server's RoundedDecorator::RoundShape(). */
/*static*/ void
RoundedDecorator::RoundShape(std::vector<Decorator::ShapeRect>& shape,
	window_look look, int32 frameRadius, int32 tabRadius)
{
	if (look != B_TITLED_WINDOW_LOOK && look != B_DOCUMENT_WINDOW_LOOK
		&& look != B_MODAL_WINDOW_LOOK && look != B_FLOATING_WINDOW_LOOK
		&& look != kLeftTitledWindowLook) {
		return;
	}

	Decorator::ShapeRect& frame = shape[0];
	const BRect border = frame.rect;
	frame.leftTop = frame.rightTop = frame.leftBottom = frame.rightBottom
		= frameRadius;

	for (size_t i = 1; i < shape.size(); i++) {
		Decorator::ShapeRect& tab = shape[i];

		// A title bar inside the frame has no corners of its own to round.
		if (border.Contains(tab.rect))
			continue;

		tab.leftTop = tabRadius;

		// The frame keeps its corner where a tab grows out of it.
		if (look == kLeftTitledWindowLook) {
			tab.leftBottom = tabRadius;
			if (tab.rect.top <= border.top + frameRadius)
				frame.leftTop = 0;
			if (tab.rect.bottom >= border.bottom - frameRadius)
				frame.leftBottom = 0;
		} else {
			tab.rightTop = tabRadius;
			if (tab.rect.left <= border.left + frameRadius)
				frame.leftTop = 0;
			if (tab.rect.right >= border.right - frameRadius)
				frame.rightTop = 0;
		}
	}
}


/*static*/ void
RoundedDecorator::StrokeCorners(DrawingEngine* engine,
	const Decorator::ShapeRect& shape, rgb_color leftTop, rgb_color rightTop,
	rgb_color leftBottom, rgb_color rightBottom)
{
	const BRect& rect = shape.rect;
	const int32 lt = (int32)shape.leftTop;
	const int32 rt = (int32)shape.rightTop;
	const int32 lb = (int32)shape.leftBottom;
	const int32 rb = (int32)shape.rightBottom;

	if (lt > 0)
		StrokeCorner(engine, rect.LeftTop(), lt, WindowShape::LEFT_TOP, leftTop);
	if (rt > 0) {
		StrokeCorner(engine, BPoint(rect.right - rt + 1, rect.top), rt,
			WindowShape::RIGHT_TOP, rightTop);
	}
	if (lb > 0) {
		StrokeCorner(engine, BPoint(rect.left, rect.bottom - lb + 1), lb,
			WindowShape::LEFT_BOTTOM, leftBottom);
	}
	if (rb > 0) {
		StrokeCorner(engine, BPoint(rect.right - rb + 1, rect.bottom - rb + 1),
			rb, WindowShape::RIGHT_BOTTOM, rightBottom);
	}
}


/*!	app_server draws the line, as for its own decorators. */
/*static*/ void
RoundedDecorator::StrokeCorner(DrawingEngine* engine, BPoint leftTop,
	int32 radius, WindowShape::corner_kind kind, rgb_color color)
{
	decor_canvas* canvas = engine != NULL ? engine->fCanvas : NULL;
	if (canvas == NULL || canvas->size < offsetof(decor_canvas, stroke_corner)
			+ sizeof(canvas->stroke_corner)
		|| canvas->stroke_corner == NULL) {
		return;
	}
	canvas->stroke_corner(canvas, to_decor_point(leftTop), radius, kind,
		color);
}


//	#pragma mark - Decorator


Decorator::Tab::Tab()
	:
	closePressed(false),
	zoomPressed(false),
	minimizePressed(false),
	look(B_TITLED_WINDOW_LOOK),
	flags(0),
	isFocused(false),
	tabOffset(0),
	tabLocation(0.0f),
	textOffset(10.0f),
	truncatedTitleLength(0),
	buttonFocus(false),
	isHighlighted(false),
	minTabSize(0.0f),
	maxTabSize(0.0f)
{
	memset(closeBitmaps, 0, sizeof(closeBitmaps));
	memset(minimizeBitmaps, 0, sizeof(minimizeBitmaps));
	memset(zoomBitmaps, 0, sizeof(zoomBitmaps));
}


Decorator::Decorator(DesktopSettings& settings, BRect frame,
	Desktop* desktop)
	:
	fDrawingEngine(NULL),
	fTitleBarRect(),
	fFrame(frame),
	fResizeRect(),
	fBorderRect(),
	fOutlineBorderRect(),
	fBorderWidth(-1),
	fOutlineBorderWidth(-1),
	fTopTab(NULL),
	fFootprintValid(false)
{
	memset(fRegionHighlights, HIGHLIGHT_NONE, sizeof(fRegionHighlights));
}


Decorator::~Decorator()
{
}


window_look
Decorator::Look(int32 tab) const
{
	Decorator::Tab* decoratorTab = TabAt(tab);
	return decoratorTab != NULL ? decoratorTab->look : B_TITLED_WINDOW_LOOK;
}


uint32
Decorator::Flags(int32 tab) const
{
	Decorator::Tab* decoratorTab = TabAt(tab);
	return decoratorTab != NULL ? decoratorTab->flags : 0;
}


BRect
Decorator::TabRect(int32 tab) const
{
	Decorator::Tab* decoratorTab = TabAt(tab);
	return decoratorTab != NULL ? decoratorTab->tabRect : BRect();
}


const char*
Decorator::Title(int32 tab) const
{
	Decorator::Tab* decoratorTab = TabAt(tab);
	return decoratorTab != NULL ? decoratorTab->title.String() : "";
}


bool
Decorator::IsFocus(int32 tab) const
{
	Decorator::Tab* decoratorTab = TabAt(tab);
	return decoratorTab != NULL && decoratorTab->isFocused;
}


float
Decorator::TabLocation(int32 tab) const
{
	Decorator::Tab* decoratorTab = _TabAt(tab);
	return decoratorTab != NULL ? (float)decoratorTab->tabOffset : 0.0f;
}


bool
Decorator::SetTabLocation(int32 tab, float location, bool isShifting,
	BRegion* updateRegion)
{
	Decorator::Tab* decoratorTab = TabAt(tab);
	if (decoratorTab == NULL)
		return false;
	if (_SetTabLocation(decoratorTab, location, isShifting, updateRegion)) {
		_InvalidateFootprint();
		return true;
	}
	return false;
}


Decorator::Region
Decorator::RegionAt(BPoint where, int32& tabIndex) const
{
	tabIndex = -1;

	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		if (tab->closeRect.Contains(where)) {
			tabIndex = i;
			return REGION_CLOSE_BUTTON;
		}
		if (tab->zoomRect.Contains(where)) {
			tabIndex = i;
			return REGION_ZOOM_BUTTON;
		}
		if (tab->tabRect.Contains(where)) {
			tabIndex = i;
			return REGION_TAB;
		}
	}

	return REGION_NONE;
}


const BRegion&
Decorator::GetFootprint()
{
	if (!fFootprintValid) {
		fFootprint.MakeEmpty();
		_GetFootprint(&fFootprint);
		if (IsOutlineResizing())
			_GetOutlineFootprint(&fFootprint);
		fFootprintValid = true;
	}
	return fFootprint;
}


bool
Decorator::SetRegionHighlight(Region region, uint8 highlight, BRegion* dirty,
	int32 tab)
{
	const int32 index = (int32)region - 1;
	if (index < 0 || index >= REGION_COUNT - 1)
		return false;
	fRegionHighlights[index] = highlight;
	return true;
}


uint8
Decorator::RegionHighlight(Region region, int32 tab) const
{
	const int32 index = (int32)region - 1;
	return index >= 0 && index < REGION_COUNT - 1
		? fRegionHighlights[index] : 0;
}


bool
Decorator::GetSettings(BMessage* settings) const
{
	return false;
}


void
Decorator::GetShape(std::vector<ShapeRect>& shape) const
{
	ShapeRect frame = { fBorderRect.IsValid() ? fBorderRect : fFrame,
		0, 0, 0, 0 };
	shape.push_back(frame);

	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		const BRect& tabRect = fTabList.ItemAt(i)->tabRect;
		if (tabRect.IsValid()) {
			ShapeRect tab = { tabRect, 0, 0, 0, 0 };
			shape.push_back(tab);
		}
	}
}


void
Decorator::GetSizeLimits(int32* minWidth, int32* minHeight, int32* maxWidth,
	int32* maxHeight) const
{
	float minTabSize = 0;
	if (CountTabs() > 0)
		minTabSize = _TabAt(0)->minTabSize;

	if (fTitleBarRect.IsValid()) {
		*minWidth = (int32)roundf(std::max((float)*minWidth,
			minTabSize - 2 * fBorderWidth));
	}
	if (fResizeRect.IsValid()) {
		*minHeight = (int32)roundf(std::max((float)*minHeight,
			fResizeRect.Height() - fBorderWidth));
	}
}


void
Decorator::ExtendDirtyRegion(Region region, BRegion& dirty)
{
}


rgb_color
Decorator::UIColor(color_which which)
{
	return fDrawingEngine != NULL && fDrawingEngine->fCanvas != NULL
		? fDrawingEngine->fCanvas->ui_color(fDrawingEngine->fCanvas, which)
		: make_color(0, 0, 0);
}


float
Decorator::TabHeight()
{
	if (fTitleBarRect.IsValid())
		return fTitleBarRect.Height();
	return fBorderWidth;
}


Decorator::Tab*
Decorator::_AllocateNewTab()
{
	Decorator::Tab* tab = new(std::nothrow) Decorator::Tab;
	if (tab == NULL)
		return NULL;
	_SetFocus(tab);
	return tab;
}


void
Decorator::_DrawTabs(BRect rect)
{
	Decorator::Tab* focusTab = NULL;
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);
		if (tab->isFocused) {
			focusTab = tab;
			continue;
		}
		_DrawTab(tab, rect);
	}

	if (focusTab != NULL)
		_DrawTab(focusTab, rect);
}


void
Decorator::_SetFocus(Decorator::Tab* tab)
{
}


bool
Decorator::_SetTabLocation(Decorator::Tab* tab, float location,
	bool isShifting, BRegion* updateRegion)
{
	return false;
}


Decorator::Tab*
Decorator::_TabAt(int32 index) const
{
	return fTabList.ItemAt(index);
}


void
Decorator::_FontsChanged(DesktopSettings& settings, BRegion* updateRegion)
{
	_UpdateFont(settings);
	_DoLayout();
	_DoOutlineLayout();
	_InvalidateFootprint();
}


void
Decorator::_SetLook(Decorator::Tab* tab, DesktopSettings& settings,
	window_look look, BRegion* updateRegion)
{
	tab->look = look;
	_UpdateFont(settings);
	_DoLayout();
	_DoOutlineLayout();
	_InvalidateFootprint();
}


void
Decorator::_SetFlags(Decorator::Tab* tab, uint32 flags, BRegion* updateRegion)
{
	tab->flags = flags;
	_DoLayout();
	_DoOutlineLayout();
	_InvalidateFootprint();
}


void
Decorator::_MoveBy(BPoint offset)
{
	for (int32 i = 0; i < fTabList.CountItems(); i++) {
		Decorator::Tab* tab = fTabList.ItemAt(i);

		tab->zoomRect.OffsetBy(offset);
		tab->closeRect.OffsetBy(offset);
		tab->minimizeRect.OffsetBy(offset);
		tab->tabRect.OffsetBy(offset);
	}
	fTitleBarRect.OffsetBy(offset);
	fFrame.OffsetBy(offset);
	fResizeRect.OffsetBy(offset);
	fBorderRect.OffsetBy(offset);
}


void
Decorator::_MoveOutlineBy(BPoint offset)
{
	fOutlineBorderRect.OffsetBy(offset);

	fLeftOutlineBorder.OffsetBy(offset);
	fRightOutlineBorder.OffsetBy(offset);
	fTopOutlineBorder.OffsetBy(offset);
	fBottomOutlineBorder.OffsetBy(offset);
}


void
Decorator::_ResizeOutlineBy(BPoint offset, BRegion* dirty)
{
	fOutlineBorderRect.right += offset.x;
	fOutlineBorderRect.bottom += offset.y;

	fLeftOutlineBorder.bottom += offset.y;
	fTopOutlineBorder.right += offset.x;

	fRightOutlineBorder.OffsetBy(offset.x, 0.0);
	fRightOutlineBorder.bottom += offset.y;

	fBottomOutlineBorder.OffsetBy(0.0, offset.y);
	fBottomOutlineBorder.right += offset.x;
}


void
Decorator::_SetOutlinesDelta(BPoint delta, BRegion* dirty)
{
	fOutlinesDelta = delta;
}


bool
Decorator::_SetSettings(const BMessage& settings, BRegion* updateRegion)
{
	return false;
}


void
Decorator::_GetFootprint(BRegion* region)
{
}


void
Decorator::_GetOutlineFootprint(BRegion* region)
{
	if (region == NULL)
		return;

	region->Include(fTopOutlineBorder);
	region->Include(fLeftOutlineBorder);
	region->Include(fRightOutlineBorder);
	region->Include(fBottomOutlineBorder);
}


//	#pragma mark - DecorAddOn


DecorAddOn::DecorAddOn(image_id id, const char* name)
	:
	fImageID(id)
{
}


DecorAddOn::~DecorAddOn()
{
}


Decorator*
DecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
{
	return NULL;
}


//	#pragma mark - StyleShim


/*!	A window's decorator, and what stands in for app_server around it. */
class StyleShim {
public:
	StyleShim(decor_canvas* canvas)
		:
		fSettings(canvas),
		fDecorator(NULL)
	{
		fEngine.fCanvas = canvas;
	}

	~StyleShim()
	{
		for (std::set<Decorator::Tab*>::iterator i = fTabs.begin();
				i != fTabs.end(); i++)
			delete *i;
		delete fDecorator;
	}

	static StyleShim* Of(decor_window* window)
	{
		return static_cast<StyleShim*>(window->cookie);
	}

	static status_t Create(decor_window* window, decor_canvas* canvas);

	/*!	Brings the decorator up to date with the window, as app_server
		keeps its own: the tabs and their state. Its rectangles are its own,
		kept up to date by its layout and moves.
	*/
	void Bind(decor_window* window, decor_canvas* canvas)
	{
		fEngine.fCanvas = canvas;
		fSettings = DesktopSettings(canvas);
		Decorator* decorator = fDecorator;
		decorator->fDrawingEngine = &fEngine;
		decorator->fDrawState.fCanvas = canvas;
		decorator->fDrawState.fFont.fCanvas = canvas;

		ShimList<Decorator::Tab>& list = decorator->fTabList;
		list.fCount = std::min(window->tab_count,
			(int32)ShimList<Decorator::Tab>::kMaxItems);
		for (int32 i = 0; i < list.fCount; i++) {
			decor_tab& styleTab = window->tabs[i];
			Decorator::Tab* tab = static_cast<Decorator::Tab*>(styleTab.cookie);
			if (tab == NULL || fTabs.find(tab) == fTabs.end()) {
				tab = decorator->_AllocateNewTab();
				fTabs.insert(tab);
				styleTab.cookie = tab;
			}
			list.fItems[i] = tab;

			tab->look = (window_look)styleTab.look;
			tab->flags = styleTab.flags;
			tab->isFocused = styleTab.focused;
			tab->closePressed = styleTab.pressed[DECOR_BUTTON_CLOSE];
			tab->zoomPressed = styleTab.pressed[DECOR_BUTTON_ZOOM];
			tab->minimizePressed = styleTab.pressed[DECOR_BUTTON_MINIMIZE];
			tab->isHighlighted = styleTab.highlighted;
			tab->tabLocation = styleTab.location;
			if (tab->title != styleTab.title)
				tab->title = styleTab.title;
			decorator->_SetFocus(tab);
		}
		decorator->fTopTab = list.ItemAt(window->top_tab >= 0
			? window->top_tab : 0);

		for (int32 region = 1; region < DECOR_REGION_COUNT; region++) {
			decorator->fRegionHighlights[region - 1]
				= window->highlights[region];
		}

		const BRect frame = to_rect(window->frame);
		sCornersCut = window->size >= offsetof(decor_window, corners_cut)
				+ sizeof(window->corners_cut)
			&& window->corners_cut;

		decorator->fOutlinesDelta = window->outline_resizing
			? to_rect(window->outline_frame).RightBottom()
				- frame.RightBottom()
			: BPoint(0, 0);
		decorator->_InvalidateFootprint();
	}

	//!	Bind() with the canvas it had, for calls that come without one.
	void Rebind(decor_window* window)
	{
		if (fEngine.fCanvas != NULL)
			Bind(window, fEngine.fCanvas);
	}

	//!	Frees tabs that are no longer the window's.
	void CollectTabs(decor_window* window)
	{
		std::set<Decorator::Tab*> used;
		for (int32 i = 0; i < window->tab_count; i++)
			used.insert(static_cast<Decorator::Tab*>(window->tabs[i].cookie));
		for (std::set<Decorator::Tab*>::iterator i = fTabs.begin();
				i != fTabs.end();) {
			if (used.find(*i) == used.end()) {
				delete *i;
				fTabs.erase(i++);
			} else
				i++;
		}
	}

	//!	Gives the engine the layout the decorator made.
	void WriteLayout(decor_window* window)
	{
		Decorator* decorator = fDecorator;
		window->border_width = std::max((int32)0, decorator->fBorderWidth);
		window->left_border = to_decor_rect(decorator->fLeftBorder);
		window->top_border = to_decor_rect(decorator->fTopBorder);
		window->right_border = to_decor_rect(decorator->fRightBorder);
		window->bottom_border = to_decor_rect(decorator->fBottomBorder);
		window->title_bar = to_decor_rect(decorator->fTitleBarRect);
		window->resize = to_decor_rect(decorator->fResizeRect);

		const ShimList<Decorator::Tab>& list = decorator->fTabList;
		for (int32 i = 0; i < list.CountItems(); i++) {
			const Decorator::Tab* tab = list.ItemAt(i);
			decor_tab& styleTab = window->tabs[i];
			styleTab.frame = to_decor_rect(tab->tabRect);
			styleTab.buttons[DECOR_BUTTON_CLOSE] = to_decor_rect(tab->closeRect);
			styleTab.buttons[DECOR_BUTTON_ZOOM] = to_decor_rect(tab->zoomRect);
			styleTab.buttons[DECOR_BUTTON_MINIMIZE]
				= to_decor_rect(tab->minimizeRect);
			styleTab.text_offset = tab->textOffset;
			styleTab.min_size = tab->minTabSize;
			styleTab.max_size = tab->maxTabSize;
			styleTab.location = tab->tabLocation;
			// the decorator shortens its titles itself
			styleTab.title_room = 1e6f;
			styleTab.truncate = DECOR_TRUNCATE_END;
		}

		// A lone tab narrower than the frame can slide along it, as on BeOS;
		// a title bar can't.
		const BRect border = decorator->fBorderRect;
		window->tabs_slide = list.CountItems() == 1
			&& list.ItemAt(0)->tabRect.IsValid() && border.IsValid()
			&& list.ItemAt(0)->tabRect.Width() < border.Width();
	}

	Decorator* Get() const { return fDecorator; }
	DesktopSettings& Settings() { return fSettings; }
	void SetFrame(BRect frame) { fDecorator->fFrame = frame; }

	// the decorator's own methods, which app_server would call
	static void DoLayout(Decorator* decorator)
		{ decorator->_DoLayout(); }
	static void DoOutlineLayout(Decorator* decorator)
		{ decorator->_DoOutlineLayout(); }
	static void UpdateFont(Decorator* decorator, DesktopSettings& settings)
		{ decorator->_UpdateFont(settings); }
	static void DrawFrame(Decorator* decorator, BRect rect)
		{ decorator->_DrawFrame(rect); }
	static void DrawOutlineFrame(Decorator* decorator, BRect rect)
		{ decorator->_DrawOutlineFrame(rect); }
	static void DrawTab(Decorator* decorator, Decorator::Tab* tab, BRect rect)
		{ decorator->_DrawTab(tab, rect); }
	static void DrawTitle(Decorator* decorator, Decorator::Tab* tab,
		BRect rect)
		{ decorator->_DrawTitle(tab, rect); }
	static void DrawClose(Decorator* decorator, Decorator::Tab* tab,
		bool direct, BRect rect)
		{ decorator->_DrawClose(tab, direct, rect); }
	static void DrawMinimize(Decorator* decorator, Decorator::Tab* tab,
		bool direct, BRect rect)
		{ decorator->_DrawMinimize(tab, direct, rect); }
	static void DrawZoom(Decorator* decorator, Decorator::Tab* tab,
		bool direct, BRect rect)
		{ decorator->_DrawZoom(tab, direct, rect); }
	static void MoveBy(Decorator* decorator, BPoint offset)
		{
			decorator->_MoveBy(offset);
			decorator->_MoveOutlineBy(offset);
		}
	static void GetFootprint(Decorator* decorator, BRegion* region)
		{ decorator->_GetFootprint(region); }

private:
	DrawingEngine				fEngine;
	DesktopSettings				fSettings;
	Decorator*					fDecorator;
	std::set<Decorator::Tab*>	fTabs;
};


static DecorAddOn* sAddOn = NULL;


/*static*/ status_t
StyleShim::Create(decor_window* window, decor_canvas* canvas)
{
	StyleShim* shim = new(std::nothrow) StyleShim(canvas);
	if (shim == NULL)
		return B_NO_MEMORY;

	shim->fDecorator = sAddOn->_AllocateDecorator(shim->fSettings,
		to_rect(window->frame), NULL);
	if (shim->fDecorator == NULL) {
		delete shim;
		return B_NO_MEMORY;
	}
	shim->fDecorator->fDrawingEngine = &shim->fEngine;
	shim->fDecorator->fDrawState.fCanvas = canvas;
	window->cookie = shim;
	return B_OK;
}


//	#pragma mark - the style's calls




static status_t
shim_create_window(decor_window* window, decor_canvas* canvas)
{
	return StyleShim::Create(window, canvas);
}


static void
shim_delete_window(decor_window* window)
{
	delete StyleShim::Of(window);
}


static void
shim_update_font(decor_window* window, decor_canvas* canvas)
{
	StyleShim* shim = StyleShim::Of(window);
	shim->Bind(window, canvas);
	if (shim->Get()->CountTabs() > 0)
		StyleShim::UpdateFont(shim->Get(), shim->Settings());
}


static void
shim_update_colors(decor_window* window, decor_canvas* canvas)
{
	StyleShim* shim = StyleShim::Of(window);
	shim->Bind(window, canvas);
	shim->Get()->UpdateColors(shim->Settings());
}


static void
shim_layout(decor_window* window, decor_canvas* canvas)
{
	StyleShim* shim = StyleShim::Of(window);
	shim->Bind(window, canvas);
	shim->CollectTabs(window);

	Decorator* decorator = shim->Get();
	shim->SetFrame(to_rect(window->frame));
	StyleShim::DoLayout(decorator);
	StyleShim::DoOutlineLayout(decorator);
	shim->WriteLayout(window);
}


static void
shim_moved(decor_window* window, float x, float y)
{
	StyleShim* shim = StyleShim::Of(window);
	StyleShim::MoveBy(shim->Get(), BPoint(x, y));
}


static void
shim_draw(decor_window* window, decor_canvas* canvas, decor_rect invalid)
{
	StyleShim* shim = StyleShim::Of(window);
	shim->Bind(window, canvas);
	shim->Get()->Draw(to_rect(invalid));
}


static void
shim_draw_frame(decor_window* window, decor_canvas* canvas,
	decor_rect invalid)
{
	StyleShim* shim = StyleShim::Of(window);
	shim->Bind(window, canvas);
	StyleShim::DrawFrame(shim->Get(), to_rect(invalid));
}


static void
shim_draw_tab(decor_window* window, decor_canvas* canvas, int32 index,
	decor_rect invalid)
{
	StyleShim* shim = StyleShim::Of(window);
	shim->Bind(window, canvas);
	Decorator::Tab* tab = shim->Get()->TabAt(index);
	if (tab != NULL)
		StyleShim::DrawTab(shim->Get(), tab, to_rect(invalid));
}


static void
shim_draw_title(decor_window* window, decor_canvas* canvas, int32 index)
{
	StyleShim* shim = StyleShim::Of(window);
	shim->Bind(window, canvas);
	Decorator::Tab* tab = shim->Get()->TabAt(index);
	if (tab != NULL)
		StyleShim::DrawTitle(shim->Get(), tab, tab->tabRect);
}


static void
shim_draw_button(decor_window* window, decor_canvas* canvas, int32 index,
	int32 button, decor_rect invalid)
{
	StyleShim* shim = StyleShim::Of(window);
	shim->Bind(window, canvas);
	Decorator* decorator = shim->Get();
	Decorator::Tab* tab = decorator->TabAt(index);
	if (tab == NULL)
		return;

	// the engine has set where the drawing goes
	const bool direct = canvas->copy_to_front(canvas);
	switch (button) {
		case DECOR_BUTTON_CLOSE:
			StyleShim::DrawClose(decorator, tab, direct, tab->closeRect);
			break;
		case DECOR_BUTTON_ZOOM:
			StyleShim::DrawZoom(decorator, tab, direct, tab->zoomRect);
			break;
		case DECOR_BUTTON_MINIMIZE:
			StyleShim::DrawMinimize(decorator, tab, direct,
				tab->minimizeRect);
			break;
	}
}


static void
shim_draw_outline(decor_window* window, decor_canvas* canvas,
	decor_rect invalid)
{
	StyleShim* shim = StyleShim::Of(window);
	shim->Bind(window, canvas);
	StyleShim::DrawOutlineFrame(shim->Get(), to_rect(invalid));
}


static int32
shim_region_at(decor_window* window, decor_point where, int32* tab)
{
	StyleShim* shim = StyleShim::Of(window);
	int32 index = -1;
	const Decorator::Region region
		= shim->Get()->RegionAt(BPoint(where.x, where.y), index);
	*tab = index;
	return region;
}


static void
shim_footprint(decor_window* window, decor_region* region)
{
	StyleShim* shim = StyleShim::Of(window);
	BRegion footprint;
	StyleShim::GetFootprint(shim->Get(), &footprint);
	for (int32 i = 0; i < footprint.CountRects(); i++)
		region->include(region, to_decor_rect(footprint.RectAt(i)));
}


static void
shim_size_limits(decor_window* window, int32* minWidth, int32* minHeight,
	int32* maxWidth, int32* maxHeight)
{
	StyleShim::Of(window)->Get()->GetSizeLimits(minWidth, minHeight, maxWidth,
		maxHeight);
}


static void
shim_rounded_shape(decor_window* window, decor_shape_rect* shape, int32 count)
{
	StyleShim* shim = StyleShim::Of(window);
	RoundedDecorator* rounded = dynamic_cast<RoundedDecorator*>(shim->Get());
	if (rounded == NULL)
		return;

	shim->Rebind(window);
	std::vector<Decorator::ShapeRect> decoratorShape;
	rounded->GetRoundedShape(decoratorShape);

	// The engine's shape and the decorator's come from the same layout, in
	// the same order: the frame, then each tab.
	for (int32 i = 0; i < count && i < (int32)decoratorShape.size(); i++) {
		if (to_decor_rect(decoratorShape[i].rect).left != shape[i].rect.left
			|| to_decor_rect(decoratorShape[i].rect).top
				!= shape[i].rect.top) {
			continue;
		}
		shape[i].left_top = (int32)decoratorShape[i].leftTop;
		shape[i].right_top = (int32)decoratorShape[i].rightTop;
		shape[i].left_bottom = (int32)decoratorShape[i].leftBottom;
		shape[i].right_bottom = (int32)decoratorShape[i].rightBottom;
	}
}


static decor_style sStyle;


extern "C" const decor_style*
instantiate_decor_style(uint32 apiVersion)
{
	if (apiVersion < DECOR_STYLE_API_VERSION)
		return NULL;

	if (sAddOn == NULL) {
		sAddOn = shim_instantiate_decor_addon(-1, SHIM_STYLE_NAME);
		if (sAddOn == NULL || sAddOn->InitCheck() != B_OK)
			return NULL;
	}

	memset(&sStyle, 0, sizeof(sStyle));
	sStyle.size = sizeof(sStyle);
	sStyle.api_version = DECOR_STYLE_API_VERSION;
	sStyle.name = SHIM_STYLE_NAME;
	sStyle.create_window = &shim_create_window;
	sStyle.delete_window = &shim_delete_window;
	sStyle.update_font = &shim_update_font;
	sStyle.layout = &shim_layout;
	sStyle.draw_frame = &shim_draw_frame;
	sStyle.draw_tab = &shim_draw_tab;
	sStyle.draw_title = &shim_draw_title;
	sStyle.draw_button = &shim_draw_button;
	sStyle.draw_outline = &shim_draw_outline;
	sStyle.region_at = &shim_region_at;
	sStyle.footprint = &shim_footprint;
	sStyle.draw = &shim_draw;
	sStyle.size_limits = &shim_size_limits;
	sStyle.moved = &shim_moved;
	sStyle.update_colors = &shim_update_colors;
	sStyle.rounded_shape = &shim_rounded_shape;
	return &sStyle;
}
