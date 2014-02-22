// Copyright (c) 1999-2000, Be Incoporated. All Rights Reserved.


#include "SettingsFile.h"

#include <string.h>
#include <stdlib.h>
#include <fs_attr.h>
#include <stdio.h>
#include <errno.h>

#include <Application.h>
#include <Directory.h>
#include <Roster.h>


SettingsFile::SettingsFile(char const* lname, char const* bname,
	directory_which d)
{
	fCheck = B_OK;

	if (d != (directory_which)-1) {
		// -1 means "absolute path"
		if ((fCheck = find_directory(d, &fPath)) != B_OK)
			return;
	} else if ((fCheck = fPath.SetTo("/")) != B_OK)
		return;

	if (bname == NULL) {
		// no base name, try to figure it out from the signature
		app_info ai;
		char* sig = ai.signature;
		if ((fCheck = be_app->GetAppInfo(&ai)) != B_OK)
			return;
		int plen = strlen("application/x-vnd.");
		if (strncmp(sig, "application/x-vnd.", plen) != 0) {
			plen = strlen("application/");
			if (strncmp(sig, "application/", plen) != 0) {
				// the signature is really broken. bail out.
				fCheck = B_BAD_VALUE;
				return;
			}
		}
		sig += plen;
		bool founddot = false;
		char* sep;
		while ((sep = strchr(sig, '.')) != NULL) {
			// replace each '.' by a '/' in the signature to build a relative path
			*sep = '/';
			founddot = true;
		}
		if (!founddot && ((sep = strchr(sig, '-')) != NULL)) {
			// no '.' was found. replace the first '-' by a '/', if there's a '-'
			*sep = '/';
		}
		if ((fCheck = fPath.Append(sig)) != B_OK) {
			fPath.Unset();
			return;
		}
	} else if ((fCheck = fPath.Append(bname)) != B_OK) {
		fPath.Unset();
		return;
	}

	if (lname == NULL && (fCheck = fPath.Append("Settings")) != B_OK) {
		fPath.Unset();
		return;
	} else if ((fCheck = fPath.Append(lname)) != B_OK) {
		fPath.Unset();
		return;
	}
}


SettingsFile::~SettingsFile()
{
}


status_t
SettingsFile::InitCheck() const
{
	return fCheck;
}


status_t
SettingsFile::Load() {
	status_t result;
	BFile file(fPath.Path(), B_READ_ONLY);
	result = file.InitCheck();
	if (result != B_OK)
		return result;

	result = file.Lock();
	if (result != B_OK)
		return result;

	result = Unflatten(&file);
	if (result != B_OK) {
		file.Unlock();
		MakeEmpty();
		return result;
	}
	result = file.RewindAttrs();
	if (result != B_OK) {
		file.Unlock();
		MakeEmpty();
		return result;
	}
	char attr_name[B_ATTR_NAME_LENGTH];
	while ((result = file.GetNextAttrName(attr_name)) != B_ENTRY_NOT_FOUND) {
		// walk all the attributes of the settings file
		if (result != B_OK) {
			file.Unlock();
			return result;
		}
		// found an attribute
		attr_info ai;
		result = file.GetAttrInfo(attr_name, &ai);
		if (result != B_OK) {
			file.Unlock();
			return result;
		}
		switch (ai.type) {
			case B_CHAR_TYPE:
			case B_STRING_TYPE:
			case B_BOOL_TYPE:
			case B_INT8_TYPE:
			case B_INT16_TYPE:
			case B_INT32_TYPE:
			case B_INT64_TYPE:
			case B_UINT8_TYPE:
			case B_UINT16_TYPE:
			case B_UINT32_TYPE:
			case B_UINT64_TYPE:
			case B_FLOAT_TYPE:
			case B_DOUBLE_TYPE:
			case B_OFF_T_TYPE:
			case B_SIZE_T_TYPE:
			case B_SSIZE_T_TYPE:
			case B_POINT_TYPE:
			case B_RECT_TYPE:
			case B_RGB_COLOR_TYPE:
			case B_TIME_TYPE:
			case B_MIME_TYPE:
			{
				char* partialName = strdup(attr_name);
				if (partialName == NULL) {
					file.Unlock();
					return B_NO_MEMORY;
				}
				result = _ExtractAttribute(this, &file, attr_name,
					partialName, &ai);
				free(partialName);
				if (result != B_OK) {
					file.Unlock();
					return result;
				}
				break;
			}
		}
	}
	file.Unlock();

	return B_OK;
}


status_t
SettingsFile::_ExtractAttribute(BMessage* m, BFile* f,
	const char* fullName, char* partialName, attr_info* ai)
{
	status_t result;
	char* end = strchr(partialName, ':');

	if (end == NULL) {
		// found a leaf
		if (!m->HasData(partialName, ai->type)) {
			// the name does not exist in the message - ignore it
			return B_OK;
		}
		void* buffer = malloc(ai->size);
		if (buffer == NULL) {
			// cannot allocate space to hold the data
			return B_NO_MEMORY;
		}
		if (f->ReadAttr(fullName, ai->type, 0, buffer, ai->size) != ai->size) {
			// cannot read the data
			free(buffer);
			return B_IO_ERROR;
		}
		result = m->ReplaceData(partialName, ai->type, buffer, ai->size);
		if (result != B_OK) {
			// cannot replace the data
			free(buffer);
			return result;
		}
		free(buffer);
		return B_OK;
	}

	if (end[1] != ':') {
		// found an un-numbered sub-message
		*(end++) = '\0';
			// zero-terminate the name, point to the rest of the sub-string
		if (!m->HasMessage(partialName)) {
			// archived message does not contain that entry. go away.
			return B_OK;
		}
		BMessage subm;
		result = m->FindMessage(partialName, &subm);
			// extract the sub-message
		if (result != B_OK)
			return result;

		result = _ExtractAttribute(&subm, f, fullName, end, ai);
			// keep processing
		if (result != B_OK)
			return result;

		result = m->ReplaceMessage(partialName, &subm);
			// replace the sub-message
		if (result != B_OK)
			return result;

		return B_OK;
	} else {
		// found a numbered entry
		char* endptr;
		errno = 0;
		*end = '\0';
			// zero-terminate the name
		int32 r = strtol(end + 2, &endptr, 10);
			// get the entry number
		if (errno != 0)
			return B_OK;

		if (r >= 1000000000) {
			// sanity-check
			return B_OK;
		}

		if (* endptr == ':') {
			// this is a numbered message
			if (!m->HasMessage(partialName, r)) {
				// archived message does not contain that entry, go away
				return B_OK;
			}
			BMessage subm;
			result = m->FindMessage(partialName, r, &subm);
				// extract the sub-message
			if (result != B_OK)
				return result;

			result = _ExtractAttribute(&subm, f, fullName, endptr + 1, ai);
				// recurse
			if (result != B_OK)
				return result;

			result = m->ReplaceMessage(partialName, r, &subm);
				// replace the sub-message
			if (result != B_OK)
				return result;

			return B_OK;
		} else if (*endptr == '\0') {
			// this is a numbered leaf
			if (!m->HasData(partialName, ai->type, r)) {
				// archived message does not contain this leaf
				return B_OK;
			}
			void* buffer = malloc(ai->size);
			if (buffer == NULL)
				return B_NO_MEMORY;

			if (f->ReadAttr(fullName, ai->type, 0, buffer, ai->size)
					!= ai->size) {
				// extract the attribute data
				free(buffer);
				return B_IO_ERROR;
			}
			result = m->ReplaceData(partialName, ai->type, r, buffer, ai->size); // and replace it in the message
			if (result != B_OK) {
				free(buffer);
				return result;
			}
			free(buffer);

			return B_OK;
		}
	}

	return B_OK;
}


status_t
SettingsFile::Save() const
{
	status_t result;
	BFile file(fPath.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	result = file.InitCheck();

	if (result == B_BAD_VALUE) {
		// try to create the parent directory if creating the file fails
		// the first time
		BPath parent;
		result = fPath.GetParent(&parent);
		if (result != B_OK)
			return result;

		result = create_directory(parent.Path(), 0777);
		if (result != B_OK)
			return result;

		result = file.SetTo(fPath.Path(), B_READ_WRITE | B_CREATE_FILE
			| B_ERASE_FILE);
	}
	if (result != B_OK)
		return result;

	result = file.Lock();
		// lock the file to do atomic attribute transactions on it.
	if (result != B_OK)
		return result;

	result = Flatten(&file);
	if (result != B_OK) {
		file.Unlock();
		return result;
	}
	result = _StoreAttributes(this, &file);
	if (result != B_OK) {
		file.Unlock();
		return result;
	}
	file.Unlock();

	return B_OK;
}


status_t
SettingsFile::_StoreAttributes(BMessage const* m, BFile* f,
	const char* basename)
{
	char* namefound;
	type_code typefound;
	int32 countfound;
	status_t result;
	for (int32 i = 0; i < m->CountNames(B_ANY_TYPE); i++) {
		// walk the entries in the message
		result = m->GetInfo(B_ANY_TYPE, i, &namefound, &typefound, &countfound);
		if (result != B_OK)
			return result;

		if (strchr(namefound, ':') != NULL) {
			// do not process anything that contains a colon
			// (considered a magic char)
			break;
		}

		switch (typefound) {
			case B_MESSAGE_TYPE:
			{
				// found a sub-message
				if (countfound == 1) {
					// single sub-message
					char* lname = (char*)malloc(strlen(basename)
						+ strlen(namefound) + 2);
							// allocate space for the base name
					if (lname == NULL)
						return B_NO_MEMORY;

					sprintf(lname, "%s%s:", basename, namefound);
						// create the base name for the sub-message
					BMessage subm;
					result = m->FindMessage(namefound, &subm);
					if (result != B_OK) {
						free(lname);
						return result;
					}
					result = _StoreAttributes(&subm, f, lname);
						// and process the sub-message with the base name
					if (result != B_OK) {
						free(lname);
						return result;
					}
					free(lname);
				} else if (countfound < 1000000000) {
						// (useless in 32-bit) sanity check
					char* lname = (char*)malloc(strlen(basename)
						+ strlen(namefound) + 11);
							// allocate space for the base name
					if (lname == NULL)
						return B_NO_MEMORY;

					sprintf(lname, "%ld", countfound - 1);
						// find the length of the biggest number for that field
					char format[12];
					sprintf(format, "%%s%%s::%%0%ldld:", strlen(lname));
						// create the sprintf format
					for (int32 j = 0;j<countfound;j++) {
						sprintf(lname, format, basename, namefound, j);
							// create the base name for the sub-message
						BMessage subm;
						result = m->FindMessage(namefound, j, &subm);
						if (result != B_OK) {
							free(lname);
							return result;
						}
						result = _StoreAttributes(&subm, f, lname);
							// process the sub-message with the base name
						if (result != B_OK) {
							free(lname);
							return result;
						}
					}
					free(lname);
				}

				break;
			}
			case B_CHAR_TYPE:
			case B_STRING_TYPE:
			case B_BOOL_TYPE:
			case B_INT8_TYPE:
			case B_INT16_TYPE:
			case B_INT32_TYPE:
			case B_INT64_TYPE:
			case B_UINT8_TYPE:
			case B_UINT16_TYPE:
			case B_UINT32_TYPE:
			case B_UINT64_TYPE:
			case B_FLOAT_TYPE:
			case B_DOUBLE_TYPE:
			case B_OFF_T_TYPE:
			case B_SIZE_T_TYPE:
			case B_SSIZE_T_TYPE:
			case B_POINT_TYPE:
			case B_RECT_TYPE:
			case B_RGB_COLOR_TYPE:
			case B_TIME_TYPE:
			case B_MIME_TYPE:
			{
				// found a supported type. the code is basically the same
				if (countfound == 1) {
					char* lname = (char* )malloc(strlen(basename)
						+ strlen(namefound) + 1);
					if (lname == NULL)
						return B_NO_MEMORY;

					sprintf(lname, "%s%s", basename, namefound);
					const void* datafound;
					ssize_t sizefound;
					result = m->FindData(namefound, typefound, &datafound,
						&sizefound);
					if (result != B_OK) {
						free(lname);
						return result;
					}
					sizefound = f->WriteAttr(lname, typefound, 0, datafound,
						sizefound);
					if (sizefound<0) {
						free(lname);
						return sizefound;
					}
					free(lname);
				} else if (countfound < 1000000000) {
					char* lname = (char* )malloc(strlen(basename)
						+ strlen(namefound) + 10);
					if (lname == NULL)
						return B_NO_MEMORY;

					sprintf(lname, "%ld", countfound - 1);
					char format[12];
					sprintf(format, "%%s%%s::%%0%ldld", strlen(lname));
					for (int32 j = 0; j < countfound; j++) {
						sprintf(lname, format, basename, namefound, j);
						const void* datafound;
						ssize_t sizefound;
						result = m->FindData(namefound, typefound, j,
							&datafound, &sizefound);
						if (result != B_OK) {
							free(lname);
							return result;
						}
						sizefound = f->WriteAttr(lname, typefound, 0,
							datafound, sizefound);
						if (sizefound < 0) {
							free(lname);
							return sizefound;
						}
					}
					free(lname);
				}
				break;
			}
		}
	}

	return B_OK;
}
