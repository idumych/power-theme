/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Window decorator after Hideaki Kamada's Notebook skin for WindowBlinds (1999),
 * drawn from its pictures (NotebookWindowArt.h, from ../art), laid out as its
 * notebook.uis has them: a sheet of ruled paper with a title bar across the top,
 * a close box at its left and minimize and zoom boxes at its right, a spiral
 * binding with punched holes down the left, a rounded edge on the right and the
 * pages' edge along the bottom. An inactive window's title bar is scribbled on.
 * The holes are left out of the window's footprint, so what's behind shows
 * through them.
 */
#ifndef NOTEBOOK_DECORATOR_H
#define NOTEBOOK_DECORATOR_H


#include <vector>

#include "Decorator.h"
#include "DecorManager.h"


struct Art;
class Desktop;


class NotebookDecorAddOn : public DecorAddOn {
public:
								NotebookDecorAddOn(image_id id, const char* name);

protected:
	virtual	Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, Desktop* desktop);
};


class NotebookDecorator : public Decorator {
public:
								NotebookDecorator(DesktopSettings& settings,
									BRect frame, Desktop* desktop);
	virtual						~NotebookDecorator();

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
	// a part of a picture, drawn at a place, as much of it as is in clip
	struct Piece {
		const Art*				art;
		BRect					source;
		BPoint					where;
		BRect					clip;
	};

			bool				_HasTitle() const;
			void				_LayoutTabs();
			BRect				_TitleRect(const Decorator::Tab* tab) const;

			void				_FramePieces(std::vector<Piece>& pieces,
									bool focus) const;
			void				_BarPieces(std::vector<Piece>& pieces,
									const BRect& clip, bool focus) const;
	static	void				_Repeat(std::vector<Piece>& pieces,
									const Art& art, BRect source,
									const BRect& area, const BRect& clip);
			void				_Draw(const Piece& piece, const BRect& clip);
	static	void				_AddOpaque(BRegion* region,
									const Piece& piece);
			void				_DrawButton(Decorator::Tab* tab, bool direct,
									BRect rect, const Art& normal,
									const Art& pressed, const Art& inactive,
									bool down);

			rgb_color			fFocusTextColor;
			rgb_color			fNonFocusTextColor;
};


#endif	// NOTEBOOK_DECORATOR_H
