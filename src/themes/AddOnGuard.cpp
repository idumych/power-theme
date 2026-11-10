/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Linked into every window decorator and control look that
 * build-decorators.sh builds. They use private classes of app_server and
 * libbe, so one built for another system build can crash app_server, and
 * with it the desktop, as after a system update. This is the add-on's entry
 * point: it hands over to the add-on's own (which the script renames
 * pt_guarded_instantiate_...) only on the system build it was built for, and
 * otherwise returns NULL, so that app_server falls back to its own decorator
 * and applications to Haiku's controls until the add-ons are built again.
 *
 * The script passes the size and modification time of the app_server and
 * libbe it built against, PT_BUILT_APP_SERVER and PT_BUILT_LIBBE as
 * "SIZE MTIME", and PT_GUARD_DECORATOR or PT_GUARD_CONTROL_LOOK.
 */


#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <syslog.h>

#include <image.h>
#include <OS.h>


static bool
same_file(const char* path, const char* built)
{
	struct stat st;
	if (stat(path, &st) != 0)
		return false;
	char running[64];
	snprintf(running, sizeof(running), "%lld %lld", (long long)st.st_size,
		(long long)st.st_mtime);
	return strcmp(running, built) == 0;
}


/*!	Whether the running team's app_server (its program, when \a program) or
	libbe is the one this add-on was built against.
*/
static bool
built_for(bool program, const char* built)
{
	image_info info;
	int32 cookie = 0;
	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &info) == B_OK) {
		if (program ? info.type == B_APP_IMAGE
				: strcmp(strrchr(info.name, '/') != NULL
					? strrchr(info.name, '/') + 1 : info.name, "libbe.so") == 0)
			return same_file(info.name, built);
	}
	return false;
}


static void
refuse(const char* name)
{
	syslog(LOG_WARNING, "%s was built for another system build: not loading "
		"it; build the add-ons again in Power Theme", name);
}


#ifdef PT_GUARD_DECORATOR

extern "C" void* pt_guarded_instantiate_decor_addon(image_id id,
	const char* name);


extern "C" void*
instantiate_decor_addon(image_id id, const char* name)
{
	if (!built_for(true, PT_BUILT_APP_SERVER)
		|| !built_for(false, PT_BUILT_LIBBE)) {
		refuse(name);
		return NULL;
	}
	return pt_guarded_instantiate_decor_addon(id, name);
}

#endif	// PT_GUARD_DECORATOR


#ifdef PT_GUARD_CONTROL_LOOK

extern "C" void* pt_guarded_instantiate_control_look(image_id id);


extern "C" void*
instantiate_control_look(image_id id)
{
	if (!built_for(false, PT_BUILT_LIBBE)) {
		image_info info;
		refuse(get_image_info(id, &info) == B_OK
			? info.name : "A control look");
		return NULL;
	}
	return pt_guarded_instantiate_control_look(id);
}

#endif	// PT_GUARD_CONTROL_LOOK
