/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Apart from Theme.cpp, so that PowerShelf can find icon themes without the
 * rest of it.
 */


#include "Theme.h"

#include <FindDirectory.h>
#include <Path.h>
#include <StringList.h>


void
data_directories(BStringList& directories)
{
	static const directory_which kDirectories[] = {
		B_USER_NONPACKAGED_DATA_DIRECTORY,
		B_USER_DATA_DIRECTORY,
		B_SYSTEM_NONPACKAGED_DATA_DIRECTORY,
		B_SYSTEM_DATA_DIRECTORY
	};

	for (size_t i = 0; i < sizeof(kDirectories) / sizeof(kDirectories[0]);
			i++) {
		BPath path;
		if (find_directory(kDirectories[i], &path) == B_OK)
			directories.Add(path.Path());
	}
}
