/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * The "Build window borders" window. See BuildWindow.h.
 */


#include "BuildWindow.h"

#include <errno.h>
#include <algorithm>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <Alert.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <StatusBar.h>
#include <StringView.h>
#include <TextView.h>

#include "Theme.h"


enum {
	kMsgBuild		= 'bild',
	kMsgLine		= 'line',
	kMsgFinished	= 'fini'
};

// build-decorators.sh exits with this when the compiler isn't installed
static const int kNeedTools = 3;


struct RunJob {
	BMessenger		window;
	int				fd;
	pid_t			child;
};


BuildWindow::BuildWindow(BMessenger target)
	:
	BWindow(BRect(0, 0, 520, 420), "Build window borders",
		B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_ZOOMABLE),
	fTarget(target),
	fChild(-1),
	fRunning(false),
	fStopping(false)
{
	fIntro = new BTextView("intro");
	fIntro->MakeEditable(false);
	fIntro->MakeSelectable(false);
	fIntro->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fIntro->SetLowUIColor(B_PANEL_BACKGROUND_COLOR);
	fIntro->SetStylable(false);
	fIntro->SetWordWrap(true);
	fIntro->SetText("Window borders run inside app_server, so they have to "
		"be built for this exact system. This compiles Reactionary, Smoke, "
		"Gonx and, if the system lacks it, Haiku's Flat decorator, against "
		"the running system. When the headers that come with Themes are from "
		"another system build, it fetches this build's own from its source "
		"repository first, which needs a network connection.");
	fIntro->SetExplicitMinSize(BSize(be_plain_font->Size() * 34,
		be_plain_font->Size() * 6.5));

	fProgress = new BStatusBar("progress");
	fProgress->SetMaxValue(100.0f);
	fStatus = new BStringView("status", "");
	fStatus->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fStatus->SetTruncation(B_TRUNCATE_END);

	fLog = new BTextView("log");
	fLog->MakeEditable(false);
	fLog->SetStylable(false);
	BFont fixed(be_fixed_font);
	fixed.SetSize(std::max(9.0f, be_fixed_font->Size() - 1));
	fLog->SetFontAndColor(&fixed);
	BScrollView* logScroll = new BScrollView("log scroll", fLog, 0, false,
		true);
	logScroll->SetExplicitMinSize(BSize(B_SIZE_UNSET,
		be_fixed_font->Size() * 12));

	fClose = new BButton("close", "Close", new BMessage(B_QUIT_REQUESTED));
	fBuild = new BButton("build", "Build", new BMessage(kMsgBuild));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(fIntro)
		.Add(fProgress)
		.Add(fStatus)
		.Add(logScroll, 1)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fClose)
			.Add(fBuild)
		.End()
	.End();

	fBuild->MakeDefault(true);
	CenterOnScreen();

	if (find_build_script(fScript) != B_OK) {
		fBuild->SetEnabled(false);
		fStatus->SetText("build-decorators.sh isn't installed with Themes.");
	} else
		fStatus->SetText("Ready to build.");
}


BuildWindow::~BuildWindow()
{
}


void
BuildWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgBuild:
			if (fRunning)
				_Stop();
			else
				_Start(false);
			break;

		case kMsgLine:
			_Line(message->GetString("line", ""));
			break;

		case kMsgFinished:
			_Finished(message->GetInt32("status", 1));
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


bool
BuildWindow::QuitRequested()
{
	if (fRunning) {
		// stop the build first; the window closes when it has
		_Stop();
		return false;
	}
	return true;
}


/*!	Reads the script's output line by line, for the window, and waits
	for it to end.
*/
status_t
BuildWindow::_RunThread(void* data)
{
	RunJob* job = (RunJob*)data;
	FILE* output = fdopen(job->fd, "r");
	char buffer[2048];
	while (output != NULL && fgets(buffer, sizeof(buffer), output) != NULL) {
		BString line(buffer);
		line.RemoveAll("\n");
		BMessage message(kMsgLine);
		message.AddString("line", line);
		job->window.SendMessage(&message);
	}
	if (output != NULL)
		fclose(output);
	else
		close(job->fd);

	int status = 0;
	int result = 1;
	if (waitpid(job->child, &status, 0) == job->child) {
		if (WIFEXITED(status))
			result = WEXITSTATUS(status);
		else
			result = -1;
	}
	BMessage finished(kMsgFinished);
	finished.AddInt32("status", result);
	job->window.SendMessage(&finished);
	delete job;
	return B_OK;
}


void
BuildWindow::_Start(bool installTools)
{
	int pipes[2];
	if (pipe(pipes) != 0) {
		fStatus->SetText(strerror(errno));
		return;
	}

	pid_t child = fork();
	if (child < 0) {
		close(pipes[0]);
		close(pipes[1]);
		fStatus->SetText(strerror(errno));
		return;
	}
	if (child == 0) {
		// its own process group, so that Stop reaches what it runs too
		setpgid(0, 0);
		dup2(pipes[1], 1);
		dup2(pipes[1], 2);
		close(pipes[0]);
		close(pipes[1]);
		execl("/bin/sh", "sh", fScript.String(),
			installTools ? "--install-tools" : (char*)NULL, (char*)NULL);
		_exit(127);
	}
	close(pipes[1]);

	RunJob* job = new RunJob;
	job->window = BMessenger(this);
	job->fd = pipes[0];
	job->child = child;
	thread_id thread = spawn_thread(_RunThread, "build output",
		B_NORMAL_PRIORITY, job);
	if (thread < 0) {
		kill(-child, SIGTERM);
		close(pipes[0]);
		delete job;
		return;
	}

	fChild = child;
	fRunning = true;
	fStopping = false;
	fError = "";
	fLog->SetText("");
	fProgress->Reset();
	fStatus->SetText("Starting" B_UTF8_ELLIPSIS);
	fBuild->SetLabel("Stop");
	fClose->SetEnabled(false);
	resume_thread(thread);
}


void
BuildWindow::_Stop()
{
	if (!fRunning || fChild < 0)
		return;
	fStopping = true;
	fStatus->SetText("Stopping" B_UTF8_ELLIPSIS);
	kill(-fChild, SIGTERM);
}


/*!	A line of the script's output: "STEP <percent> <what>" moves the
	progress bar, "ERROR <what>" is the failure to show; the rest goes to
	the log.
*/
void
BuildWindow::_Line(const BString& line)
{
	if (line.StartsWith("STEP ")) {
		BString rest;
		line.CopyInto(rest, 5, line.Length() - 5);
		const int32 space = rest.FindFirst(' ');
		const float percent = atof(rest.String());
		BString what;
		if (space >= 0)
			rest.CopyInto(what, space + 1, rest.Length() - space - 1);
		if (percent > fProgress->CurrentValue())
			fProgress->Update(percent - fProgress->CurrentValue());
		if (!what.IsEmpty()) {
			BString status(what);
			if (percent < 100)
				status << B_UTF8_ELLIPSIS;
			fStatus->SetText(status);
			_AddToLog(BString("== ") << what << "\n");
		}
		return;
	}
	if (line.StartsWith("ERROR ")) {
		line.CopyInto(fError, 6, line.Length() - 6);
		_AddToLog(BString("Error: ") << fError << "\n");
		return;
	}
	if (line.StartsWith("NEED_TOOLS"))
		return;
	_AddToLog(BString(line) << "\n");
}


void
BuildWindow::_Finished(int status)
{
	fRunning = false;
	fChild = -1;
	fBuild->SetLabel("Build");
	fClose->SetEnabled(true);

	if (fStopping) {
		fStatus->SetText("Stopped. Nothing was installed unless the log "
			"says so.");
		fStopping = false;
		return;
	}

	if (status == 0) {
		fProgress->Update(100.0f - fProgress->CurrentValue());
		fStatus->SetText("Built. Themes with these window borders can be "
			"applied now.");
		fTarget.SendMessage(kMsgDecoratorsBuilt);
		return;
	}

	if (status == kNeedTools) {
		fStatus->SetText("The compiler isn't installed.");
		BAlert* alert = new BAlert("Build window borders",
			"Building the window borders needs the C++ compiler (the gcc and "
			"haiku_devel packages). Install them now with pkgman? This "
			"needs a network connection.",
			"Cancel", "Install and build", NULL, B_WIDTH_AS_USUAL,
			B_IDEA_ALERT);
		alert->SetShortcut(0, B_ESCAPE);
		if (alert->Go() == 1)
			_Start(true);
		return;
	}

	BString text("Failed");
	if (!fError.IsEmpty())
		text << ": " << fError;
	text << ". See the log.";
	fStatus->SetText(text);
}


void
BuildWindow::_AddToLog(const BString& text)
{
	fLog->Insert(fLog->TextLength(), text.String(), text.Length());
	fLog->ScrollToOffset(fLog->TextLength());
}
