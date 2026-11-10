/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Opens one window of each kind a decorator draws, at fixed places, so that
 * two decorators can be screenshotted and compared pixel for pixel:
 *
 *	g++ -o decor-test tools/decor-test.cpp -lbe
 *	./decor-test &
 *
 * The document window comes up last, so it's the active one.
 */


#include <Application.h>
#include <View.h>
#include <Window.h>


static const window_look kLeftTitledWindowLook = window_look(25);


class TestWindow : public BWindow {
public:
	TestWindow(BRect frame, const char* title, window_look look,
		window_feel feel, uint32 flags)
		:
		BWindow(frame, title, look, feel, flags | B_QUIT_ON_WINDOW_CLOSE)
	{
		BView* view = new BView(Bounds(), "plain", B_FOLLOW_ALL, 0);
		view->SetViewColor(255, 255, 255);
		AddChild(view);
	}
};


int
main()
{
	BApplication app("application/x-vnd.idumych-decor-test");

	struct {
		BRect			frame;
		const char*		title;
		window_look		look;
		uint32			flags;
	} windows[] = {
		{ BRect(40, 50, 330, 190), "Titled", B_TITLED_WINDOW_LOOK, 0 },
		{ BRect(40, 250, 330, 330), "Floating", B_FLOATING_WINDOW_LOOK, 0 },
		{ BRect(40, 380, 330, 460), "Modal", B_MODAL_WINDOW_LOOK, 0 },
		{ BRect(40, 510, 330, 570), "Bordered", B_BORDERED_WINDOW_LOOK, 0 },
		{ BRect(40, 620, 330, 700), "Not resizable or zoomable",
			B_TITLED_WINDOW_LOOK, B_NOT_RESIZABLE | B_NOT_ZOOMABLE },
		{ BRect(420, 520, 560, 600),
			"A title far too long to fit in the window's tab",
			B_TITLED_WINDOW_LOOK, 0 },
		{ BRect(640, 520, 940, 600), "Left titled", kLeftTitledWindowLook, 0 },
		{ BRect(420, 50, 940, 440), "Document", B_DOCUMENT_WINDOW_LOOK, 0 },
	};

	for (size_t i = 0; i < sizeof(windows) / sizeof(windows[0]); i++) {
		TestWindow* window = new TestWindow(windows[i].frame,
			windows[i].title, windows[i].look, B_NORMAL_WINDOW_FEEL,
			windows[i].flags);
		window->Show();
		snooze(100000);
	}

	app.Run();
	return 0;
}
