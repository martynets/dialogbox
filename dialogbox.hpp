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
			remove=				0x00002000,
			clear=				0x00004000,
			position=			0x00008000,
			query=				0x00010000,
			print=				0x80000000,

			// masks
			option_mask=		0x0000000F, // property bits (4 properties max.)
			command_mask=		0xFFFFFFF0,	// command bits (28 commands max.)

			// properties available for widgets of any type
			option_enabled=		set | unset | 0x00000001,
			option_focus=		set | 0x00000002,
			option_stylesheet=	set | unset | 0x00000004,
			option_visible=		set | unset | 0x00000008,

			// options specific to some commands (say, sub-commands)
			option_vertical=	step | 0x00000001,
			option_behind=		position | 0x00000001,
			option_onto=		position | 0x00000002,
			option_space=		add | 0x00000001,
			option_stretch=		add | 0x00000002
		};
	unsigned int command;

	enum Controls
		{
			none=					0x00000000,

			// widgets
			dialog=					0x80000000,	// main window
			frame=					0x40000000,
			separator=				0x20000000,
			label=					0x10000000,
			groupbox=				0x08000000,
			pushbutton=				0x04000000,
			radiobutton=			0x02000000,
			checkbox=				0x01000000,
			textbox=				0x00800000,
			listbox=				0x00400000,
			combobox=				0x00200000,
			item=					0x00100000,
			progressbar=			0x00080000,
			slider=					0x00040000,
			textview=				0x00020000,

			// masks
			property_mask=			0x000001FF, // property bits (9 properties max.)
			widget_mask=			0xFFFFFE00,	// all widgets listed above (23 types max.)
			no_caption_widget_mask=	widget_mask ^ frame ^ separator ^ progressbar ^ slider ^ textview,

			// properties specific for particular widget types
			property_title=			no_caption_widget_mask | 0x00000001,
			property_text=			no_caption_widget_mask | 0x00000002,
			property_icon=			dialog | item | pushbutton | radiobutton | checkbox | 0x00000004,
			property_checked=		groupbox | pushbutton | radiobutton | checkbox | 0x00000008,
			property_checkable=		groupbox | pushbutton | radiobutton | checkbox | 0x00000010,
			property_iconsize=		listbox | combobox | pushbutton | radiobutton | checkbox | 0x00000020,
			property_vertical=		groupbox | frame | separator | progressbar | slider | 0x00000080,
			property_apply=			pushbutton | 0x00000040,
			property_exit=			pushbutton | 0x00000080,
			property_default=		pushbutton | 0x00000100,	// !!! pushbutton exceeds 8 bits
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
			property_current=		item | 0x00000008,
			property_editable=		combobox | 0x00000004,
			property_selection=		combobox | listbox | 0x00000008,
			property_activation=	listbox | 0x00000004,
			property_minimum=		progressbar | slider | 0x00000001,
			property_maximum=		progressbar | slider | 0x00000002,
			property_value=			progressbar | slider | 0x00000004,
			property_busy=			progressbar | 0x00000008,
			property_file=			textview | 0x00000004,
		};
	unsigned int control;

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


void set_enabled(QWidget* widget, bool enable);
QWidget* find_widget_recursively(QLayoutItem* item, const char* name);
QLayout* find_layout_recursively(QLayout* layout, QWidget* widget);


class DialogBox : public QDialog
{
    Q_OBJECT

public:
    DialogBox(const char* title, const char* about=NULL, bool resizable=false, FILE* out=stdout);

    void ClearDialog();

	enum Content { text, pixmap, movie };

    void AddPushbutton(const char* title, const char* name, bool apply=false, bool exit=false, bool def=false);
    void AddCheckbox(const char* title, const char* name, bool checked=false);
    void AddRadiobutton(const char* title, const char* name, bool checked=false);
    void AddTextbox(const char* title, const char* name, const char* text=NULL, const char* placeholder=NULL, bool password=false);
	void AddLabel(const char* title, const char* name=NULL, enum Content content=text);

    void AddGroupbox(const char* title, const char* name, bool vertical=true, bool checkable=false, bool checked=false);
    void AddFrame(const char* name, bool vertical=true, unsigned int style=0);
    inline void EndGroup() { group_layout=0; }

    void AddListbox(const char* title, const char* name, bool activation=false, bool selection=false);
	void AddCombobox(const char* title, const char* name, bool editable=false, bool selection=false);
	void ClearList(char* name);
	void AddItem(const char* title, const char* icon=NULL, bool current=false);
    inline void EndList() { current_view=0; }

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

	void AddSeparator(const char* name=NULL, bool vertical=false, unsigned int style=0);

	void AddProgressbar(const char* name, bool vertical=false, bool busy=false);
	void AddSlider(const char* name, bool vertical=false, int min=0, int max=100);
	void AddTextview(const char* name, const char* file=NULL);

    void StepHorizontal();
    void StepVertical();

    void RemoveWidget(char* name);
    void Position(char* name, bool behind=false, bool onto=false);

    void SetOptions(QWidget* widget, unsigned int options, unsigned int mask, const char* text);

	void HideDefault();
	void ShowDefault();

public slots:
	void ExecuteCommand(DialogCommand);
	void done(int);

private slots:
	void report() { print_widgets_recursively(layout()); }
	void PushbuttonClicked();
	void PushbuttonToggled(bool);
	void ListboxItemActivated(const QModelIndex&);
	void ListboxItemSelected(QListWidgetItem*);
	void ComboboxItemSelected(int);
	void SliderValueChanged(int);
	void SliderRangeChanged(int, int);

private:
	QPushButton* default_pb;

	QBoxLayout* current_layout;
	int current_index;

	QBoxLayout* group_layout;
	int group_index;

	QAbstractItemView* current_view;	// the list items are added to
	int view_index;
	QWidget* current_list_widget;

	QAbstractItemView* chosen_view;		// the list modifications are made on (set by the FindWidget)
	int chosen_row;						// the list item modifications are made on
	bool chosen_row_flag;				// flag that indicates the list item was set by the FindWidget
	QWidget* chosen_list_widget;

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
	QWidget* FindWidget(char* name);
	inline QLayout* FindLayout(QWidget* widget) { return(find_layout_recursively(layout(), widget)); }

	DialogCommand::Controls WidgetType(QWidget*);

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

	size_t token;
	size_t buffer_index;

	void process_token();
	void issue_command();

};


/*******************************************************************************
 *	Below class is the work around QListWidget limitation:
 * 		the widget reports current item as "activated" (emits the signal) on the
 * 		Enter key press event but propagates this event to next widgets.
 * 		The default/autodefault pushbutton might respond to it even by closing
 * 		the dialog.
 * ****************************************************************************/
class Listbox : public QListWidget
{

    Q_OBJECT

private:
	bool activate_flag;

public:
	Listbox(): activate_flag(false) {};

	void setActivateFlag(bool flag);
	inline bool activateFlag() { return(activate_flag); };
	void focusInEvent(QFocusEvent *event);
	void focusOutEvent(QFocusEvent *event);
};

#endif
