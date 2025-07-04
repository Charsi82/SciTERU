// SciTE - Scintilla based Text Editor
/** @file StripDefinition.h
 ** Definition of classes build user strips from a description string.
 **/
// Copyright 2012 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef STRIPDEFINITION_H
#define STRIPDEFINITION_H

class UserControl {
public:
	static constexpr int widthControl = 20;
	enum UCControlType { ucStatic, ucEdit, ucCombo, ucButton, ucDefaultButton } controlType;
	GUI::gui_string text;

#ifdef RB_USBTT
	GUI::gui_string tip_text;
#endif

	int item;
	bool fixedWidth = true;
	int widthDesired = widthControl;
	int widthAllocated = widthControl;
	GUI::Window w;
	UserControl(UCControlType controlType_, const GUI::gui_string &text_, int item_) :
		controlType(controlType_),
		text(text_),
		item(item_)	{
	}
};

struct ColumnWidth {
	int widthDesired = 0;
	int widthAllocated = 0;
	bool isResizeable = false;
};

class StripDefinition {
public:
	bool hasClose = false;
	unsigned int columns = 0;
	std::vector<std::vector<UserControl> > controls;
	std::vector<ColumnWidth> widths;

	explicit StripDefinition(const GUI::gui_string &definition) {
		controls.clear();
		controls.push_back(std::vector<UserControl>());
		const GUI::gui_char *pdef=definition.c_str();
		unsigned int column = 0;
		int item = 0;
		while (*pdef) {
			if (*pdef == '\n') {
				controls.push_back(std::vector<UserControl>());
				column = 0;
				pdef++;
				continue;
			}
			if (*pdef == '!') {
				hasClose = true;
				pdef++;
				continue;
			}
			UserControl::UCControlType controlType = UserControl::ucStatic;
			GUI::gui_char endChar = 0;
			switch (*pdef) {
			case '\'':
				controlType = UserControl::ucStatic;
				endChar = '\'';
				break;
			case '[':
				controlType = UserControl::ucEdit;
				endChar = ']';
				break;
			case '{':
				controlType = UserControl::ucCombo;
				endChar = '}';
				break;
			case '(':
				if (pdef[1] == '(') {
					controlType = UserControl::ucDefaultButton;
					pdef++;
				} else {
					controlType = UserControl::ucButton;
				}
				endChar = ')';
				break;
			default:
				break;
			}
			pdef++;
			GUI::gui_string text;
			while (*pdef && (*pdef != endChar)) {
				text += *pdef;
				pdef++;
			}
			if ((controlType == UserControl::ucDefaultButton) && *pdef)
				pdef++;
			controls.back().push_back(UserControl(controlType, text, item));
			column++;
			if (columns < column)
				columns = column;
			if (*pdef)
				pdef++;
			item++;
		}
	}

	void CalculateColumnWidths(int widthToAllocate) {
		widths.clear();

		int widthOfNonResizeables = 4 * (columns - 1);
		int resizeables = 0;
		for (size_t column=0; column<columns; column++) {
			ColumnWidth cw;
			for (size_t line=0; line<controls.size(); line++) {
				if (column <  controls[line].size()) {
					const UserControl *ctl = &controls[line][column];
					if (ctl->fixedWidth) {
						if (cw.widthDesired < ctl->widthDesired)
							cw.widthDesired = ctl->widthDesired;
					} else {
						cw.isResizeable = true;
					}
				}
			}
			widths.push_back(cw);
			widthOfNonResizeables += cw.widthDesired;
			if (cw.isResizeable)
				resizeables++;
		}
		const int widthSpareEach = resizeables ?
					   (widthToAllocate - widthOfNonResizeables) / resizeables : 0;
		for (std::vector<ColumnWidth>::iterator cw=widths.begin(); cw != widths.end(); ++cw) {
			if (cw->isResizeable) {
				cw->widthAllocated = widthSpareEach + cw->widthDesired;
			} else {
				cw->widthAllocated = cw->widthDesired;
			}
		}
	}

	UserControl *FindControl(int control) noexcept {
		int controlID=0;
		for (std::vector<std::vector<UserControl> >::iterator line=controls.begin(); line != controls.end(); ++line) {
			for (std::vector<UserControl>::iterator ctl=line->begin(); ctl != line->end(); ++ctl) {
				if (controlID == control) {
					return &(*ctl);
				}
				controlID++;
			}
		}
		return nullptr;
	}
};

enum class StripCommand { unknown, clicked, change, focusIn, focusOut };

#endif
