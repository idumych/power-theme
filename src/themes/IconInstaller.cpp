/*
 * Copyright 2026, idumych.
 * Distributed under the terms of the MIT License.
 *
 * Icon themes on file types and folders. See IconInstaller.h.
 */


#include "IconInstaller.h"

#include <stdio.h>
#include <string.h>

#include <map>
#include <vector>

#include <Entry.h>
#include <fs_attr.h>
#include <File.h>
#include <FindDirectory.h>
#include <Mime.h>
#include <Node.h>
#include <Path.h>
#include <StringList.h>

#include "DarkIcons.h"
#include "HVIFEncoder.h"
#include "IconTheme.h"
#include "Theme.h"


static const char* kBackupName = "icons";
static const char* kNodeIconAttribute = "BEOS:ICON";


struct IconTarget {
	bool		isType;
	BString		target;
		// a MIME type, or a folder's path
	BStringList	names;
};


//	#pragma mark - the icon map


static bool
find_icon_map(BPath& path)
{
	BStringList directories;
	data_directories(directories);
	for (int32 i = 0; i < directories.CountStrings(); i++) {
		path.SetTo(directories.StringAt(i), "PowerTheme/icon-map");
		if (BEntry(path.Path()).Exists())
			return true;
	}
	return false;
}


static status_t
load_icon_map(std::vector<IconTarget>& targets, BString& log)
{
	BPath path;
	if (!find_icon_map(path)) {
		log << "The icon map (data/PowerTheme/icon-map) isn't installed.\n";
		return B_ENTRY_NOT_FOUND;
	}

	BFile file(path.Path(), B_READ_ONLY);
	off_t size;
	if (file.InitCheck() != B_OK || file.GetSize(&size) != B_OK
		|| size > 1024 * 1024)
		return B_BAD_DATA;
	BString text;
	char* buffer = text.LockBuffer(size + 1);
	ssize_t bytesRead = file.Read(buffer, size);
	text.UnlockBuffer(bytesRead > 0 ? bytesRead : 0);

	BPath home;
	find_directory(B_USER_DIRECTORY, &home);

	std::vector<IconTarget> wildcards;
	BStringList named;
	BStringList lines;
	text.Split("\n", false, lines);
	for (int32 i = 0; i < lines.CountStrings(); i++) {
		BString line = lines.StringAt(i);
		line.Trim();
		if (line.IsEmpty() || line.StartsWith("#"))
			continue;
		int32 equals = line.FindFirst('=');
		int32 space = line.FindFirst(' ');
		if (equals < 0 || space < 0 || space > equals)
			continue;

		BString kind, target, names;
		line.CopyInto(kind, 0, space);
		line.CopyInto(target, space + 1, equals - space - 1);
		line.CopyInto(names, equals + 1, line.Length() - equals - 1);
		target.Trim();

		IconTarget entry;
		entry.isType = kind == "mime";
		if (!entry.isType && kind != "folder")
			continue;
		names.Split(" ", true, entry.names);
		entry.names.Remove("");
		if (entry.names.IsEmpty() || target.IsEmpty())
			continue;

		if (!entry.isType) {
			if (target == "~")
				target = home.Path();
			else if (target.StartsWith("~/"))
				target.Replace("~", home.Path(), 1);
			if (!BEntry(target).Exists())
				continue;
		}
		entry.target = target;

		if (entry.isType && target.EndsWith("/*"))
			wildcards.push_back(entry);
		else {
			if (entry.isType) {
				if (!BMimeType(target).IsInstalled())
					continue;
				BString lower(target);
				named.Add(lower.ToLower());
			}
			targets.push_back(entry);
		}
	}

	// every installed type of a group not named on its own line
	for (size_t i = 0; i < wildcards.size(); i++) {
		BString supertype(wildcards[i].target);
		supertype.RemoveLast("/*");
		BMessage types;
		if (BMimeType::GetInstalledTypes(supertype, &types) != B_OK)
			continue;
		const char* type;
		for (int32 t = 0; types.FindString("types", t, &type) == B_OK; t++) {
			BString lower(type);
			if (named.HasString(lower.ToLower()))
				continue;
			named.Add(lower);
			IconTarget entry = wildcards[i];
			entry.target = type;
			targets.push_back(entry);
		}
	}
	return B_OK;
}


//	#pragma mark - saving and restoring


static status_t
load_backup(BMessage& backup)
{
	BPath path;
	status_t status = themes_settings_path(path, kBackupName);
	if (status != B_OK)
		return status;
	BFile file(path.Path(), B_READ_ONLY);
	status = file.InitCheck();
	if (status != B_OK)
		return status;
	return backup.Unflatten(&file);
}


static status_t
save_backup(const BMessage& backup)
{
	BPath path;
	status_t status = themes_settings_path(path, kBackupName);
	if (status != B_OK)
		return status;
	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status = file.InitCheck();
	if (status != B_OK)
		return status;
	return backup.Flatten(&file);
}


bool
has_saved_icons()
{
	BMessage backup;
	return load_backup(backup) == B_OK && !backup.IsEmpty();
}


//! The user's own copy of a type in the MIME database, if any.
static BPath
user_type_path(const char* type)
{
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("mime_db");
	BString lower(type);
	path.Append(lower.ToLower());
	return path;
}


static BString
backup_key(const IconTarget& target)
{
	BString key(target.isType ? "mime:" : "folder:");
	if (target.isType) {
		BString lower(target.target);
		key << lower.ToLower();
	} else
		key << target.target;
	return key;
}


/*!	The preferred application of \a type, if it has an icon of its own
	for files of that type: Tracker shows that one before the type's.
*/
static bool
app_with_icon_for(const char* type, BString& app)
{
	char signature[B_MIME_TYPE_LENGTH];
	if (BMimeType(type).GetPreferredApp(signature) != B_OK)
		return false;
	uint8* data = NULL;
	size_t size = 0;
	if (BMimeType(signature).GetIconForType(type, &data, &size) != B_OK
		|| data == NULL)
		return false;
	delete[] data;
	app = signature;
	return true;
}


/*!	What \a target looks like now: its icon data, if it has one; for a
	type whether the user's MIME database had its own copy of it, and the
	icon its preferred application has for it, if any.
*/
static void
record(const IconTarget& target, BMessage& entry)
{
	entry.AddBool("type", target.isType);
	entry.AddString("target", target.target);

	if (target.isType) {
		entry.AddBool("existed",
			BEntry(user_type_path(target.target).Path()).Exists());
		uint8* data = NULL;
		size_t size = 0;
		if (BMimeType(target.target).GetIcon(&data, &size) == B_OK
			&& data != NULL) {
			entry.AddData("icon", B_RAW_TYPE, data, size);
		}
		delete[] data;

		BString app;
		if (app_with_icon_for(target.target, app)) {
			data = NULL;
			BMimeType(app).GetIconForType(target.target, &data, &size);
			entry.AddString("app", app);
			entry.AddBool("app existed",
				BEntry(user_type_path(app).Path()).Exists());
			entry.AddData("app icon", B_RAW_TYPE, data, size);
			delete[] data;
		}
		return;
	}

	BNode node(target.target);
	attr_info info;
	if (node.GetAttrInfo(kNodeIconAttribute, &info) == B_OK
		&& info.size > 0 && info.size < 1024 * 1024) {
		std::vector<uint8> data(info.size);
		if (node.ReadAttr(kNodeIconAttribute, info.type, 0, &data[0],
				info.size) == info.size) {
			entry.AddData("icon", B_RAW_TYPE, &data[0], info.size);
		}
	}
}


static void
restore(const BMessage& entry)
{
	const char* target = entry.GetString("target", NULL);
	if (target == NULL)
		return;
	const void* data = NULL;
	ssize_t size = 0;
	entry.FindData("icon", B_RAW_TYPE, &data, &size);

	if (entry.GetBool("type", false)) {
		const char* app = entry.GetString("app", NULL);
		const void* appData = NULL;
		ssize_t appSize = 0;
		if (app != NULL
			&& entry.FindData("app icon", B_RAW_TYPE, &appData, &appSize)
				== B_OK) {
			BMimeType(app).SetIconForType(target, (const uint8*)appData,
				appSize);
			if (!entry.GetBool("app existed", true))
				BEntry(user_type_path(app).Path()).Remove();
		}

		BMimeType type(target);
		if (data != NULL)
			type.SetIcon((const uint8*)data, size);
		else
			type.SetIcon(NULL, 0);
		// The copy was only made to hold the icon: remove it, so the
		// system's type shows through again, updates included.
		if (!entry.GetBool("existed", true))
			BEntry(user_type_path(target).Path()).Remove();
		return;
	}

	BNode node(target);
	if (data != NULL) {
		node.WriteAttr(kNodeIconAttribute, B_VECTOR_ICON_TYPE, 0, data,
			size);
	} else
		node.RemoveAttr(kNodeIconAttribute);
}


static status_t
set_icon(const IconTarget& target, const std::vector<uint8_t>& data)
{
	if (target.isType) {
		// and where its preferred application has an icon for it, which
		// Tracker shows first, that too
		BString app;
		if (app_with_icon_for(target.target, app)) {
			BMimeType(app).SetIconForType(target.target, &data[0],
				data.size());
		}
		return BMimeType(target.target).SetIcon(&data[0], data.size());
	}

	BNode node(target.target);
	status_t status = node.InitCheck();
	if (status != B_OK)
		return status;
	ssize_t written = node.WriteAttr(kNodeIconAttribute, B_VECTOR_ICON_TYPE,
		0, &data[0], data.size());
	return written < 0 ? (status_t)written : B_OK;
}


//	#pragma mark -


status_t
apply_icon_theme(const IconTheme& theme, BString& log, int32& changed,
	const ProgressHook& progress)
{
	changed = 0;

	std::vector<IconTarget> targets;
	status_t status = load_icon_map(targets, log);
	if (status != B_OK)
		return status;

	// convert first, so nothing changes if it can't be done
	std::map<BString, std::vector<uint8_t> > converted;
		// found icon name -> HVIF
	std::map<BString, BString> lookups;
		// names -> the one found, or "" if none
	std::vector<BString> chosen(targets.size());
	int32 missing = 0;
	for (size_t i = 0; i < targets.size(); i++) {
		if (progress) {
			progress(0.8f * i / targets.size(),
				targets[i].target.String());
		}

		// Many targets have the same names (a whole group of types), and
		// many names lead to the same icon: look each up once.
		const BString key = targets[i].names.Join(" ");
		std::map<BString, BString>::iterator known = lookups.find(key);
		if (known == lookups.end()) {
			BString found;
			std::vector<IconImage> images;
			if (theme.Images(targets[i].names, images, &found)
				&& converted.find(found) == converted.end()) {
				HVIFResult result;
				std::string error;
				if (encode_hvif(images, result, &error))
					converted[found] = result.data;
				else {
					log << "The icon \"" << found << "\" can't be used: "
						<< error.c_str() << ".\n";
					found = "";
				}
			}
			known = lookups.insert(std::make_pair(key, found)).first;
		}
		const BString& found = known->second;
		if (found.IsEmpty()) {
			missing++;
			continue;
		}
		chosen[i] = found;
	}

	BMessage backup;
	load_backup(backup);

	// Save what is about to change before changing it.
	for (size_t i = 0; i < targets.size(); i++) {
		if (chosen[i].IsEmpty())
			continue;
		const BString key = backup_key(targets[i]);
		if (backup.HasMessage(key))
			continue;
		BMessage entry;
		record(targets[i], entry);
		backup.AddMessage(key, &entry);
	}
	status = save_backup(backup);
	if (status != B_OK) {
		log << "Can't save the icons in use, so none were changed: "
			<< strerror(status) << ".\n";
		return status;
	}

	BStringList setKeys;
	for (size_t i = 0; i < targets.size(); i++) {
		if (chosen[i].IsEmpty())
			continue;
		if (progress) {
			progress(0.8f + 0.2f * i / targets.size(),
				targets[i].target.String());
		}
		if (set_icon(targets[i], converted[chosen[i]]) == B_OK) {
			changed++;
			setKeys.Add(backup_key(targets[i]));
		} else
			log << "Can't set the icon of " << targets[i].target << ".\n";
	}

	// what an earlier icon theme changed and this one doesn't cover gets
	// its own icon back
	BMessage kept;
	char* name;
	type_code type;
	int32 count;
	for (int32 i = 0; backup.GetInfo(B_MESSAGE_TYPE, i, &name, &type, &count)
			== B_OK; i++) {
		BMessage entry;
		if (backup.FindMessage(name, &entry) != B_OK)
			continue;
		if (setKeys.HasString(name))
			kept.AddMessage(name, &entry);
		else
			restore(entry);
	}
	save_backup(kept);

	if (missing > 0) {
		log << missing << " file types and folders kept their icons: \""
			<< theme.Name() << "\" has none for them.\n";
	}
	set_current(NULL, theme.ID());
	if (progress)
		progress(1.0f, "");
	return B_OK;
}


status_t
apply_dark_haiku_icons(BString& log, int32& changed,
	const ProgressHook& progress)
{
	changed = 0;
	status_t status = revert_icons(log);
	if (status != B_OK)
		return status;

	std::vector<IconTarget> targets;
	std::vector<std::vector<uint8_t> > icons;

	// every type with an icon, broad ones like "image" included, as those
	// stand in for the types with none
	BMessage types;
	BMimeType::GetInstalledSupertypes(&types);
	BMimeType::GetInstalledTypes(&types);
	const char* typeNames[] = { "super_types", "types" };
	for (int32 n = 0; n < 2; n++) {
		const char* name;
		for (int32 i = 0; types.FindString(typeNames[n], i, &name) == B_OK; i++) {
			BMimeType type(name);
			// an application: what's shown is the icon in its file
			entry_ref hint;
			if (type.GetAppHint(&hint) == B_OK)
				continue;

			uint8* data = NULL;
			size_t size = 0;
			BString app;
			if (type.GetIcon(&data, &size) != B_OK || data == NULL) {
				if (!app_with_icon_for(name, app)
					|| BMimeType(app).GetIconForType(name, &data, &size) != B_OK
					|| data == NULL) {
					continue;
				}
			}
			std::vector<uint8_t> dark;
			const bool made = make_dark_icon(data, size, dark);
			delete[] data;
			if (!made)
				continue;

			IconTarget target;
			target.isType = true;
			target.target = name;
			targets.push_back(target);
			icons.push_back(dark);
		}
	}

	// the folders with icons of their own
	std::vector<IconTarget> mapped;
	status = load_icon_map(mapped, log);
	if (status != B_OK)
		return status;
	for (size_t i = 0; i < mapped.size(); i++) {
		if (mapped[i].isType)
			continue;
		BNode node(mapped[i].target);
		attr_info info;
		if (node.GetAttrInfo(kNodeIconAttribute, &info) != B_OK
			|| info.size <= 0 || info.size >= 1024 * 1024) {
			continue;
		}
		std::vector<uint8_t> data(info.size);
		std::vector<uint8_t> dark;
		if (node.ReadAttr(kNodeIconAttribute, info.type, 0, &data[0], info.size)
				!= info.size
			|| !make_dark_icon(&data[0], data.size(), dark)) {
			continue;
		}
		targets.push_back(mapped[i]);
		icons.push_back(dark);
	}

	BMessage backup;
	for (size_t i = 0; i < targets.size(); i++) {
		if (progress) {
			progress(0.5f * i / targets.size(), targets[i].target.String());
		}
		BMessage entry;
		record(targets[i], entry);
		backup.AddMessage(backup_key(targets[i]), &entry);
	}
	status = save_backup(backup);
	if (status != B_OK) {
		log << "Can't save the icons in use, so none were changed: "
			<< strerror(status) << ".\n";
		return status;
	}

	for (size_t i = 0; i < targets.size(); i++) {
		if (progress) {
			progress(0.5f + 0.5f * i / targets.size(),
				targets[i].target.String());
		}
		if (set_icon(targets[i], icons[i]) == B_OK)
			changed++;
		else
			log << "Can't set the icon of " << targets[i].target << ".\n";
	}

	set_current(NULL, kDarkHaikuIcons);
	if (progress)
		progress(1.0f, "");
	return B_OK;
}


status_t
revert_icons(BString& log)
{
	BMessage backup;
	if (load_backup(backup) != B_OK)
		return B_OK;

	char* name;
	type_code type;
	int32 count;
	for (int32 i = 0; backup.GetInfo(B_MESSAGE_TYPE, i, &name, &type, &count)
			== B_OK; i++) {
		BMessage entry;
		if (backup.FindMessage(name, &entry) == B_OK)
			restore(entry);
	}

	BPath path;
	if (themes_settings_path(path, kBackupName) == B_OK)
		BEntry(path.Path()).Remove();
	set_current(NULL, "");
	return B_OK;
}
