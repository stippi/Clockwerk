/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TABLE_CLIP_H
#define TABLE_CLIP_H

#include <GraphicsDefs.h>
#include <List.h>

#include "Clip.h"
//#include "XMLStorable.h"

class Font;
class ColorProperty;
class FontProperty;

class TableData /*: public XMLStorable*/ {
public:
			class Listener;
			class Event;
			class SelectableCell;

								TableData();
								TableData(const TableData& other);
	virtual						~TableData();

	// XMLStorable interface
//	virtual	status_t			XMLStore(XMLHelper& xmlHelper) const;
//	virtual	status_t			XMLRestore(XMLHelper& xmlHelper);

	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;
	virtual	status_t			Unarchive(const BMessage* from);

	// TableData
			status_t			InitCheck() const;

			TableData&			operator=(const TableData& other);

			bool				SetDimensions(uint32 columns, uint32 rows);

			uint32				CountColumns() const
									{ return fColumnCount; }
			uint32				CountRows() const
									{ return fRowCount; }

			BRect				Bounds() const;
			BRect				Bounds(int32 column, int32 row) const;

			Selectable*			SelectCell(uint32 column, uint32 row);

			// getters
			float				ColumnWidth(uint32 column) const;
			float				RowHeight(uint32 row) const;

			void				GetDefaultFont(Font* font) const;

			uint8				DefaultHorizontalAlignment() const;
			uint8				DefaultVerticalAlignment() const;

			// properties at a given cell
			rgb_color			CellBackgroundColor(
									uint32 column, uint32 row) const;
			rgb_color			CellContentColor(
									uint32 column, uint32 row) const;

			void				GetCellFont(uint32 column, uint32 row,
											Font* font) const;

			uint8				CellHorizontalAlignment(
									uint32 column, uint32 row) const;
			uint8				CellVerticalAlignment(
									uint32 column, uint32 row) const;

			BString				CellText(
									uint32 column, uint32 row) const;

			// setters
			void				SetDefaultColumnWidth(float width);
			void				SetDefaultRowHeight(float height);

			void				SetDefaultHorizontalAlignment(uint8 alignment);
			void				SetDefaultVerticalAlignment(uint8 alignment);
			void				SetDefaultFont(const Font& font);

			void				SetColumnWidth(uint32 column, float width);
			void				SetRowHeight(uint32 row, float height);

			// set properties at a given cell
			void				SetCellHorizontalAligment(
									uint32 column, uint32 row, uint8 alignment);
			void				SetCellVerticalAligment(
									uint32 column, uint32 row, uint8 alignment);

			void				SetCellFont(uint32 column, uint32 row,
									const Font& font);

			void				SetCellText(uint32 column, uint32 row,
									const BString& text);

			void				SetCellBackgroundColor(uint32 column,
									uint32 row, const rgb_color& color);
			void				SetCellContentColor(uint32 column, uint32 row,
									const rgb_color& color);


			bool				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

private:
			template<typename PropertyType> class CellProperty;
			class CommonProperties;
			class TableProperties;
			class RowProperties;
			class ColumnProperties;
			class CellProperties;
			struct RowWithCells;
			friend class CommonProperties;

			bool				_CreateCells(RowWithCells* row,
											 uint32 fromIndex, uint32 toIndex);

			ColumnProperties*	_ColumnAt(uint32 index) const;
			RowProperties*		_RowAt(uint32 index) const;
			CellProperties*		_CellAt(uint32 column, uint32 row) const;

			void				_Notify(const Event& event);

			TableProperties*	fTableProperties;

			BList				fColumns;
			BList				fRows;

			uint32				fColumnCount;
			uint32				fRowCount;

			BList				fListeners;
};

enum {
	TABLE_CELL_TEXT	= 0,
	TABLE_CELL_BACKGROUND_COLOR,
	TABLE_CELL_CONTENT_COLOR,
	TABLE_CELL_HORIZONTAL_ALIGNMENT,
	TABLE_CELL_VERTICAL_ALIGNMENT,
	TABLE_CELL_FONT,

	TABLE_COLUMN_WIDTH,
	TABLE_ROW_HEIGHT,

	TABLE_DIMENSIONS,
};

class TableData::Listener {
public:
								Listener();
	virtual						~Listener();

	virtual	void				TableDataChanged(const Event& event) = 0;
};

class TableData::Event {
public:
								Event(TableData* table, uint32 property,
									  int32 column = -1, int32 row = -1);
								~Event();

	TableData*					table;
	uint32						property;
	int32						column;
	int32						row;
};

class FloatProperty;
class IntProperty;
class OptionProperty;

class TableClip : public Clip {
public:
								TableClip(const char* name = NULL);
								TableClip(const TableClip& other);
	virtual						~TableClip();

	// ServerObject interface
	virtual	status_t			SetTo(const ServerObject* other);

	virtual	bool				IsMetaDataOnly() const;

	// Clip interface
	virtual	uint64				Duration();

	virtual	BRect				Bounds(BRect canvasBounds);

	virtual	bool				GetIcon(BBitmap* icon);

	virtual	status_t			InitCheck();

	// PropertyObject interface
	virtual	void				ValueChanged(Property* property);

	// TableClip
			TableData&			Table()
									{ return fTableData; }

			float				ColumnSpacing() const;
			float				RowSpacing() const;
			float				RoundCornerRadius() const;

			int32				FadeInMode() const;
			int64				FadeInFrames() const;

			int32				FadeOutMode() const;
			int64				FadeOutFrames() const;

private:
			void				_Init();

			TableData			fTableData;

			IntProperty*		fColumnCount;
			IntProperty*		fRowCount;

			FloatProperty*		fColumnWidth;
			FloatProperty*		fColumnSpacing;
			FloatProperty*		fRowHeight;
			FloatProperty*		fRowSpacing;
			FloatProperty*		fCornerRadius;

			OptionProperty*		fHorizontalAlignment;
			OptionProperty*		fVerticalAlignment;
			FontProperty*		fFont;
			FloatProperty*		fFontSize;

			OptionProperty*		fFadeInMode;
			IntProperty*		fFadeInFrames;

			OptionProperty*		fFadeOutMode;
			IntProperty*		fFadeOutFrames;
};

#endif // TABLE_CLIP_H
