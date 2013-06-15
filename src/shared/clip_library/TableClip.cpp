/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "TableClip.h"

#include <new>

#include <stdio.h>
#include <stdlib.h>

#include <Bitmap.h>
#include <GraphicsDefs.h>

#include "common_constants.h"
#include "ui_defines.h"
#include "support_ui.h"

#include "AttributeMessage.h"
#include "ColorProperty.h"
#include "CommonPropertyIDs.h"
#include "Font.h"
#include "FontProperty.h"
#include "Icons.h"
#include "OptionProperty.h"
#include "Painter.h"
#include "Property.h"
#include "PropertyObject.h"
#include "Selectable.h"
#include "Selection.h"
//#include "XMLHelper.h"

using std::nothrow;

// CellProperty
template<typename PropertyType>
class TableData::CellProperty {
public:
	CellProperty(const PropertyType& value)
		: value(value),
		  lastChangedTime(real_time_clock_usecs())
	{
	}
	CellProperty()
		: value(),
		  lastChangedTime(-1)
	{
	}

	void Unset()
	{
		lastChangedTime = -1;
	}

	bool IsSet() const
	{
		return lastChangedTime >= 0;
	}

	bool Set(const PropertyType& value,
			 bigtime_t time = real_time_clock_usecs())
	{
		if (!IsSet() || this->value != value) {
			this->value = value;
			lastChangedTime = time;
			return true;
		}
		return false;
	}

	CellProperty& operator=(const CellProperty& other)
	{
		Set(other.value, other.lastChangedTime);
		return *this;
	}

	PropertyType	value;
	bigtime_t		lastChangedTime;
};

// CommonProperties
class TableData::CommonProperties /*: public XMLStorable*/ {
 public:
								CommonProperties(CommonProperties* parent1,
												 CommonProperties* parent2);
	virtual						~CommonProperties();

	// XMLStorable interface
//	virtual	status_t			XMLStore(XMLHelper& xmlHelper) const;
//	virtual	status_t			XMLRestore(XMLHelper& xmlHelper);
	virtual	status_t			Archive(BMessage& archive) const;
	virtual	status_t			Unarchive(BMessage& archive);

	// CommonProperties
	virtual	bigtime_t			GetBackgroundColor(rgb_color* color);
	virtual	bigtime_t			GetContentColor(rgb_color* color);

	virtual	bigtime_t			GetFont(Font* font);

	virtual	bigtime_t			GetHorizontalAlignment(uint8* alignment);
	virtual	bigtime_t			GetVerticalAlignment(uint8* alignment);

			CommonProperties&	operator=(const CommonProperties& other);

			bool				SetBackgroundColor(const rgb_color& color);
			bool				SetContentColor(const rgb_color& color);

			bool				SetFont(const Font& font);

			bool				SetHorizontalAlignment(uint8 alignment);
			bool				SetVerticalAlignment(uint8 alignment);

 protected:
			typedef TableData::CellProperty<rgb_color>	Color;
			typedef TableData::CellProperty<Font>		CellFont;
			typedef TableData::CellProperty<uint8>		Alignment;


//			template<typename PropertyType>
//			status_t			_StoreAttribute(
//									const TableData::CellProperty<PropertyType>& property,
//									XMLHelper& xml, const char* tagName) const;
//
//			template<typename PropertyType>
//			status_t			_RestoreAttribute(
//									TableData::CellProperty<PropertyType>& property,
//									XMLHelper& xml, const char* tagName);

			template<typename PropertyType>
			status_t			_StoreAttribute(
									const TableData::CellProperty<PropertyType>& property,
									BMessage& archive,
									const char* tagName) const;

			template<typename PropertyType>
			status_t			_RestoreAttribute(
									TableData::CellProperty<PropertyType>& property,
									BMessage& archive, const char* tagName);

			template<typename PropertyType, typename Function>
			bigtime_t			_GetValue(TableData::CellProperty<PropertyType>& property,
										  Function function,
										  PropertyType* value);

			CommonProperties*	fParent1;
			CommonProperties*	fParent2;

			Color				fBackgroundColor;
			Color				fContentColor;
			CellFont			fFont;
			Alignment			fHorizontalAlignment;
			Alignment			fVerticalAlignment;
};

// TableProperties
class TableData::TableProperties : public CommonProperties {
 public:
								TableProperties();
	virtual						~TableProperties();

	// XMLStorable interface
//	virtual	status_t			XMLStore(XMLHelper& xmlHelper) const;
//	virtual	status_t			XMLRestore(XMLHelper& xmlHelper);
	virtual	status_t			Archive(BMessage& archive) const;
	virtual	status_t			Unarchive(BMessage& archive);

	// TableProperties
			bool				SetColumnWidth(float width);
			bigtime_t			GetColumnWidth(float* width);

			bool				SetRowHeight(float height);
			bigtime_t			GetRowHeight(float* height);

			TableProperties&	operator=(const TableProperties& other);

 private:
			typedef TableData::CellProperty<float>	Size;

			Size				fColumnWidth;
			Size				fRowHeight;
};

// RowProperties
class TableData::RowProperties : public CommonProperties {
 public:
								RowProperties(TableProperties* parent);
	virtual						~RowProperties();

	// XMLStorable interface
//	virtual	status_t			XMLStore(XMLHelper& xmlHelper) const;
//	virtual	status_t			XMLRestore(XMLHelper& xmlHelper);
	virtual	status_t			Archive(BMessage& archive) const;
	virtual	status_t			Unarchive(BMessage& archive);

	// RowProperties
			bool				SetHeight(float height);
			bigtime_t			GetHeight(float* height);

			RowProperties&		operator=(const RowProperties& other);

 private:
			typedef TableData::CellProperty<float>	Size;

			TableProperties*	fParent;

			Size				fHeight;
};

// ColumnProperties
class TableData::ColumnProperties : public CommonProperties {
 public:
								ColumnProperties(TableProperties* parent);
	virtual						~ColumnProperties();

	// XMLStorable interface
//	virtual	status_t			XMLStore(XMLHelper& xmlHelper) const;
//	virtual	status_t			XMLRestore(XMLHelper& xmlHelper);
	virtual	status_t			Archive(BMessage& archive) const;
	virtual	status_t			Unarchive(BMessage& archive);

	// ColumnProperties
			bool				SetWidth(float width);
			bigtime_t			GetWidth(float* width);

			ColumnProperties&	operator=(const ColumnProperties& other);

 private:
			typedef TableData::CellProperty<float>	Size;

			TableProperties*	fParent;

			Size				fWidth;
};

// CellProperties
class TableData::CellProperties : public CommonProperties {
 public:
								CellProperties(CommonProperties* column,
											   CommonProperties* row);
	virtual						~CellProperties();

	// XMLStorable interface
//	virtual	status_t			XMLStore(XMLHelper& xmlHelper) const;
//	virtual	status_t			XMLRestore(XMLHelper& xmlHelper);
	virtual	status_t			Archive(BMessage& archive) const;
	virtual	status_t			Unarchive(BMessage& archive);

	// CellProperties
			bigtime_t			GetText(BString* text);
			bool				SetText(const BString& text);

			CellProperties&		operator=(const CellProperties& other);

			SelectableCell*		selectable;

 private:
			typedef TableData::CellProperty<BString>	Text;

			Text				fText;

};

// SelectableCell
class TableData::SelectableCell : public Selectable,
								  public PropertyObject,
								  public TableData::Listener {
 public:
								SelectableCell();
	virtual						~SelectableCell();

	// Selectable interface
	virtual	void				SelectedChanged();

	// PropertyObject interface
	virtual	void				ValueChanged(Property* property);

	// TableData::Listener interface
	virtual void				TableDataChanged(
									const TableData::Event& event);

	// SelectableCell
			void				SetTo(TableData* table,
									  uint32 column, uint32 row);

 private:
			TableData*			fTable;

			IntProperty*		fColumn;
			IntProperty*		fRow;

			StringProperty*		fText;

			OptionProperty*		fHorizontalAlignment;
			OptionProperty*		fVerticalAlignment;
			FontProperty*		fFont;
			FloatProperty*		fFontSize;

			ColorProperty*		fBackgroundColor;
			ColorProperty*		fContentColor;
};

// RowWithCells
struct TableData::RowWithCells {
	RowWithCells(TableProperties* tableProperties)
		: properties(tableProperties),
		  cells(20)
	{
	}

	RowProperties	properties;
	BList			cells;
};

// #pragma mark -

// constructor
TableData::CommonProperties::CommonProperties(CommonProperties* parent1,
											  CommonProperties* parent2)
	: fParent1(parent1),
	  fParent2(parent2)
{
}

// destructor
TableData::CommonProperties::~CommonProperties()
{
}

// _StoreAttibute
//template<typename PropertyType>
//status_t
//TableData::CommonProperties::_StoreAttribute(
//	const TableData::CellProperty<PropertyType>& property,
//	XMLHelper& xml, const char* tagName) const
//{
//	if (!property.IsSet())
//		return B_OK;
//
//	status_t ret = xml.CreateTag(tagName);
//
//	if (ret == B_OK)
//		ret = xml.SetAttribute("value", property.value);
//	if (ret == B_OK)
//		ret = xml.SetAttribute("changed_time", property.lastChangedTime);
//
//	if (ret == B_OK)
//		ret = xml.CloseTag();
//
//	return ret;
//}
//
//// _RestoreAttibute
//template<typename PropertyType>
//status_t
//TableData::CommonProperties::_RestoreAttribute(
//	TableData::CellProperty<PropertyType>& property,
//	XMLHelper& xml, const char* tagName)
//{
//	status_t ret = xml.OpenTag(tagName);
//
//	if (ret == B_OK) {
//		property.value = xml.GetAttribute("value", property.value);
//		property.lastChangedTime = xml.GetAttribute("changed_time",
//													property.lastChangedTime);
//		return xml.CloseTag();
//			// if the tag could be opened, it needs to be closed again
//	}
//
//	return B_OK;
//		// no problem, simply use default values
//}
//
//// XMLStore
//status_t
//TableData::CommonProperties::XMLStore(XMLHelper& xml) const
//{
//	status_t ret = _StoreAttribute(fBackgroundColor, xml, "BG_COLOR");
//
//	if (ret == B_OK)
//		ret = _StoreAttribute(fContentColor, xml, "CONTENT_COLOR");
//
//	if (ret == B_OK)
//		ret = _StoreAttribute(fFont, xml, "FONT");
//
//	if (ret == B_OK)
//		ret = _StoreAttribute(fHorizontalAlignment, xml, "H_ALIGNMENT");
//
//	if (ret == B_OK)
//		ret = _StoreAttribute(fVerticalAlignment, xml, "V_ALIGNMENT");
//
//	return ret;
//}
//
//// XMLRestore
//status_t
//TableData::CommonProperties::XMLRestore(XMLHelper& xml)
//{
//	status_t ret = _RestoreAttribute(fBackgroundColor, xml, "BG_COLOR");
//
//	if (ret == B_OK)
//		ret = _RestoreAttribute(fContentColor, xml, "CONTENT_COLOR");
//
//	if (ret == B_OK)
//		ret = _RestoreAttribute(fFont, xml, "FONT");
//
//	if (ret == B_OK)
//		ret = _RestoreAttribute(fHorizontalAlignment, xml, "H_ALIGNMENT");
//
//	if (ret == B_OK)
//		ret = _RestoreAttribute(fVerticalAlignment, xml, "V_ALIGNMENT");
//
//	return ret;
//}

// _StoreAttibute
template<typename PropertyType>
status_t
TableData::CommonProperties::_StoreAttribute(
	const TableData::CellProperty<PropertyType>& property,
	BMessage& archive, const char* tagName) const
{
	if (!property.IsSet())
		return B_OK;

	AttributeMessage archivedProperty;
	status_t ret = archivedProperty.SetAttribute("value", property.value);
	if (ret == B_OK) {
		ret = archivedProperty.SetAttribute("changed_time",
			property.lastChangedTime);
	}

	if (ret == B_OK)
		ret = archive.AddMessage(tagName, &archivedProperty);

	return ret;
}

// _RestoreAttibute
template<typename PropertyType>
status_t
TableData::CommonProperties::_RestoreAttribute(
	TableData::CellProperty<PropertyType>& property,
	BMessage& archive, const char* tagName)
{
	AttributeMessage archivedProperty;
	status_t ret = archive.FindMessage(tagName, &archivedProperty);

	if (ret == B_OK) {
		property.value = archivedProperty.GetAttribute("value", property.value);
		property.lastChangedTime = archivedProperty.GetAttribute("changed_time",
			property.lastChangedTime);
	}

	return B_OK;
		// no problem, simply use/keep default values
}

// Archive
status_t
TableData::CommonProperties::Archive(BMessage& archive) const
{
	status_t ret = _StoreAttribute(fBackgroundColor, archive, "BG_COLOR");

	if (ret == B_OK)
		ret = _StoreAttribute(fContentColor, archive, "CONTENT_COLOR");

	if (ret == B_OK)
		ret = _StoreAttribute(fFont, archive, "FONT");

	if (ret == B_OK)
		ret = _StoreAttribute(fHorizontalAlignment, archive, "H_ALIGNMENT");

	if (ret == B_OK)
		ret = _StoreAttribute(fVerticalAlignment, archive, "V_ALIGNMENT");

	return ret;
}

// Unarchive
status_t
TableData::CommonProperties::Unarchive(BMessage& archive)
{
	status_t ret = _RestoreAttribute(fBackgroundColor, archive, "BG_COLOR");

	if (ret == B_OK)
		ret = _RestoreAttribute(fContentColor, archive, "CONTENT_COLOR");

	if (ret == B_OK)
		ret = _RestoreAttribute(fFont, archive, "FONT");

	if (ret == B_OK)
		ret = _RestoreAttribute(fHorizontalAlignment, archive, "H_ALIGNMENT");

	if (ret == B_OK)
		ret = _RestoreAttribute(fVerticalAlignment, archive, "V_ALIGNMENT");

	return ret;
}


// _GetValue
template<typename PropertyType, typename Function>
bigtime_t
TableData::CommonProperties::_GetValue(
	TableData::CellProperty<PropertyType>& property,
	Function function, PropertyType* value)
{
	if ((!fParent1 && !fParent2) || property.IsSet()) {
		*value = property.value;
		return property.lastChangedTime;
	}
	if (fParent1 && fParent2) {
		// pick the one with the newer value
		PropertyType value1;
		PropertyType value2;
		bigtime_t parent1Time = (fParent1->*function)(&value1);
		bigtime_t parent2Time = (fParent2->*function)(&value2);
		if (parent1Time > parent2Time) {
			*value = value1;
			return parent1Time;
		} else {
			*value = value2;
			return parent2Time;
		}
	} else if (fParent1) {
		return (fParent1->*function)(value);
	} else {
		return (fParent2->*function)(value);
	}
}

// GetBackgroundColor
bigtime_t
TableData::CommonProperties::GetBackgroundColor(rgb_color* color)
{
	return _GetValue(fBackgroundColor,
					 &CommonProperties::GetBackgroundColor,
					 color);
}

// GetContentColor
bigtime_t
TableData::CommonProperties::GetContentColor(rgb_color* color)
{
	return _GetValue(fContentColor,
					 &CommonProperties::GetContentColor,
					 color);
}

// GetFont
bigtime_t
TableData::CommonProperties::GetFont(Font* font)
{
	return _GetValue(fFont,
					 &CommonProperties::GetFont,
					 font);
}

// GetHorizontalAligment
bigtime_t
TableData::CommonProperties::GetHorizontalAlignment(uint8* alignment)
{
	return _GetValue(fHorizontalAlignment,
					 &CommonProperties::GetHorizontalAlignment,
					 alignment);
}

// GetVerticalAligment
bigtime_t
TableData::CommonProperties::GetVerticalAlignment(uint8* alignment)
{
	return _GetValue(fVerticalAlignment,
					 &CommonProperties::GetVerticalAlignment,
					 alignment);
}

// operator=
TableData::CommonProperties&
TableData::CommonProperties::operator=(const CommonProperties& other)
{
	fBackgroundColor = other.fBackgroundColor;
	fContentColor = other.fContentColor;
	fFont = other.fFont;
	fHorizontalAlignment = other.fHorizontalAlignment;
	fVerticalAlignment = other.fVerticalAlignment;
	return *this;
}

// SetBackgroundColor
bool
TableData::CommonProperties::SetBackgroundColor(const rgb_color& color)
{
	return fBackgroundColor.Set(color);
}

// SetContentColor
bool
TableData::CommonProperties::SetContentColor(const rgb_color& color)
{
	return fContentColor.Set(color);
}

// SetFont
bool
TableData::CommonProperties::SetFont(const Font& font)
{
	return fFont.Set(font);
}

// SetHorizontalAligment
bool
TableData::CommonProperties::SetHorizontalAlignment(uint8 alignment)
{
	return fHorizontalAlignment.Set(alignment);
}

// SetVerticalAligment
bool
TableData::CommonProperties::SetVerticalAlignment(uint8 alignment)
{
	return fVerticalAlignment.Set(alignment);
}

// #pragma mark -

// constructor
TableData::TableProperties::TableProperties()
	: CommonProperties(NULL, NULL)
{
	fBackgroundColor.Set((rgb_color){ 255, 255, 255, 255 });
	fContentColor.Set((rgb_color){ 0, 0, 0, 255 });
	fColumnWidth.Set(75.0);
	fRowHeight.Set(25.0);
	fHorizontalAlignment.Set(ALIGN_BEGIN);
	fVerticalAlignment.Set(ALIGN_CENTER);
}

// destructor
TableData::TableProperties::~TableProperties()
{
}

//// XMLStore
//status_t
//TableData::TableProperties::XMLStore(XMLHelper& xml) const
//{
//	status_t ret = CommonProperties::XMLStore(xml);
//
//	if (ret == B_OK)
//		ret = _StoreAttribute(fColumnWidth, xml, "COLUMN_WIDTH");
//
//	if (ret == B_OK)
//		ret = _StoreAttribute(fRowHeight, xml, "ROW_HEIGHT");
//
//	return ret;
//}
//
//// XMLRestore
//status_t
//TableData::TableProperties::XMLRestore(XMLHelper& xml)
//{
//	status_t ret = CommonProperties::XMLRestore(xml);
//
//	if (ret == B_OK)
//		ret = _RestoreAttribute(fColumnWidth, xml, "COLUMN_WIDTH");
//
//	if (ret == B_OK)
//		ret = _RestoreAttribute(fRowHeight, xml, "ROW_HEIGHT");
//
//	return ret;
//}

// Archive
status_t
TableData::TableProperties::Archive(BMessage& archive) const
{
	status_t ret = CommonProperties::Archive(archive);

	if (ret == B_OK)
		ret = _StoreAttribute(fColumnWidth, archive, "COLUMN_WIDTH");

	if (ret == B_OK)
		ret = _StoreAttribute(fRowHeight, archive, "ROW_HEIGHT");

	return ret;
}

// Unarchive
status_t
TableData::TableProperties::Unarchive(BMessage& archive)
{
	status_t ret = CommonProperties::Unarchive(archive);

	if (ret == B_OK)
		ret = _RestoreAttribute(fColumnWidth, archive, "COLUMN_WIDTH");

	if (ret == B_OK)
		ret = _RestoreAttribute(fRowHeight, archive, "ROW_HEIGHT");

	return ret;
}

// SetColumnWidth
bool
TableData::TableProperties::SetColumnWidth(float width)
{
	return fColumnWidth.Set(width);
}

// GetColumnWidth
bigtime_t
TableData::TableProperties::GetColumnWidth(float* width)
{
	*width = fColumnWidth.value;
	return fColumnWidth.lastChangedTime;
}

// SetRowHeight
bool
TableData::TableProperties::SetRowHeight(float height)
{
	return fRowHeight.Set(height);
}

// GetRowHeight
bigtime_t
TableData::TableProperties::GetRowHeight(float* height)
{
	*height = fRowHeight.value;
	return fRowHeight.lastChangedTime;
}

// operator=
TableData::TableProperties&
TableData::TableProperties::operator=(const TableProperties& other)
{
	CommonProperties::operator=(other);
	fRowHeight = other.fRowHeight;
	fColumnWidth = other.fColumnWidth;
	return *this;
}


// #pragma mark -

// constructor
TableData::RowProperties::RowProperties(TableProperties* parent)
	: CommonProperties(parent, NULL),
	  fParent(parent)
{
}

// destructor
TableData::RowProperties::~RowProperties()
{
}

//// XMLStore
//status_t
//TableData::RowProperties::XMLStore(XMLHelper& xml) const
//{
//	status_t ret = CommonProperties::XMLStore(xml);
//
//	if (ret == B_OK)
//		ret = _StoreAttribute(fHeight, xml, "HEIGHT");
//
//	return ret;
//}
//
//// XMLRestore
//status_t
//TableData::RowProperties::XMLRestore(XMLHelper& xml)
//{
//	status_t ret = CommonProperties::XMLRestore(xml);
//
//	if (ret == B_OK)
//		ret = _RestoreAttribute(fHeight, xml, "HEIGHT");
//
//	return ret;
//}

// Archive
status_t
TableData::RowProperties::Archive(BMessage& archive) const
{
	status_t ret = CommonProperties::Archive(archive);

	if (ret == B_OK)
		ret = _StoreAttribute(fHeight, archive, "HEIGHT");

	return ret;
}

// Unarchive
status_t
TableData::RowProperties::Unarchive(BMessage& archive)
{
	status_t ret = CommonProperties::Unarchive(archive);

	if (ret == B_OK)
		ret = _RestoreAttribute(fHeight, archive, "HEIGHT");

	return ret;
}

// SetHeight
bool
TableData::RowProperties::SetHeight(float height)
{
	return fHeight.Set(height);
}

// GetHeight
bigtime_t
TableData::RowProperties::GetHeight(float* height)
{
	if (fHeight.IsSet()) {
		*height = fHeight.value;
		return fHeight.lastChangedTime;
	}
	return fParent->GetRowHeight(height);
}

// operator=
TableData::RowProperties&
TableData::RowProperties::operator=(const RowProperties& other)
{
	CommonProperties::operator=(other);
	fHeight = other.fHeight;
	return *this;
}


// #pragma mark -

// constructor
TableData::ColumnProperties::ColumnProperties(TableProperties* parent)
	: CommonProperties(parent, NULL),
	  fParent(parent)
{
}

// destructor
TableData::ColumnProperties::~ColumnProperties()
{
}

//// XMLStore
//status_t
//TableData::ColumnProperties::XMLStore(XMLHelper& xml) const
//{
//	status_t ret = CommonProperties::XMLStore(xml);
//
//	if (ret == B_OK)
//		ret = _StoreAttribute(fWidth, xml, "WIDTH");
//
//	return ret;
//}
//
//// XMLRestore
//status_t
//TableData::ColumnProperties::XMLRestore(XMLHelper& xml)
//{
//	status_t ret = CommonProperties::XMLRestore(xml);
//
//	if (ret == B_OK)
//		ret = _RestoreAttribute(fWidth, xml, "WIDTH");
//
//	return ret;
//}

// Archive
status_t
TableData::ColumnProperties::Archive(BMessage& archive) const
{
	status_t ret = CommonProperties::Archive(archive);

	if (ret == B_OK)
		ret = _StoreAttribute(fWidth, archive, "WIDTH");

	return ret;
}

// Unarchive
status_t
TableData::ColumnProperties::Unarchive(BMessage& archive)
{
	status_t ret = CommonProperties::Unarchive(archive);

	if (ret == B_OK)
		ret = _RestoreAttribute(fWidth, archive, "WIDTH");

	return ret;
}

// SetWidth
bool
TableData::ColumnProperties::SetWidth(float width)
{
	return fWidth.Set(width);
}

// GetWidth
bigtime_t
TableData::ColumnProperties::GetWidth(float* width)
{
	if (fWidth.IsSet()) {
		*width = fWidth.value;
		return fWidth.lastChangedTime;
	}
	return fParent->GetColumnWidth(width);
}

// operator=
TableData::ColumnProperties&
TableData::ColumnProperties::operator=(const ColumnProperties& other)
{
	CommonProperties::operator=(other);
	fWidth = other.fWidth;
	return *this;
}

// #pragma mark -

// constructor
TableData::CellProperties::CellProperties(CommonProperties* column,
										  CommonProperties* row)
	: CommonProperties(column, row),
	  selectable(NULL),
	  fText()
{
}

// destructor
TableData::CellProperties::~CellProperties()
{
	delete selectable;
}

//// XMLStore
//status_t
//TableData::CellProperties::XMLStore(XMLHelper& xml) const
//{
//	status_t ret = CommonProperties::XMLStore(xml);
//
//	if (ret == B_OK)
//		ret = _StoreAttribute(fText, xml, "TEXT");
//
//	return ret;
//}
//
//// XMLRestore
//status_t
//TableData::CellProperties::XMLRestore(XMLHelper& xml)
//{
//	status_t ret = CommonProperties::XMLRestore(xml);
//
//	// NOTE: we can't use the generic _RestoreAttribute() here,
//	// since XMLHelper::GetAttribute(BString& defaultValue) returns
//	// status_t instead of a BString!
//	if (ret == B_OK && xml.OpenTag("TEXT") == B_OK) {
//		fText.value = xml.GetAttribute("value", fText.value.String());
//		fText.lastChangedTime = xml.GetAttribute("changed_time",
//												 fText.lastChangedTime);
//		ret = xml.CloseTag();
//	}
//
//	return ret;
//}

// Archive
status_t
TableData::CellProperties::Archive(BMessage& archive) const
{
	status_t ret = CommonProperties::Archive(archive);

	if (ret == B_OK)
		ret = _StoreAttribute(fText, archive, "TEXT");

	return ret;
}

// Unarchive
status_t
TableData::CellProperties::Unarchive(BMessage& archive)
{
	status_t ret = CommonProperties::Unarchive(archive);

	if (ret == B_OK)
		ret = _RestoreAttribute(fText, archive, "TEXT");

	return ret;
}

// GetText
bigtime_t
TableData::CellProperties::GetText(BString* text)
{
	*text = fText.value;
	return fText.lastChangedTime;
}

// SetText
bool
TableData::CellProperties::SetText(const BString& text)
{
	return fText.Set(text);
}

// operator=
TableData::CellProperties&
TableData::CellProperties::operator=(const CellProperties& other)
{
	CommonProperties::operator=(other);
	fText = other.fText;
	return *this;
}

// #pragma mark - SelectableCell

// constructor
TableData::SelectableCell::SelectableCell()
	: fTable(NULL)
{
	fColumn = new IntProperty(PROPERTY_CELL_COLUMN, -1);
	fColumn->SetEditable(false);
	fRow = new IntProperty(PROPERTY_CELL_ROW, -1);
	fRow->SetEditable(false);

	fText = new StringProperty(PROPERTY_TEXT, "");

	fHorizontalAlignment = new OptionProperty(PROPERTY_HORIZONTAL_ALIGNMENT);
	fVerticalAlignment = new OptionProperty(PROPERTY_VERTICAL_ALIGNMENT);
	fFont = new FontProperty(PROPERTY_FONT, *be_bold_font);
	fFontSize = new FloatProperty(PROPERTY_FONT_SIZE,
								  kDefaultFontSize, 1.0, 500.0);

	fBackgroundColor = new ColorProperty(PROPERTY_BACKGROUND_COLOR);
	fContentColor = new ColorProperty(PROPERTY_COLOR);

	fHorizontalAlignment->AddOption(ALIGN_BEGIN, "Left");
	fHorizontalAlignment->AddOption(ALIGN_CENTER, "Center");
	fHorizontalAlignment->AddOption(ALIGN_END, "Right");
	fHorizontalAlignment->AddOption(ALIGN_STRETCH, "Justify");
	fHorizontalAlignment->SetCurrentOptionID(ALIGN_CENTER);

	fVerticalAlignment->AddOption(ALIGN_BEGIN, "Top");
	fVerticalAlignment->AddOption(ALIGN_CENTER, "Center");
	fVerticalAlignment->AddOption(ALIGN_END, "Bottom");
	fVerticalAlignment->AddOption(ALIGN_STRETCH, "Stretch");
	fVerticalAlignment->SetCurrentOptionID(ALIGN_CENTER);

	AddProperty(fColumn);
	AddProperty(fRow);

	AddProperty(fText);
	AddProperty(fHorizontalAlignment);
	AddProperty(fVerticalAlignment);
	AddProperty(fFont);
	AddProperty(fFontSize);
	AddProperty(fBackgroundColor);
	AddProperty(fContentColor);
}

// destructor
TableData::SelectableCell::~SelectableCell()
{
}

// SelectedChanged
void
TableData::SelectableCell::SelectedChanged()
{
	printf("SelectedChanged()\n");
	// TODO: control focus in TableManipulator!
}

// SetTo
void
TableData::SelectableCell::SetTo(TableData* table,
								 uint32 column, uint32 row)
{
	if (fTable)
		fTable->RemoveListener(this);

	fTable = table;

	if (!fTable)
		return;

	fTable->AddListener(this);


	fColumn->SetValue(column);
	fRow->SetValue(row);

	fText->SetValue(fTable->CellText(column, row));

	fHorizontalAlignment->SetCurrentOptionID(
		fTable->CellHorizontalAlignment(column, row));
	fVerticalAlignment->SetCurrentOptionID(
		fTable->CellVerticalAlignment(column, row));

	Font font;
	fTable->GetCellFont(column, row, &font);

	fFont->SetValue(font);
	fFontSize->SetValue(font.Size());

	fBackgroundColor->SetValue(fTable->CellBackgroundColor(column, row));
	fContentColor->SetValue(fTable->CellContentColor(column, row));
}

// ValueChanged
void
TableData::SelectableCell::ValueChanged(Property* property)
{
	if (!fTable)
		return;

	int32 column = fColumn->Value();
	int32 row = fRow->Value();

	switch (property->Identifier()) {
		case PROPERTY_CELL_COLUMN:
printf("PROPERTY_CELL_COLUMN\n");
			// TODO: change focused cell in manipulator
			break;
		case PROPERTY_CELL_ROW:
printf("PROPERTY_CELL_ROW\n");
			// TODO: change focused cell in manipulator
			break;

		case PROPERTY_TEXT:
			fTable->SetCellText(column, row, fText->Value());
			break;

		case PROPERTY_HORIZONTAL_ALIGNMENT:
			fTable->SetCellHorizontalAligment(column, row,
				(uint8)fHorizontalAlignment->CurrentOptionID());
			break;
		case PROPERTY_VERTICAL_ALIGNMENT:
			fTable->SetCellVerticalAligment(column, row,
				(uint8)fVerticalAlignment->CurrentOptionID());
			break;

		case PROPERTY_BACKGROUND_COLOR:
			fTable->SetCellBackgroundColor(column, row,
				fBackgroundColor->Value());
			break;
		case PROPERTY_COLOR:
			fTable->SetCellContentColor(column, row,
				fContentColor->Value());
			break;

		case PROPERTY_FONT:
		case PROPERTY_FONT_SIZE: {
			Font font = fFont->Value();
			font.SetSize(fFontSize->Value());
			fTable->SetCellFont(column, row, font);
			break;
		}
	}

	PropertyObject::ValueChanged(property);
}

// TableDataChanged
void
TableData::SelectableCell::TableDataChanged(
									const TableData::Event& event)
{
	if (fTable != event.table
		|| (event.column >= 0 && event.column != fColumn->Value())
		|| (event.row >= 0 && event.row != fRow->Value()))
		return;

	switch (event.property) {
		case TABLE_CELL_TEXT:
			fText->SetValue(fTable->CellText(fColumn->Value(),
											 fRow->Value()));
			break;
		case TABLE_CELL_BACKGROUND_COLOR:
			// TODO
			break;
		case TABLE_CELL_CONTENT_COLOR:
			// TODO
			break;
		case TABLE_CELL_HORIZONTAL_ALIGNMENT:
			fHorizontalAlignment->SetCurrentOptionID(
				fTable->CellHorizontalAlignment(fColumn->Value(),
												fRow->Value()));
			break;
		case TABLE_CELL_VERTICAL_ALIGNMENT:
			fVerticalAlignment->SetCurrentOptionID(
				fTable->CellVerticalAlignment(fColumn->Value(),
											  fRow->Value()));
			break;
		case TABLE_CELL_FONT:
			// TODO
			break;
	}
}

// #pragma mark - TableData

// constructor
TableData::TableData()
	: fTableProperties(new (nothrow) TableProperties()),

	  fColumns(20),
	  fRows(20),

	  fColumnCount(0),
	  fRowCount(0),

	  fListeners(4)
{
//	SetDimensions(2, 3);
//	SetCellText(0, 0, "1. FC Bochum vs. Bayern MÜnchen");
//	SetCellText(0, 1, "Energie Cottbus vs. Werder Bremen");
//	SetCellText(0, 2, "1. FC Rostock vs. Dynamo Dresden");
//	SetCellText(1, 0, "2:1");
//	SetCellText(1, 1, "7:0");
//	SetCellText(1, 2, "0:2");
//
//	SetColumnWidth(0, 500);
//	SetColumnWidth(1, 100);
//
//	SetCellHorizontalAligment(0, 1, ALIGN_CENTER);
//	SetCellVerticalAligment(0, 1, ALIGN_CENTER);
//	SetCellHorizontalAligment(0, 2, ALIGN_END);
//	SetCellVerticalAligment(0, 2, ALIGN_END);
}

// constructor
TableData::TableData(const TableData& other)
	: fTableProperties(new (nothrow) TableProperties()),

	  fColumns(20),
	  fRows(20),

	  fColumnCount(0),
	  fRowCount(0),

	  fListeners(4)
{
	*this = other;
}

// destructor
TableData::~TableData()
{
	for (uint32 i = 0; i < fRowCount; i++) {
		RowWithCells* row = (RowWithCells*)fRows.ItemAtFast(i);
		for (uint32 j = 0; j < fColumnCount; j++)
			delete (CellProperties*)row->cells.ItemAtFast(j);
		delete row;
	}

	for (uint32 i = 0; i < fColumnCount; i++)
		delete (ColumnProperties*)fColumns.ItemAtFast(i);

	delete fTableProperties;
}

// #pragma mark -

//// XMLStore
//status_t
//TableData::XMLStore(XMLHelper& xml) const
//{
//	status_t ret = InitCheck();
//	if (ret < B_OK)
//		return ret;
//
//	ret = xml.CreateTag("TABLE");
//
//	if (ret == B_OK)
//		ret = xml.SetAttribute("columns", (int32)fColumnCount);
//	if (ret == B_OK)
//		ret = xml.SetAttribute("rows", (int32)fRowCount);
//	if (ret == B_OK)
//		ret = xml.StoreObject(fTableProperties);
//
//	if (ret == B_OK)
//		ret = xml.CreateTag("COLUMNS");
//
//	if (ret == B_OK) {
//		int32 columnCount = min_c((int32)fColumnCount, fColumns.CountItems());
//		for (int32 i = 0; i < columnCount; i++) {
//			ColumnProperties* column
//				= (ColumnProperties*)fColumns.ItemAtFast(i);
//
//			ret = xml.CreateTag("COLUMN");
//			if (ret < B_OK)
//				break;
//			ret = xml.StoreObject(column);
//			if (ret < B_OK)
//				break;
//			ret = xml.CloseTag(); // COLUMN
//			if (ret < B_OK)
//				break;
//		}
//
//		if (ret == B_OK)
//			ret = xml.CloseTag(); // COLUMNS
//	}
//
//	if (ret == B_OK)
//		ret = xml.CreateTag("ROWS");
//
//	if (ret == B_OK) {
//		int32 rowCount = min_c((int32)fRowCount, fRows.CountItems());
//		for (int32 i = 0; i < rowCount; i++) {
//			RowWithCells* row = (RowWithCells*)fRows.ItemAtFast(i);
//
//
//			ret = xml.CreateTag("ROW");
//			if (ret < B_OK)
//				break;
//			ret = xml.StoreObject(row->properties);
//			if (ret < B_OK)
//				break;
//
//			int32 cellCount = min_c((int32)fColumnCount,
//									row->cells.CountItems());
//			for (int32 j = 0; j < cellCount; j++) {
//				CellProperties* cell
//					= (CellProperties*)row->cells.ItemAtFast(j);
//
//				ret = xml.CreateTag("CELL");
//				if (ret < B_OK)
//					break;
//				ret = xml.StoreObject(cell);
//				if (ret < B_OK)
//					break;
//				ret = xml.CloseTag(); // CELL
//				if (ret < B_OK)
//					break;
//			}
//
//			ret = xml.CloseTag(); // ROW
//			if (ret < B_OK)
//				break;
//		}
//
//		if (ret == B_OK)
//			ret = xml.CloseTag(); // ROWS
//	}
//
//	if (ret == B_OK)
//		ret = xml.CloseTag(); // TABLE
//
//	return ret;
//}
//
//// XMLRestore
//status_t
//TableData::XMLRestore(XMLHelper& xml)
//{
//	status_t ret = xml.OpenTag("TABLE");
//	if (ret < B_OK)
//		return ret;
//
//	int32 columns = xml.GetAttribute("columns", int32(-1));
//	int32 rows = xml.GetAttribute("rows", int32(-1));
//	if (columns < 0 || rows < 0 || columns > 10000 || rows > 10000
//		|| !SetDimensions((uint32)columns, (uint32)rows))
//		return B_ERROR;
//
//	ret = xml.RestoreObject(fTableProperties);
//
//	if (ret == B_OK) {
//		ret = xml.OpenTag("COLUMNS");
//
//		if (ret == B_OK) {
//			for (uint32 i = 0; i < fColumnCount; i++) {
//				ColumnProperties* column
//					= (ColumnProperties*)fColumns.ItemAtFast(i);
//
//				ret = xml.OpenTag("COLUMN");
//				if (ret < B_OK)
//					break;
//				ret = xml.RestoreObject(column);
//				if (ret < B_OK)
//					break;
//				ret = xml.CloseTag(); // COLUMN
//				if (ret < B_OK)
//					break;
//			}
//		}
//
//		if (ret == B_OK)
//			ret = xml.CloseTag(); // COLUMNS
//	}
//
//	if (ret == B_OK) {
//		ret = xml.OpenTag("ROWS");
//
//		if (ret == B_OK) {
//			for (uint32 i = 0; i < fRowCount; i++) {
//				RowWithCells* row = (RowWithCells*)fRows.ItemAtFast(i);
//
//
//				ret = xml.OpenTag("ROW");
//				if (ret < B_OK)
//					break;
//				ret = xml.RestoreObject(row->properties);
//				if (ret < B_OK)
//					break;
//
//				for (uint32 j = 0; j < fColumnCount; j++) {
//					CellProperties* cell
//						= (CellProperties*)row->cells.ItemAtFast(j);
//
//					ret = xml.OpenTag("CELL");
//					if (ret < B_OK)
//						break;
//					ret = xml.RestoreObject(cell);
//					if (ret < B_OK)
//						break;
//					ret = xml.CloseTag(); // CELL
//					if (ret < B_OK)
//						break;
//				}
//
//				ret = xml.CloseTag(); // ROW
//				if (ret < B_OK)
//					break;
//			}
//		}
//
//		if (ret == B_OK)
//			ret = xml.CloseTag(); // COLUMNS
//	}
//
//	if (ret == B_OK)
//		ret = xml.CloseTag(); // TABLE
//
//	return ret;
//}

// Archive
status_t
TableData::Archive(BMessage* into, bool deep) const
{
	if (into == NULL)
		return B_BAD_VALUE;
	status_t ret = InitCheck();
	if (ret < B_OK)
		return ret;

	AttributeMessage tableArchive;

	if (ret == B_OK)
		ret = tableArchive.SetAttribute("columns", (int32)fColumnCount);
	if (ret == B_OK)
		ret = tableArchive.SetAttribute("rows", (int32)fRowCount);
	if (ret == B_OK)
		ret = fTableProperties->Archive(tableArchive);

	if (ret == B_OK) {
		BMessage columnsArchive;
		int32 columnCount = min_c((int32)fColumnCount, fColumns.CountItems());
		for (int32 i = 0; i < columnCount; i++) {
			ColumnProperties* column
				= (ColumnProperties*)fColumns.ItemAtFast(i);

			BMessage columnArchive;
			ret = column->Archive(columnArchive);
			if (ret < B_OK)
				break;
			ret = columnsArchive.AddMessage("COLUMN", &columnArchive);
			if (ret < B_OK)
				break;
		}

		if (ret == B_OK)
			ret = tableArchive.AddMessage("COLUMNS", &columnsArchive);
	}

	if (ret == B_OK) {
		BMessage rowsArchive;
		int32 rowCount = min_c((int32)fRowCount, fRows.CountItems());
		for (int32 i = 0; i < rowCount; i++) {
			RowWithCells* row = (RowWithCells*)fRows.ItemAtFast(i);

			BMessage rowArchive;
			ret = row->properties.Archive(rowArchive);
			if (ret < B_OK)
				break;

			int32 cellCount = min_c((int32)fColumnCount,
				row->cells.CountItems());
			for (int32 j = 0; j < cellCount; j++) {
				CellProperties* cell
					= (CellProperties*)row->cells.ItemAtFast(j);

				BMessage cellArchive;
				ret = cell->Archive(cellArchive);
				if (ret < B_OK)
					break;
				ret = rowArchive.AddMessage("CELL", &cellArchive);
				if (ret < B_OK)
					break;
			}

			ret = rowsArchive.AddMessage("ROW", &rowArchive);
			if (ret < B_OK)
				break;
		}

		if (ret == B_OK)
			ret = tableArchive.AddMessage("ROWS", &rowsArchive);
	}

	if (ret == B_OK)
		ret = into->AddMessage("TABLE", &tableArchive);

	return ret;
}

// Unarchive
status_t
TableData::Unarchive(const BMessage* from)
{
	if (from == NULL)
		return B_BAD_VALUE;

	AttributeMessage tableArchive;
	status_t ret = from->FindMessage("TABLE", &tableArchive);
	if (ret < B_OK)
		return ret;

	int32 columns = tableArchive.GetAttribute("columns", int32(-1));
	int32 rows = tableArchive.GetAttribute("rows", int32(-1));
	if (columns < 0 || rows < 0 || columns > 10000 || rows > 10000
		|| !SetDimensions((uint32)columns, (uint32)rows))
		return B_ERROR;

	ret = fTableProperties->Unarchive(tableArchive);

	if (ret == B_OK) {
		BMessage columnsArchive;
		ret = tableArchive.FindMessage("COLUMNS", &columnsArchive);

		if (ret == B_OK) {
			for (uint32 i = 0; i < fColumnCount; i++) {
				ColumnProperties* column
					= (ColumnProperties*)fColumns.ItemAtFast(i);

				BMessage columnArchive;
				ret = columnsArchive.FindMessage("COLUMN", i, &columnArchive);
				if (ret < B_OK)
					break;
				ret = column->Unarchive(columnArchive);
				if (ret < B_OK)
					break;
			}
		}
	}

	if (ret == B_OK) {
		BMessage rowsArchive;
		ret = tableArchive.FindMessage("ROWS", &rowsArchive);

		if (ret == B_OK) {
			for (uint32 i = 0; i < fRowCount; i++) {
				RowWithCells* row = (RowWithCells*)fRows.ItemAtFast(i);

				BMessage rowArchive;
				ret = rowsArchive.FindMessage("ROW", i, &rowArchive);
				if (ret < B_OK)
					break;
				ret = row->properties.Unarchive(rowArchive);
				if (ret < B_OK)
					break;

				for (uint32 j = 0; j < fColumnCount; j++) {
					CellProperties* cell
						= (CellProperties*)row->cells.ItemAtFast(j);

					BMessage cellArchive;
					ret = rowArchive.FindMessage("CELL", j, &cellArchive);
					if (ret < B_OK)
						break;
					ret = cell->Unarchive(cellArchive);
					if (ret < B_OK)
						break;
				}

				if (ret < B_OK)
					break;
			}
		}
	}

	return ret;
}

// #pragma mark -

// InitCheck
status_t
TableData::InitCheck() const
{
	if (!fTableProperties)
		return B_NO_MEMORY;

	return B_OK;
}

// operator =
TableData&
TableData::operator=(const TableData& other)
{
	if (other.InitCheck() < B_OK || &other == this)
		return *this;

	if (!SetDimensions(other.fColumnCount, other.fRowCount))
		return *this;

	*fTableProperties = *other.fTableProperties;

	for (uint32 i = 0; i < fColumnCount; i++) {
		ColumnProperties* ourColumn
			= (ColumnProperties*)fColumns.ItemAtFast(i);
		ColumnProperties* otherColumn
			= (ColumnProperties*)other.fColumns.ItemAtFast(i);
		*ourColumn = *otherColumn;
	}

	for (uint32 i = 0; i < fRowCount; i++) {
		RowWithCells* ourRow = (RowWithCells*)fRows.ItemAtFast(i);
		RowWithCells* otherRow = (RowWithCells*)other.fRows.ItemAtFast(i);
		ourRow->properties = otherRow->properties;
		for (uint32 j = 0; j < fColumnCount; j++) {
			CellProperties* ourCell
				= (CellProperties*)ourRow->cells.ItemAtFast(j);
			CellProperties* otherCell
				= (CellProperties*)otherRow->cells.ItemAtFast(j);
			*ourCell = *otherCell;
		}
	}

	return *this;
}

// SetDimensions
bool
TableData::SetDimensions(uint32 columns, uint32 rows)
{
	if (fColumnCount == columns && fRowCount == rows)
		return true;

	// adjust column count and create new properties as needed
	uint32 allocatedColumnCount = fColumns.CountItems();
	for (uint32 i = allocatedColumnCount; i < columns; i++) {
		ColumnProperties* properties
			= new (nothrow) ColumnProperties(fTableProperties);
		if (!properties || !fColumns.AddItem(properties)) {
			delete properties;
			return false;
		}
	}
	fColumnCount = columns;

	// adjust row count and create new row properties as needed
	uint32 allocatedRowCount = fRows.CountItems();

	// create new cells
	if (allocatedColumnCount < columns) {
		// extend existing rows with new cells
		for (uint32 i = 0; i < allocatedRowCount; i++) {
			RowWithCells* row = (RowWithCells*)fRows.ItemAtFast(i);
			// create cells for new columns
			if (!_CreateCells(row, allocatedColumnCount, columns - 1))
				return false;
		}
	}

	for (uint32 i = allocatedRowCount; i < rows; i++) {
		RowWithCells* row
			= new (nothrow) RowWithCells(fTableProperties);
		if (!row || !fRows.AddItem(row)) {
			delete row;
			return false;
		}
		// create new cells
		if (!_CreateCells(row, 0, columns - 1))
			return false;
	}
	fRowCount = rows;

	_Notify(Event(this, TABLE_DIMENSIONS));

	return true;
}

// #pragma mark -

// Bounds
BRect
TableData::Bounds() const
{
	float width = 0.0;
	for (uint32 i = 0; i < fColumnCount; i++)
		width += ColumnWidth(i);

	float height = 0.0;
	for (uint32 i = 0; i < fRowCount; i++)
		height += RowHeight(i);

	return BRect(0, 0, width - 1, height - 1);
}

// Bounds
BRect
TableData::Bounds(int32 column, int32 row) const
{
	BRect bounds(Bounds());

	if (column >= 0 && column < (int32)fColumnCount) {
		float pos = 0.0;
		for (int32 i = 0; i < column; i++)
			pos += ColumnWidth(i);

		bounds.left = pos;
		bounds.right = pos + ColumnWidth(column) - 1;
	}

	if (row >= 0 && row < (int32)fRowCount) {
		float pos = 0.0;
		for (int32 i = 0; i < row; i++)
			pos += RowHeight(i);

		bounds.top = pos;
		bounds.bottom = pos + RowHeight(row) - 1;
	}

	return bounds;
}

// SelectCell
Selectable*
TableData::SelectCell(uint32 column, uint32 row)
{
	if (CellProperties* cell = _CellAt(column, row)) {
		if (!cell->selectable) {
			cell->selectable = new SelectableCell();
		}
		cell->selectable->SetTo(this, column, row);
		return cell->selectable;
	}

	return NULL;
}

// ColumnWidth
float
TableData::ColumnWidth(uint32 columnIndex) const
{
	float value;
	if (ColumnProperties* column = _ColumnAt(columnIndex))
		column->GetWidth(&value);
	else
		fTableProperties->GetColumnWidth(&value);
	return value;
}

// RowHeight
float
TableData::RowHeight(uint32 rowIndex) const
{
	float value;
	if (RowProperties* row = _RowAt(rowIndex))
		row->GetHeight(&value);
	else
		fTableProperties->GetRowHeight(&value);
	return value;
}

// GetDefaultFont
void
TableData::GetDefaultFont(Font* font) const
{
	fTableProperties->GetFont(font);
}

// DefaultHorizontalAlignment
uint8
TableData::DefaultHorizontalAlignment() const
{
	uint8 alignment;
	fTableProperties->GetHorizontalAlignment(&alignment);
	return alignment;
}

// DefaultVerticalAlignment
uint8
TableData::DefaultVerticalAlignment() const
{
	uint8 alignment;
	fTableProperties->GetVerticalAlignment(&alignment);
	return alignment;
}

// CellBackgroundColor
rgb_color
TableData::CellBackgroundColor(uint32 column, uint32 row) const
{
	rgb_color value;
	if (CellProperties* cell = _CellAt(column, row))
		cell->GetBackgroundColor(&value);
	else
		fTableProperties->GetBackgroundColor(&value);
	return value;
}

// CellContentColor
rgb_color
TableData::CellContentColor(uint32 column, uint32 row) const
{
	rgb_color value;
	if (CellProperties* cell = _CellAt(column, row))
		cell->GetContentColor(&value);
	else
		fTableProperties->GetContentColor(&value);
	return value;
}

// GetCellFont
void
TableData::GetCellFont(uint32 column, uint32 row, Font* font) const
{
	if (CellProperties* cell = _CellAt(column, row))
		cell->GetFont(font);
	else
		fTableProperties->GetFont(font);
}

// CellHorizontalAligment
uint8
TableData::CellHorizontalAlignment(uint32 column, uint32 row) const
{
	uint8 value;
	if (CellProperties* cell = _CellAt(column, row))
		cell->GetHorizontalAlignment(&value);
	else
		fTableProperties->GetHorizontalAlignment(&value);
	return value;
}

// CellVerticalAligment
uint8
TableData::CellVerticalAlignment(uint32 column, uint32 row) const
{
	uint8 value;
	if (CellProperties* cell = _CellAt(column, row))
		cell->GetVerticalAlignment(&value);
	else
		fTableProperties->GetVerticalAlignment(&value);
	return value;
}

// CellText
BString
TableData::CellText(uint32 column, uint32 row) const
{
	BString value;
	if (CellProperties* cell = _CellAt(column, row))
		cell->GetText(&value);
	return value;
}

// #pragma mark -

// SetDefaultColumnWidth
void
TableData::SetDefaultColumnWidth(float width)
{
	if (fTableProperties->SetColumnWidth(width))
		_Notify(Event(this, TABLE_COLUMN_WIDTH));
}

// SetDefaultRowHeight
void
TableData::SetDefaultRowHeight(float height)
{
	if (fTableProperties->SetRowHeight(height))
		_Notify(Event(this, TABLE_ROW_HEIGHT));
}

// SetDefaultHorizontalAlignment
void
TableData::SetDefaultHorizontalAlignment(uint8 alignment)
{
	if (fTableProperties->SetHorizontalAlignment(alignment))
		_Notify(Event(this, TABLE_CELL_HORIZONTAL_ALIGNMENT));
}

// SetDefaultVerticalAlignment
void
TableData::SetDefaultVerticalAlignment(uint8 alignment)
{
	if (fTableProperties->SetVerticalAlignment(alignment))
		_Notify(Event(this, TABLE_CELL_VERTICAL_ALIGNMENT));
}

// SetDefaultFont
void
TableData::SetDefaultFont(const Font& font)
{
	if (fTableProperties->SetFont(font))
		_Notify(Event(this, TABLE_CELL_FONT));
}

// SetColumnWidth
void
TableData::SetColumnWidth(uint32 columnIndex, float width)
{
	if (ColumnProperties* column = _ColumnAt(columnIndex)) {
		if (column->SetWidth(width))
			_Notify(Event(this, TABLE_COLUMN_WIDTH, columnIndex));
	}
}

// SetRowHeight
void
TableData::SetRowHeight(uint32 rowIndex, float height)
{
	if (RowProperties* row = _RowAt(rowIndex)) {
		if (row->SetHeight(height))
			_Notify(Event(this, TABLE_ROW_HEIGHT, -1, rowIndex));
	}
}

// #pragma mark -

// SetCellHorizontalAligment
void
TableData::SetCellHorizontalAligment(uint32 column, uint32 row,
									 uint8 alignment)
{
	if (CellProperties* cell = _CellAt(column, row)) {
		if (cell->SetHorizontalAlignment(alignment))
			_Notify(Event(this, TABLE_CELL_HORIZONTAL_ALIGNMENT,
						  column, row));
	}
}

// SetCellVerticalAligment
void
TableData::SetCellVerticalAligment(uint32 column, uint32 row,
								   uint8 alignment)
{
	if (CellProperties* cell = _CellAt(column, row)) {
		if (cell->SetVerticalAlignment(alignment))
			_Notify(Event(this, TABLE_CELL_VERTICAL_ALIGNMENT, column, row));
	}
}

// SetCellFont
void
TableData::SetCellFont(uint32 column, uint32 row, const Font& font)
{
	if (CellProperties* cell = _CellAt(column, row)) {
		if (cell->SetFont(font))
			_Notify(Event(this, TABLE_CELL_FONT, column, row));
	}
}

// SetCellText
void
TableData::SetCellText(uint32 column, uint32 row, const BString& text)
{
	if (CellProperties* cell = _CellAt(column, row)) {
		if (cell->SetText(text))
			_Notify(Event(this, TABLE_CELL_TEXT, column, row));
	}
}

// SetCellBackgroundColor
void
TableData::SetCellBackgroundColor(uint32 column, uint32 row,
								  const rgb_color& color)
{
	if (CellProperties* cell = _CellAt(column, row)) {
		if (cell->SetBackgroundColor(color))
			_Notify(Event(this, TABLE_CELL_BACKGROUND_COLOR, column, row));
	}
}

// SetCellContentColor
void
TableData::SetCellContentColor(uint32 column, uint32 row,
							   const rgb_color& color)
{
	if (CellProperties* cell = _CellAt(column, row)) {
		if (cell->SetContentColor(color))
			_Notify(Event(this, TABLE_CELL_CONTENT_COLOR, column, row));
	}
}

// #pragma mark -

// AddListener
bool
TableData::AddListener(Listener* listener)
{
	if (!listener || fListeners.HasItem(listener))
		return false;

	return fListeners.AddItem(listener);
}

// RemoveListener
void
TableData::RemoveListener(Listener* listener)
{
	fListeners.RemoveItem(listener);
}

// #pragma mark -

bool
TableData::_CreateCells(RowWithCells* row, uint32 fromIndex, uint32 toIndex)
{
	for (uint32 i = fromIndex; i <= toIndex; i++) {
		ColumnProperties* column
			= (ColumnProperties*)fColumns.ItemAtFast(i);
		CellProperties* cell
			= new (nothrow) CellProperties(column, &row->properties);
		if (!cell || !row->cells.AddItem(cell)) {
			delete cell;
			return false;
		}
	}
	return true;
}

// _ColumnAt
TableData::ColumnProperties*
TableData::_ColumnAt(uint32 index) const
{
	return (ColumnProperties*)fColumns.ItemAt(index);
}

// _RowAt
TableData::RowProperties*
TableData::_RowAt(uint32 index) const
{
	RowWithCells* row = (RowWithCells*)fRows.ItemAt(index);
	return (row ? &row->properties : NULL);
}

// _CellAt
TableData::CellProperties*
TableData::_CellAt(uint32 columnIndex, uint32 rowIndex) const
{
	RowWithCells* row = (RowWithCells*)fRows.ItemAt(rowIndex);
	return (row ? (CellProperties*)row->cells.ItemAt(columnIndex) : NULL);
}

// #pragma mark -

// _Notify
void
TableData::_Notify(const Event& event)
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		((Listener*)listeners.ItemAtFast(i))->TableDataChanged(event);
	}
}

TableData::Listener::Listener() {}
TableData::Listener::~Listener() {}

TableData::Event::Event(TableData* table, uint32 property,
						int32 column, int32 row)
	: table(table),
	  property(property),
	  column(column),
	  row(row)
{}

TableData::Event::~Event() {}

// #pragma mark - TableClip


// constructor
TableClip::TableClip(const char* name)
	: Clip("TableClip", name),

	  fTableData()
{
	_Init();

	// sync properties with table
	// TODO: maybe it's better to sync all of them?
	// TODO: should we listen on the table an transfer values
	// to the properties?
	if (fColumnCount && fRowCount) {
		fTableData.SetDimensions(fColumnCount->Value(),
								 fRowCount->Value());
	}
	if (fFont && fFontSize) {
		Font font(fFont->Value());
		font.SetSize(fFontSize->Value());
		fTableData.SetDefaultFont(font);
	}
}

// constructor
TableClip::TableClip(const TableClip& other)
	: Clip(other, true),

	  fTableData(other.fTableData)
{
	_Init();
}

// destructor
TableClip::~TableClip()
{
}

// SetTo
status_t
TableClip::SetTo(const ServerObject* _other)
{
	const TableClip* other = dynamic_cast<const TableClip*>(_other);
	if (!other)
		return B_ERROR;

	fTableData = other->fTableData;

	return Clip::SetTo(other);
}

// IsMetaDataOnly
bool
TableClip::IsMetaDataOnly() const
{
	return false;
}

// Duration
uint64
TableClip::Duration()
{
	return 0;
}

// Bounds
BRect
TableClip::Bounds(BRect canvasBounds)
{
	return fTableData.Bounds();
}

// GetIcon
bool
TableClip::GetIcon(BBitmap* icon)
{
	return GetBuiltInIcon(icon, kTableIcon);
}

// InitCheck
status_t
TableClip::InitCheck()
{
	status_t ret = fTableData.InitCheck();
	if (ret < B_OK)
		return ret;

	// TODO: load from FileBasedClip::Ref()

	return B_OK;
}

// #pragma mark -

// ValueChanged
void
TableClip::ValueChanged(Property* property)
{
	switch (property->Identifier()) {
		case PROPERTY_TABLE_COLUMN_COUNT:
		case PROPERTY_TABLE_ROW_COUNT:
			if (fColumnCount && fRowCount) {
				fTableData.SetDimensions(fColumnCount->Value(),
										 fRowCount->Value());
			}
			break;
		case PROPERTY_TABLE_COLUMN_WIDTH:
			if (fColumnWidth)
				fTableData.SetDefaultColumnWidth(fColumnWidth->Value());
			break;
		case PROPERTY_TABLE_ROW_HEIGHT:
			if (fRowHeight)
				fTableData.SetDefaultRowHeight(fRowHeight->Value());
			break;

		case PROPERTY_HORIZONTAL_ALIGNMENT:
			if (fHorizontalAlignment) {
				fTableData.SetDefaultHorizontalAlignment(
					fHorizontalAlignment->CurrentOptionID());
			}
			break;
		case PROPERTY_VERTICAL_ALIGNMENT:
			if (fVerticalAlignment) {
				fTableData.SetDefaultVerticalAlignment(
					fVerticalAlignment->CurrentOptionID());
			}
			break;
		case PROPERTY_FONT:
		case PROPERTY_FONT_SIZE:
			if (fFont && fFontSize) {
				Font font(fFont->Value());
				font.SetSize(fFontSize->Value());
				fTableData.SetDefaultFont(font);
			}
			break;
	}

	Clip::ValueChanged(property);
}

// #pragma mark -

// ColumnSpacing
float
TableClip::ColumnSpacing() const
{
	if (fColumnSpacing)
		return fColumnSpacing->Value();

	return 4.0;
}

// RowSpacing
float
TableClip::RowSpacing() const
{
	if (fRowSpacing)
		return fRowSpacing->Value();

	return 4.0;
}

// RoundCornerRadius
float
TableClip::RoundCornerRadius() const
{
	if (fCornerRadius)
		return fCornerRadius->Value();

	return 4.0;
}

// FadeInMode
int32
TableClip::FadeInMode() const
{
	if (fFadeInMode)
		return fFadeInMode->CurrentOptionID();

	return TABLE_FADE_IN_EXPAND;
}

// FadeInFrames
int64
TableClip::FadeInFrames() const
{
	if (fFadeInFrames)
		return fFadeInFrames->Value();

	return 10;
}

// FadeOutMode
int32
TableClip::FadeOutMode() const
{
	if (fFadeOutMode)
		return fFadeOutMode->CurrentOptionID();

	return TABLE_FADE_IN_EXPAND;
}

// FadeOutFrames
int64
TableClip::FadeOutFrames() const
{
	if (fFadeOutFrames)
		return fFadeOutFrames->Value();

	return 10;
}

// #pragma mark -

// _Init
void
TableClip::_Init()
{
	fColumnCount = dynamic_cast<IntProperty*>(
	  					FindProperty(PROPERTY_TABLE_COLUMN_COUNT));
	fRowCount = dynamic_cast<IntProperty*>(
	  					FindProperty(PROPERTY_TABLE_ROW_COUNT));

	fColumnWidth = dynamic_cast<FloatProperty*>(
	 					FindProperty(PROPERTY_TABLE_COLUMN_WIDTH));
	fColumnSpacing = dynamic_cast<FloatProperty*>(
	  					FindProperty(PROPERTY_TABLE_COLUMN_SPACING));

	fRowHeight = dynamic_cast<FloatProperty*>(
	  					FindProperty(PROPERTY_TABLE_ROW_HEIGHT));
	fRowSpacing = dynamic_cast<FloatProperty*>(
	  					FindProperty(PROPERTY_TABLE_ROW_SPACING));

	fCornerRadius = dynamic_cast<FloatProperty*>(
	  					FindProperty(PROPERTY_TABLE_ROUND_CORNER_RADIUS));

	fHorizontalAlignment = dynamic_cast<OptionProperty*>(
	  					FindProperty(PROPERTY_HORIZONTAL_ALIGNMENT));
	fVerticalAlignment = dynamic_cast<OptionProperty*>(
	  					FindProperty(PROPERTY_VERTICAL_ALIGNMENT));
	fFont = dynamic_cast<FontProperty*>(FindProperty(PROPERTY_FONT));
	fFontSize = dynamic_cast<FloatProperty*>(
	  					FindProperty(PROPERTY_FONT_SIZE));

	fFadeInMode = dynamic_cast<OptionProperty*>(
	  					FindProperty(PROPERTY_TABLE_FADE_IN_MODE));
	fFadeInFrames = dynamic_cast<IntProperty*>(
	  					FindProperty(PROPERTY_TABLE_FADE_IN_FRAMES));
	fFadeOutMode = dynamic_cast<OptionProperty*>(
	  					FindProperty(PROPERTY_TABLE_FADE_OUT_MODE));
	fFadeOutFrames = dynamic_cast<IntProperty*>(
	  					FindProperty(PROPERTY_TABLE_FADE_OUT_FRAMES));
}

