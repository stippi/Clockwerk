/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ClipObjectFactory.h"

#include <new>
#include <stdio.h>

#include <fs_attr.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <NodeInfo.h>
#include <Path.h>

#include "common.h"

#include "ClockClip.h"
#include "CollectablePlaylist.h"
#include "CollectingPlaylist.h"
#include "ColorClip.h"
#include "CommonPropertyIDs.h"
#include "FileBasedClip.h"
#include "Playlist.h"
#include "ScrollingTextClip.h"
#include "SequenceContainerPlaylist.h"
#include "ServerObjectManager.h"
#include "SlideShowPlaylist.h"
#include "StretchingPlaylist.h"
#include "support.h"
#include "TableClip.h"
#include "TextClip.h"
#include "TimerClip.h"
#include "WeatherClip.h"
//#include "XMLExporter.h"
//#include "XMLImporter.h"

using std::nothrow;

// constructor
ClipObjectFactory::ClipObjectFactory(bool allowLazyLoading)
	: fAllowLazyLoading(allowLazyLoading)
{
}

// destructor
ClipObjectFactory::~ClipObjectFactory()
{
}

// Instantiate
ServerObject*
ClipObjectFactory::Instantiate(BString& type, const BString& id,
	ServerObjectManager* library)
{
	ServerObject* object = NULL;

	if (type == "FileBasedClip") {
		// find file by using server id as name
		entry_ref ref;
		status_t error = library->GetRef(id, ref);
		if (error == B_OK) {
			object = FileBasedClip::CreateClip(library, &ref, error,
				false, fAllowLazyLoading);
		} else {
			printf("ClipObjectFactory::Instantiate() - "
				   "GetRef() failed: '%s'\n", strerror(error));
		}
	} else if (type == "ClockClip") {
		object = new (nothrow) ClockClip("<new clock>");
	} else if (type == "ColorClip") {
		object = new (nothrow) ColorClip("<new color>");
	} else if (type == "ScrollingTextClip") {
		object = new (nothrow) ScrollingTextClip("<new ticker>");
	} else if (type == "TextClip") {
		object = new (nothrow) TextClip("<new text>");
	} else if (type == "TimerClip") {
		object = new (nothrow) TimerClip("<new timer>");
	} else if (type == "WeatherClip") {
		object = new (nothrow) WeatherClip("<new weather>");
	} else if (type == "TableClip") {
		TableClip* table = new (nothrow) TableClip("<new table>");

		entry_ref ref;
		status_t error = library->GetRef(id, ref);
		if (error == B_OK) {
			// an existing table object
			BFile file(&ref, B_READ_ONLY);
#if 0
			XMLImporter importer;
			try {
				importer.Restore(&table->Table(), &file);
				object = table;
			} catch (...) {
				delete table;
				printf("ClipObjectFactory::Instantiate() - "
					   "exception while loading table\n");
			}
#else
			try {
				BMessage tableArchive;
				status_t error = tableArchive.Unflatten(&file);
				if (error == B_OK)
					error = table->Table().Unarchive(&tableArchive);
				if (error == B_OK) {
					object = table;
					table->SetDataSaved(true);
				} else
					delete table;
			} catch (...) {
				delete table;
				printf("ClipObjectFactory::Instantiate() - "
					   "exception while loading table\n");
			}
#endif
		} else {
			// a new table object
			object = table;
		}
	} else if (string_ends_with(type, "Playlist")) {
		// load playlist, it is also a clip
		entry_ref ref;
		BFile file;
		status_t error = B_OK;
		if (id.Length() > 0) {
			error = library->GetRef(id, ref);
			if (error == B_OK)
				error = file.SetTo(&ref, B_READ_ONLY);
			if (error < B_OK) {
				printf("ClipObjectFactory::Instantiate() - failed "
					"to init file for existing id: %s\n", strerror(error));
			}
		}
		if (error == B_OK) {
			Playlist* playlist;
			const char* defaultName = NULL;
			if (type == "SlideShowPlaylist") {
				playlist = new (nothrow) SlideShowPlaylist();
				defaultName = "<new Sildeshow Playlist>";
			} else if (type == "StretchingPlaylist") {
				playlist = new (nothrow) StretchingPlaylist();
				defaultName = "<new Stretching Playlist>";
			} else if (type == "CollectablePlaylist") {
				playlist = new (nothrow) CollectablePlaylist();
				defaultName = "<new Template Playlist>";
			} else if (type == "CollectingPlaylist") {
				playlist = new (nothrow) CollectingPlaylist();
				defaultName = "<new Template Collector>";
			} else if (type == "SequenceContainerPlaylist") {
				playlist = new (nothrow) SequenceContainerPlaylist();
				defaultName = "<new Sequence Container Playlist>";
			} else {
				playlist = new (nothrow) Playlist();
				defaultName = "<new Playlist>";
			}

			if (file.InitCheck() == B_OK) {
#if 0
				XMLImporter importer;
				try {
					// NOTE: the playlist will only reference
					// clips by their id, therefor one needs
					// to call library->ResolveDependencies()
					// after all objects are loaded
					importer.Import(playlist, &file, &ref);
					object = playlist;
					playlist->SetDataSaved(true);
				} catch (...) {
					delete playlist;
					printf("ClipObjectFactory::Instantiate() - "
						   "exception while loading playlist\n");
				}
#else
				try {
					BMessage playlistArchive;
					status_t error = playlistArchive.Unflatten(&file);
					if (error == B_OK)
						error = playlist->Unarchive(&playlistArchive);
					if (error == B_OK) {
						object = playlist;
						playlist->SetDataSaved(true);
					} else
						delete playlist;
				} catch (...) {
					delete playlist;
					printf("ClipObjectFactory::Instantiate() - "
						   "exception while loading playlist\n");
				}
#endif
			} else {
				// this is a new object
				playlist->SetName(defaultName);
				object = playlist;
			}
		}
	}

	if (!object) {
		if (type != "ClockwerkComponent"
			&& type != "ClockwerkRevision"
			&& type != "ClientSettings") {

			print_info("ClipObjectFactory::Instantiate() - "
				"using place holder object for type %s and id %s\n",
				type.String(), id.String());
		}
		// instantiate placeholder
		object = new (nothrow) ServerObject(type.String());
	}

	// we already know the id
	if (object && id.Length() > 0)
		object->SetID(id);

	if (object && (object->IsMetaDataOnly() || object->IsExternalData())) {
		// we know that this object's data is saved
		object->SetDataSaved(true);
	}

	return object;
}

// InstantiateClone
ServerObject*
ClipObjectFactory::InstantiateClone(const ServerObject* other,
	ServerObjectManager* library)
{
	ServerObject* object = NULL;

	BString id = ServerObjectManager::NextID();
	BString sourceID = other->ID();

	const FileBasedClip* fileClip
					= dynamic_cast<const FileBasedClip*>(other);
	const Playlist* playlist
					= dynamic_cast<const Playlist*>(other);
	const SequenceContainerPlaylist* sequencePL
					= dynamic_cast<const SequenceContainerPlaylist*>(other);
	const SlideShowPlaylist* slideShowPL
					= dynamic_cast<const SlideShowPlaylist*>(other);
	const StretchingPlaylist* stretchingPL
					= dynamic_cast<const StretchingPlaylist*>(other);
	const ClockClip* clock
					= dynamic_cast<const ClockClip*>(other);
	const CollectablePlaylist* collectablePL
					= dynamic_cast<const CollectablePlaylist*>(other);
	const CollectingPlaylist* collectingPL
					= dynamic_cast<const CollectingPlaylist*>(other);
	const ColorClip* color
					= dynamic_cast<const ColorClip*>(other);
	const ScrollingTextClip* ticker
					= dynamic_cast<const ScrollingTextClip*>(other);
	const TextClip* text
					= dynamic_cast<const TextClip*>(other);
	const TableClip* table
					= dynamic_cast<const TableClip*>(other);
	const TimerClip* timer
					= dynamic_cast<const TimerClip*>(other);
	const WeatherClip* weather
					= dynamic_cast<const WeatherClip*>(other);

	if (fileClip || playlist || table) {
		// find file by using server id as name
		// copy the file
		entry_ref srcRef;
		status_t error = library->GetRef(sourceID, srcRef);
		if (error < B_OK)
			return NULL;
		entry_ref dstRef;
		error = library->GetRef(id, dstRef);
		if (error < B_OK)
			return NULL;

		BFile source(&srcRef, B_READ_ONLY);
		error = source.InitCheck();
		if (error >= B_OK) {
			BFile dest(&dstRef, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
			error = dest.InitCheck();
			if (error < B_OK)
				return NULL;
			copy_data(source, dest);
		} // else the file does not yet exist
		  // (the original has never been saved yet)

		if (slideShowPL)
			object = new SlideShowPlaylist(*slideShowPL);
		else if (collectablePL)
			object = new CollectablePlaylist(*collectablePL);
				// CollectablePlaylist is also a StretchingPlaylist,
				// so process this first!
		else if (collectingPL)
			object = new CollectingPlaylist(*collectingPL);
		else if (sequencePL)
			object = new SequenceContainerPlaylist(*sequencePL);
		else if (stretchingPL)
			object = new StretchingPlaylist(*stretchingPL);
		else if (playlist)
			object = new Playlist(*playlist, true);
		else if (table)
			object = new TableClip(*table);
		else
			object = FileBasedClip::CreateClip(library, &dstRef, error);

	} else if (clock) {
		object = new (nothrow) ClockClip(*clock);
	} else if (color) {
		object = new (nothrow) ColorClip(*color);
	} else if (ticker) {
		object = new (nothrow) ScrollingTextClip(*ticker);
	} else if (text) {
		object = new (nothrow) TextClip(*text);
	} else if (timer) {
		object = new (nothrow) TimerClip(*timer);
	} else if (weather) {
		object = new (nothrow) WeatherClip(*weather);
	} else {
//		printf("ClipObjectFactory::Instantiate() - "
//			   "unkown object type (%s)\n", type.String());
		// just use a "plain" ServerObject
		object = new (nothrow) ServerObject(*other, true);
	}

	// reset the property values that have been copied
	if (object) {
		object->SetID(id);
		BString name(other->Name());
		name << " copy";
		object->SetName(name);
		object->SetStatus(SYNC_STATUS_LOCAL);
		object->SetVersion(0);
		if (object->IsMetaDataOnly() || object->IsExternalData()) {
			// we know that this object's data is saved
			object->SetDataSaved(true);
		}
	}

	return object;
}

// pragma mark -

// StoreObject
status_t
ClipObjectFactory::StoreObject(ServerObject* object,
	ServerObjectManager* library)
{
	Playlist* playlist = dynamic_cast<Playlist*>(object);
	TableClip* table = dynamic_cast<TableClip*>(object);
#if 0
	Schedule* schedule = dynamic_cast<Schedule*>(object);

	XMLStorable* storable = NULL;
	if (table)
		storable = &table->Table();
	else if (schedule)
		storable = schedule;

	if (!playlist && !storable)
		return B_OK;
#else
	if (!playlist && !table)
		return B_OK;
#endif

	entry_ref _docRef;
	entry_ref* docRef = &_docRef;
	status_t ret = library->GetRef(object->ID(), *docRef);
	if (ret < B_OK)
		return ret;

	const entry_ref* ref = docRef;
	entry_ref tempRef;
	BEntry entry(docRef, true);

	if (entry.Exists()) {
		if (entry.IsDirectory())
			return B_ERROR;

		// if the file exists create a temporary file in
		// the same folder and hope that it doesn't already exist...
		BPath tempPath(docRef);
		if (tempPath.GetParent(&tempPath) >= B_OK) {
			BString helper(docRef->name);
			helper << system_time();
			if (tempPath.Append(helper.String()) >= B_OK
				&& entry.SetTo(tempPath.Path()) >= B_OK
				&& entry.GetRef(&tempRef) >= B_OK) {
				// have the output ref point to the temporary
				// file instead
				ref = &tempRef;
			}
		}
	}

	const char* fileMIME = NULL;
	// do the actual save operation into a file
	BFile outFile(ref, B_CREATE_FILE | B_READ_WRITE | B_ERASE_FILE);
	ret = outFile.InitCheck();
	if (ret < B_OK) {
		printf("ClipObjectFactory::StoreObject() - "
			   "failed to create output file: %s\n", strerror(ret));
		return ret;
	}

#if 0
	XMLExporter exporter;
	// Xerces may throw an exception...
	try {
		if (playlist)
			ret = exporter.Export(playlist, &outFile, docRef);
		else {
			ret = exporter.Store(storable, &outFile);
		}

		if (ret == B_OK) {
			// success, update export entry_ref,
			// set name if not yet set and get MIME type
			fileMIME = exporter.MIMEType();
		} else {
			printf("ClipObjectFactory::StoreObject() - "
				   "failed to store: %s\n", strerror(ret));
		}
	} catch (const XMLException& e) {
		printf("ClipObjectFactory::StoreObject() - "
			   "XML exception occured: file %s, line: %d\n"
			   "message: %s\n", e.getSrcFile(), e.getSrcLine(),
			   (const char*)e.getMessage());
		ret = B_ERROR;
#else
	try {
		BMessage archive;
		if (playlist != NULL) {
			ret = playlist->Archive(&archive);
			fileMIME = "application/x-clockwerk-playlist";
		} else {
			ret = table->Table().Archive(&archive);
			fileMIME = "application/x-clockwerk-table";
		}
		if (ret == B_OK)
			ret = archive.Flatten(&outFile);
		if (ret != B_OK) {
			fprintf(stderr, "ClipObjectFactory::StoreObject() - "
				"failed to store: %s\n", strerror(ret));
		}
#endif
	} catch (...) {
		fprintf(stderr, "ClipObjectFactory::StoreObject() - "
			"unkown exception occured!\n");
		ret = B_ERROR;
	}
	outFile.Unset();

	if (ret < B_OK && ref != docRef) {
		// in case of failure, remove temporary file
		entry.Remove();
	}

	if (ret >= B_OK && ref != docRef) {
		// move temp file overwriting actual document file
		BEntry docEntry(docRef, true);
		BDirectory dir;
		if ((ret = docEntry.GetParent(&dir)) >= B_OK) {
			// copy attributes of previous document file
			BNode sourceNode(&docEntry);
			BNode destNode(&entry);
			if (sourceNode.InitCheck() >= B_OK && destNode.InitCheck() >= B_OK) {
				// lock the nodes
				if (sourceNode.Lock() >= B_OK) {
					if (destNode.Lock() >= B_OK) {
						// iterate over the attributes
						char attrName[B_ATTR_NAME_LENGTH];
						while (sourceNode.GetNextAttrName(attrName) >= B_OK) {
							attr_info info;
							if (sourceNode.GetAttrInfo(attrName, &info) >= B_OK) {
								char *buffer = new (nothrow) char[info.size];
								if (buffer && (size_t)sourceNode.ReadAttr(
										attrName, info.type, 0, buffer,
										info.size) == info.size) {
									destNode.WriteAttr(attrName, info.type, 0,
													   buffer, info.size);
								}
								delete[] buffer;
							}
						}
						destNode.Unlock();
					}
					sourceNode.Unlock();
				}
			}
			// clobber the orginal file with the new temporary one
			ret = entry.MoveTo(&dir, docRef->name, true);
		}
	}

	if (ret >= B_OK && fileMIME) {
		// set file type
		BNode node(docRef);
		BNodeInfo nodeInfo(&node);
		if (nodeInfo.InitCheck() == B_OK)
			nodeInfo.SetType(fileMIME);
	}

	object->SetDataSaved(true);

	return ret;
}


