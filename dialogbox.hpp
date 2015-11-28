/*
 * GUI widgets for shell scripts - dialogbox version 0.8
 *
 * Copyright (C) 2015 Andriy Martynets <martynets@volia.ua>
 *--------------------------------------------------------------------------------------------------------------
 * This file is part of dialogbox.
 *
 * Dialogbox is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * Dialogbox is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with dialogbox.
 * If not, see http://www.gnu.org/licenses/.
 *--------------------------------------------------------------------------------------------------------------
 */

#ifndef DIALOGBOX_H
#define DIALOGBOX_H

#include <QtGui>

#define BUFFER_SIZE 1024

#define GRAPHICS_ALIGNMENT	Qt::AlignCenter
#define TEXT_ALIGNMENT		Qt::AlignTop | Qt::AlignLeft
#define WIDGETS_ALIGNMENT	TEXT_ALIGNMENT

struct DialogCommand
{
	char buffer[BUFFER_SIZE];
	size_t widgettitle;
	size_t widgetname;
	size_t widgettext;
	size_t widgetauxtext;

	enum Commands { noop, add, end, step, set, unset, enable, disable, remove, position, print };
	int widgetcommand;

	enum Widgets { none, label, groupbox, pushbutton, radiobutton, checkbox, textbox, stretch, space, separator };
	int widgettype;

	enum Options
		{
			option_reset=		0x00000000,
			option_checked=		0x00000001,
			option_checkable=	0x00000002,
			option_vertical=	0x00000004,
			option_password=	0x00000008,
			option_picture=		0x00000010,
			option_animation=	0x00000020,
			option_apply=		0x00000040,
			option_exit=		0x00000080,
			option_default=		0x00000100,
			option_focus=		0x00000200,
			option_title=		0x00000400,
			option_icon=		0x00000800,
			option_text=		0x00001000,
			option_placeholder=	0x00002000,
			option_enabled=		0x00004000,
			option_behind=		0x00008000,
			option_onto=		0x00010000,
			option_iconsize=	0x00020000,
			option_stylesheet=	0x00040000
		};
	int widgetoptions;

	enum Stages {
			stage_command=		0x00000001,
			stage_type=			0x00000002,
			stage_title=		0x00000004,
			stage_name=			0x00000008,
			stage_text=			0x00000010,
			stage_aux_text=		0x00000020,
			stage_options=		0x00000040
		};
	int widgetstage;

	inline DialogCommand()
		{
			buffer[BUFFER_SIZE-1]=0;
			widgettitle=widgetname=widgettext=widgetauxtext=BUFFER_SIZE-1;
		}

	inline char* GetTitle()
		{
			return( buffer+widgettitle );
		}

	inline char* GetName()
		{
			return( buffer+widgetname );
		}

	inline char* GetText()
		{
			return( buffer+widgettext );
		}

	inline char* GetAuxText()
		{
			return( buffer+widgetauxtext );
		}
};

class DialogBox : public QDialog
{
    Q_OBJECT

public:
    DialogBox(const char* title, const char* about=0, FILE* out=stdout);

	enum Content { text, pixmap, movie };

    void AddPushbutton(const char* title, const char* name, bool apply=false, bool exit=false, bool def=false);
    void AddCheckbox(const char* title, const char* name, bool checked=false);
    void AddRadiobutton(const char* title, const char* name, bool checked=false);
    void AddTextbox(const char* title, const char* name, const char* text=0, const char* placeholder=0, bool password=false);
	void AddLabel(const char* title, const char* name=0, enum Content content=text);
    void AddGroupbox(const char* title, const char* name, bool vertical=true, bool checkable=false, bool checked=false);
    inline void EndGroupbox()
		{
			group_layout=0;
		}

    inline void AddStretch(int stretch=1)
		{
			if(group_layout) group_layout->insertStretch(group_index++,stretch);
			else current_layout->insertStretch(current_index++,stretch);
		}

    inline void AddSpace(int space=1)
		{
			if(group_layout) group_layout->insertSpacing(group_index++,space);
			else current_layout->insertSpacing(current_index++,space);
		}

	void AddSeparator(const char* name=0, bool vertical=false);

    void StepHorizontally();
    void StepVertically();

    void RemoveWidget(char* name);
    void Position(char* name, bool behind=false, bool onto=false);

    void SetOptions(QWidget* widget, int options, int mask, char* text);

public slots:
	void ExecuteCommand(DialogCommand);

private slots:
	void report() { print_widgets_recursively(layout()); }
	void pbclicked();

private:
	QBoxLayout* current_layout;
	int current_index;
	QBoxLayout* group_layout;
	int group_index;

	FILE* output;

	bool empty;

	void update_tab_order();
	void sanitize_label(QWidget* label, enum Content content);

	bool remove_if_empty(QLayout* layout);
	bool is_empty(QLayout* layout);
	void sanitize_layout(QLayout* layout);

	void print_widgets_recursively(QLayoutItem*);
	void print_widget(QWidget*);
	QWidget* find_widget_recursively(QLayoutItem* item, char* name);
	QLayout* find_layout_recursively(QLayout* layout, QWidget* widget);
	void print_structure_recursively(QLayoutItem* item=0);

public:
	inline QWidget* FindWidget(char* name) { return(find_widget_recursively(layout(), name)); }
	inline QLayout* FindLayout(QWidget* widget) { return(find_layout_recursively(layout(), widget)); }

};

class DialogParser : public QThread, private DialogCommand
{

    Q_OBJECT

public:
	DialogParser(DialogBox* parent=0, FILE* in=stdin);
	~DialogParser();

	void setParent(DialogBox* parent);
	void run();

signals:
	void SendCommand(DialogCommand);

private:
	DialogBox* dialog;
	FILE* input;

	size_t token;
	size_t buffer_index;

	void process_token();
	void issue_command();

};

#endif
