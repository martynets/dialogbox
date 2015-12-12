/*
 * GUI widgets for shell scripts - dialogbox version 0.9
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

#define DEFAULT_ALIGNMENT	Qt::Alignment(0)
#define GRAPHICS_ALIGNMENT	Qt::AlignCenter
#define TEXT_ALIGNMENT		Qt::AlignTop | Qt::AlignLeft
#define WIDGETS_ALIGNMENT	TEXT_ALIGNMENT
#define LAYOUTS_ALIGNMENT	WIDGETS_ALIGNMENT

struct DialogCommand
{
	char buffer[BUFFER_SIZE];
	size_t title;
	size_t name;
	size_t text;
	size_t auxtext;

	enum Commands
		{
			// commands
			noop=				0x00000000,
			add=				0x00000100,
			end=				0x00000200,
			step=				0x00000400,
			set=				0x00000800,
			unset=				0x00001000,
			enable=				0x00002000,
			disable=			0x00004000,
			remove=				0x00008000,
			position=			0x00010000,
			print=				0x00020000,

			// masks
			option_mask=		0x0000000F, // property bits (4 properties max.)
			command_mask=		0xFFFFFFF0,	// command bits (28 commands max.)

			// properties available for widgets of any type
			option_enabled=		set | unset | 0x00000001,
			option_focus=		set | 0x00000002,
			option_stylesheet=	set | unset | 0x00000004,

			// options specific to some commands (say, sub-commands)
			option_vertical=	step | 0x00000001,
			option_behind=		position | 0x00000001,
			option_onto=		position | 0x00000002
		};
	unsigned int command;

	enum Controls
		{
			none=					0x00000000,
			space=					0x80000000, // spacer item
			stretch=				0x40000000, // spacer item

			// widgets
			dialog=					0x20000000,	// main window
			frame=					0x10000000,
			separator=				0x08000000,
			label=					0x04000000,
			groupbox=				0x02000000,
			pushbutton=				0x01000000,
			radiobutton=			0x00800000,
			checkbox=				0x00400000,
			textbox=				0x00200000,

			// masks
			property_mask=			0x00000FFF, // property bits (12 properties max.)
			widget_mask=			0x3FFFF000,	// all widgets listed above (18 types max.)
			infwidget_mask=			widget_mask ^ dialog ^ frame ^ separator,

			// properties specific for particular widget types
			property_title=			dialog | infwidget_mask | 0x00000001,
			property_text=			infwidget_mask | 0x00000002,
			property_icon=			dialog | pushbutton | radiobutton | checkbox | 0x00000004,
			property_checked=		groupbox | pushbutton | radiobutton | checkbox | 0x00000008,
			property_checkable=		groupbox | pushbutton | radiobutton | checkbox | 0x00000010,
			property_iconsize=		pushbutton | radiobutton | checkbox | 0x00000020,
			property_vertical=		groupbox | frame | separator | 0x00000080,
			property_apply=			pushbutton | 0x00000040,
			property_exit=			pushbutton | 0x00000080,
			property_default=		pushbutton | 0x00000100,	// !!! pushbutton exceeds 8 bit
			property_password=		textbox | 0x00000004,
			property_placeholder=	textbox | 0x00000008,
			property_picture=		label | 0x00000004,
			property_animation=		label | 0x00000008,
			property_plain=			frame | separator | 0x00000010,
			property_raised=		frame | separator | 0x00000020,
			property_sunken=		frame | separator | 0x00000040,
			property_noframe=		frame | 0x00000001,
			property_box=			frame | 0x00000002,
			property_panel=			frame | 0x00000004,
			property_styled=		frame | 0x00000008,
		};
	unsigned int control;

	enum Stages
		{
			stage_command=		0x00000001,
			stage_type=			0x00000002,
			stage_title=		0x00000004,
			stage_name=			0x00000008,
			stage_text=			0x00000010,
			stage_aux_text=		0x00000020,
			stage_options=		0x00000040
		};
	unsigned int stage;

	inline DialogCommand()
		{
			buffer[BUFFER_SIZE-1]=0;
			title=name=text=auxtext=BUFFER_SIZE-1;
		}

	inline char* GetTitle()
		{
			return( buffer+title );
		}

	inline char* GetName()
		{
			return( buffer+name );
		}

	inline char* GetText()
		{
			return( buffer+text );
		}

	inline char* GetAuxText()
		{
			return( buffer+auxtext );
		}
};


// The function below returns type of the given widget as coded by the above enum
DialogCommand::Controls WidgetType(QWidget*);

void set_enabled(QWidget* widget, bool enable);
QWidget* find_widget_recursively(QLayoutItem* item, const char* name);
QLayout* find_layout_recursively(QLayout* layout, QWidget* widget);


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
    void AddFrame(const char* name, bool vertical=true, unsigned int style=0);
    inline void EndGroup()
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

	void AddSeparator(const char* name=0, bool vertical=false, unsigned int style=0);

    void StepHorizontal();
    void StepVertical();

    void RemoveWidget(const char* name);
    void Position(const char* name, bool behind=false, bool onto=false);

    void SetOptions(QWidget* widget, unsigned int options, unsigned int mask, const char* text);

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
	void print_structure_recursively(QLayoutItem* item=0);

public:
	inline QWidget* FindWidget(const char* name) { return(find_widget_recursively(layout(), name)); }
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
