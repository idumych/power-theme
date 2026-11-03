/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * The "Build add-ons" window: runs build-decorators.sh, which compiles the
 * window decorators and control looks against the running system
 * (fetching its headers when the bundled ones are from another build), and
 * shows its steps in a progress bar and its output in a log.
 */
#ifndef BUILD_WINDOW_H
#define BUILD_WINDOW_H


#include <Messenger.h>
#include <String.h>
#include <Window.h>


class BButton;
class BStatusBar;
class BStringView;
class BTextView;


// sent to the target when decorators were built
enum {
	kMsgDecoratorsBuilt = 'dbld'
};


class BuildWindow : public BWindow {
public:
								BuildWindow(BMessenger target);
	virtual						~BuildWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

private:
	static	status_t			_RunThread(void* data);

			void				_Start(bool installTools);
			void				_Stop();
			void				_Line(const BString& line);
			void				_Finished(int status);
			void				_AddToLog(const BString& text);

			BMessenger			fTarget;
			BString				fScript;
			pid_t				fChild;
			bool				fRunning;
			bool				fStopping;
			BString				fError;
			BString				fNeedTools;

			BTextView*			fIntro;
			BStatusBar*			fProgress;
			BStringView*		fStatus;
			BTextView*			fLog;
			BButton*			fClose;
			BButton*			fBuild;
};


#endif	// BUILD_WINDOW_H
