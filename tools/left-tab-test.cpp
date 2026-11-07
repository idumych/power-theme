/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * A window with its tab on the left, at a known place, for the test plan's
 * flat-tab check. app_server has no public name for this look.
 *
 *   g++ left-tab-test.cpp -o left-tab-test -lbe
 */

#include <Application.h>
#include <Window.h>

static const window_look kLeftTitledWindowLook = (window_look)25;

int
main()
{
	BApplication app("application/x-vnd.idumych-left-tab-test");
	BWindow* window = new BWindow(BRect(200, 200, 400, 400), "L",
		kLeftTitledWindowLook, B_NORMAL_WINDOW_FEEL, B_QUIT_ON_WINDOW_CLOSE);
	window->Show();
	app.Run();
	return 0;
}
