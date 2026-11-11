// Stands in for app_server's RoundedDecorator.h: see StyleShim.h. With it,
// a decorator's code for rounded corners on a composited desktop is built
// into the style, and app_server's engine cuts the corners off through
// decor_style::rounded_shape.
#ifndef STYLE_SHIM_ROUNDED_DECORATOR_H
#define STYLE_SHIM_ROUNDED_DECORATOR_H

#include <vector>

#include "StyleShim.h"


class WindowShape {
public:
	enum corner_kind {
		LEFT_TOP,
		RIGHT_TOP,
		LEFT_BOTTOM,
		RIGHT_BOTTOM
	};
};


class RoundedDecorator {
public:
	virtual						~RoundedDecorator() {}
	virtual	void				GetRoundedShape(
									std::vector<Decorator::ShapeRect>& shape)
										const = 0;

	// Whether app_server cuts the corners off the window being drawn.
	static	bool				IsActive(Desktop* desktop);
	static	void				RoundShape(
									std::vector<Decorator::ShapeRect>& shape,
									window_look look, int32 frameRadius,
									int32 tabRadius);
	static	void				StrokeCorners(DrawingEngine* engine,
									const Decorator::ShapeRect& shape,
									rgb_color leftTop, rgb_color rightTop,
									rgb_color leftBottom,
									rgb_color rightBottom);
	static	void				StrokeCorner(DrawingEngine* engine,
									BPoint leftTop, int32 radius,
									WindowShape::corner_kind kind,
									rgb_color color);
};

#endif
