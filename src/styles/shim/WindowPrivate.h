// The values of libbe's private window looks, feels and flags that the
// decorators use: see StyleShim.h.
#ifndef STYLE_SHIM_WINDOW_PRIVATE_H
#define STYLE_SHIM_WINDOW_PRIVATE_H

#include <Window.h>

const window_look kDesktopWindowLook = window_look(4);
const window_look kLeftTitledWindowLook = window_look(25);
const window_feel kDesktopWindowFeel = window_feel(1024);
const uint32 kWindowScreenFlag = 0x10000;
const uint32 kAcceptKeyboardFocusFlag = 0x40000;

#endif
