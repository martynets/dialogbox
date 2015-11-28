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

#include <QApplication>
#include "dialogbox.hpp"

static char* about_label="_dbabout_";

/*******************************************************************************
 *
 * Constructors, destructor
 *
 * ****************************************************************************/

DialogBox::DialogBox(const char* title, const char* about, FILE* out):
										current_layout(new QVBoxLayout),
										current_index(0),
										group_layout(0),
										output(out),
										empty(true)
{

    QVBoxLayout *mainLayout=new QVBoxLayout;
    QHBoxLayout *hl=new QHBoxLayout;

    setLayout(mainLayout);
    mainLayout->addLayout(hl);
    hl->addLayout(current_layout);

	mainLayout->setSizeConstraint(QLayout::SetFixedSize);
	mainLayout->setAlignment(WIDGETS_ALIGNMENT);
	current_layout->setAlignment(WIDGETS_ALIGNMENT);

    setWindowTitle(title);
    if(about) AddLabel(about, about_label);
}

/*******************************************************************************
 *
 * Widget management methods
 *
 * ****************************************************************************/

void DialogBox::AddPushbutton(const char* title, const char* name, bool apply, bool exit, bool def)
{
	QPushButton* pb=new QPushButton(title);

	pb->setObjectName(QString(name));

	if(group_layout) group_layout->insertWidget(group_index++,pb);
	else current_layout->insertWidget(current_index++,pb);

	connect(pb, SIGNAL(clicked()), this, SLOT(pbclicked()));
    if(apply)
		{
			connect(pb, SIGNAL(clicked()), this, SLOT(report()));
			if(exit) connect(pb, SIGNAL(clicked()), this, SLOT(accept()));
		}
	else if(exit) connect(pb, SIGNAL(clicked()), this, SLOT(reject()));
	pb->setDefault(def);

	update_tab_order();
}

void DialogBox::AddCheckbox(const char* title, const char* name, bool checked)
{
	QCheckBox* cb=new QCheckBox(title);

	cb->setObjectName(QString(name));
	cb->setChecked(checked);

	if(group_layout) group_layout->insertWidget(group_index++,cb);
	else current_layout->insertWidget(current_index++,cb);

	update_tab_order();
}

void DialogBox::AddRadiobutton(const char* title, const char* name, bool checked)
{
	QRadioButton* rb=new QRadioButton(title);

	rb->setObjectName(QString(name));
	rb->setChecked(checked);

	if(group_layout) group_layout->insertWidget(group_index++,rb);
	else current_layout->insertWidget(current_index++,rb);

	update_tab_order();
}

void DialogBox::AddLabel(const char* title, const char* name, enum Content content)
{
	QLabel* lb=new QLabel;

	lb->setObjectName(QString(name));

	if(group_layout) group_layout->insertWidget(group_index++,lb);
	else current_layout->insertWidget(current_index++,lb);

	sanitize_label(lb, content);

	switch(content)
		{
			case pixmap:
				lb->setPixmap(QPixmap(title)); // QLabel copies QPixmap object
				break;
			case movie:
				{
					QMovie* mv=new QMovie(title);

					lb->setMovie(mv); // QLabel stores pointer to QMovie object
					mv->setParent(lb);
					mv->start();
				}
				break;
			default:
				lb->setText(title); // QLabel copies QString object
		}
}

void DialogBox::AddGroupbox(const char* title, const char* name, bool vertical, bool checkable, bool checked)
{
	QGroupBox* gb=new QGroupBox(title);

    gb->setObjectName(QString(name));
    gb->setCheckable(checkable);
    gb->setChecked(checked);

    //gb->setFlat(strlen(title) ? false : true);

    group_layout=(vertical ? (QBoxLayout*)new QVBoxLayout : (QBoxLayout*)new QHBoxLayout);
    group_index=0;
    gb->setLayout(group_layout);
	group_layout->setAlignment(WIDGETS_ALIGNMENT);
	current_layout->insertWidget(current_index++,gb);

	update_tab_order();
}

void DialogBox::AddTextbox(const char* title, const char* name, const char* text, const char* placeholder, bool password)
{
	QHBoxLayout* box=new QHBoxLayout;
	QLabel* label=new QLabel(title);
	QLineEdit* edit=new QLineEdit(text);

	edit->setPlaceholderText(placeholder);
	edit->setEchoMode(password?QLineEdit::Password:QLineEdit::Normal);

	label->setObjectName(name);
	label->setBuddy(edit);
	label->setFocusProxy(edit);

	box->addWidget(label);
	box->addWidget(edit);

	if(group_layout) ((QBoxLayout*)group_layout)->insertLayout(group_index++,box);
	else ((QBoxLayout*)current_layout)->insertLayout(current_index++,box);

	update_tab_order();
}

void DialogBox::AddSeparator(const char* name, bool vertical)
{
	QFrame* separator=new QFrame;

	separator->setObjectName(name);

	if(vertical)
		{
			separator->setFrameShape(QFrame::VLine);
			separator->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::MinimumExpanding);
		}
	else
		{
			separator->setFrameShape(QFrame::HLine);
			separator->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Fixed);
		}
	separator->setFrameShadow(QFrame::Sunken);

	if(group_layout) group_layout->insertWidget(group_index++,separator);
	else current_layout->insertWidget(current_index++,separator);
}

static int indexOf(QLayout* layout)
{
	int i=-1, j;

	if(QLayout* parent=(QLayout*)layout->parent())
		{
			for(i=0, j=parent->count(); i<j; i++)
				if(parent->itemAt(i)->layout()==layout) return(i);
		}
	return(i);
}

static int indexOf(QWidget* widget, QLayout* layout) // QLayout::indexOf(QWidget*) ???
{
	int i=-1, j;

	for(i=0, j=layout->count(); i<j; i++)
		if(layout->itemAt(i)->widget()==widget) return(i);
	return(i);
}

void DialogBox::StepHorizontally()
{
	QBoxLayout* oldlayout=current_layout;
	QVBoxLayout* vbox=new QVBoxLayout;

	((QBoxLayout*)current_layout->parent())->insertLayout(indexOf(current_layout)+1,vbox);
	current_layout=vbox;
	current_index=0;
	current_layout->setAlignment(WIDGETS_ALIGNMENT);
	EndGroupbox();
	sanitize_layout(oldlayout);
}

void DialogBox::StepVertically()
{
	QBoxLayout* oldlayout=current_layout;
	QVBoxLayout* vbox=new QVBoxLayout;
	QHBoxLayout* hbox=new QHBoxLayout;
	QBoxLayout* rootlayout=(QBoxLayout*)current_layout->parent()->parent();

	rootlayout->insertLayout(indexOf((QLayout*)current_layout->parent())+1,hbox);
	hbox->addLayout(vbox);
	current_layout=vbox;
	current_index=0;
	current_layout->setAlignment(WIDGETS_ALIGNMENT);
	EndGroupbox();
	sanitize_layout(oldlayout);
}

void DialogBox::RemoveWidget(char* name)
{
	QWidget* widget;

	if(widget=FindWidget(name))
		{
			if(QLayout* layout=FindLayout(widget))
				{
					if(widget->layout()==group_layout) EndGroupbox();	// for QGroupBox objects
					if(layout==group_layout)
						{
							int index=indexOf(widget, layout);
							if(index<group_index) group_index--;
						}
					if(layout==current_layout)
						{
							int index=indexOf(widget, layout);
							if(index<current_index) current_index--;
						}

					if(QWidget* proxywidget=widget->focusProxy())		// for QLineEdit objects
						{
							layout->removeWidget(proxywidget);
							delete proxywidget;
						}

					layout->removeWidget(widget);
					delete widget;	// this also deletes child layouts and widgets
									// (parented to parentWidget by addWidget and addLayout) and so forth

					sanitize_layout(layout);
				}
		}
}

void DialogBox::Position(char* name, bool behind, bool onto)
{
	QWidget* widget;
	QBoxLayout* layout;

	if((widget=FindWidget(name)) && (layout=(QBoxLayout*)FindLayout(widget)))
		{
			QObject* parent;
			int index;

			if(widget->focusProxy())
				{
					parent=layout->parent()->parent();
					index=indexOf(layout);
					layout=(QBoxLayout*)layout->parent();
				}
			else
				{
					parent=layout->parent();
					index=indexOf(widget, layout);
				}

			if(parent->isWidgetType())	// for widgets installed onto QGroupBox
				{
					group_index=index;
					if(behind) group_index++;

					group_layout=layout;

					if(layout=(QBoxLayout*)FindLayout(widget=(QWidget*)parent))
						{
							current_index=indexOf(widget, layout)+1;
							current_layout=layout;
						}
				}
			else
				{
					EndGroupbox();
					current_index=index;
					if(behind) current_index++;

					current_layout=layout;

					if(onto && (layout=(QBoxLayout*)widget->layout()))	// position onto QGroupBox object
						{
							group_layout=layout;
							group_index=layout->count();
						}
				}
		}
}


/*******************************************************************************
 *
 *	Sets options for the given widget.
 *	options and mask are DialogCommand::Options values.
 *	mask indicates which bits to process when options indicates whether the
 *	option is to set or to unset.
 *	text is the string used for options which require string values.
 *
 * ****************************************************************************/
void DialogBox::SetOptions(QWidget* widget, int options, int mask, char* text)
{
	if(!widget) return;

	const QMetaObject* metaobj=widget->metaObject();
	QMetaProperty property;

	// focus makes sense for set command only
	if(mask & DialogCommand::option_focus && options & DialogCommand::option_focus)
		{
			QTimer::singleShot(0, widget, SLOT(setFocus()));

			// select text for QLineEdit objects
			// selectedText property is not writable and this must be done in the class specific way
			if(QWidget* proxywidget=widget->focusProxy())
				((QLineEdit*)proxywidget)->selectAll();
		}

	// default makes sense for set command only
	if(mask & DialogCommand::option_default && options & DialogCommand::option_default)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("default"))).isWritable())
				property.write(widget, QVariant(true));
		}

	if(mask & DialogCommand::option_checked)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("checked"))).isWritable())
				property.write(widget, QVariant(options & DialogCommand::option_checked));
		}

	if(mask & DialogCommand::option_checkable)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("checkable"))).isWritable())
				property.write(widget, QVariant(options & DialogCommand::option_checkable));
		}

	if(mask & DialogCommand::option_enabled)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("enabled"))).isWritable())
				property.write(widget, QVariant(options & DialogCommand::option_enabled));
			if(QWidget* proxywidget=widget->focusProxy())
				{
					const QMetaObject* proxymetaobj=proxywidget->metaObject();
					if((property=proxymetaobj->property(proxymetaobj->indexOfProperty("enabled"))).isWritable())
						property.write(proxywidget, QVariant(options & DialogCommand::option_enabled));
				}
		}

	// password makes sense for QLineEdit objects only
	if(mask & DialogCommand::option_password)
		{
			if(QWidget* proxywidget=widget->focusProxy())
				{
					const QMetaObject* proxymetaobj=proxywidget->metaObject();
					if((property=proxymetaobj->property(proxymetaobj->indexOfProperty("echoMode"))).isWritable())
						property.write(proxywidget, QVariant(options & DialogCommand::option_password ? QLineEdit::Password : QLineEdit::Normal));
				}
		}

	// text makes difference for QLineEdit objects only
	// for the rest it is the same as title
	if(mask & DialogCommand::option_text)
		{
			if(QWidget* proxywidget=widget->focusProxy())
				{
					const QMetaObject* proxymetaobj=proxywidget->metaObject();
					if((property=proxymetaobj->property(proxymetaobj->indexOfProperty("text"))).isWritable())
						property.write(proxywidget, QVariant(QString(options & DialogCommand::option_text ? text : NULL)));
				}
			else
				{
					// these options will be applied by code below
					mask|=DialogCommand::option_title;
					options|=options & DialogCommand::option_text ? DialogCommand::option_title : 0;
				}
		}

	// placeholder makes sense for QLineEdit objects only
	if(mask & DialogCommand::option_placeholder)
		{
			if(QWidget* proxywidget=widget->focusProxy())
				{
					const QMetaObject* proxymetaobj=proxywidget->metaObject();
					if((property=proxymetaobj->property(proxymetaobj->indexOfProperty("placeholderText"))).isWritable())
						property.write(proxywidget, QVariant(QString(options & DialogCommand::option_placeholder ? text : NULL)));
				}
		}

	// animation makes sense for QLabel objects only
	// also avoid to set picture for QLineEdit labels (which have focusProxy set)
	if(mask & DialogCommand::option_animation)
		{
			// there is no movie property for QLabel objects
			if(!widget->focusProxy() && !strcmp(metaobj->className(), "QLabel"))
				{
					sanitize_label(widget, DialogBox::movie);
					if(QMovie* mv=new QMovie(options & DialogCommand::option_animation ? text : NULL))
						{
							((QLabel*)widget)->setMovie(mv);
							mv->start();
							mv->setParent(widget);
						}
				}
		}

	// picture makes sense for QLabel objects only
	// also avoid to set picture for QLineEdit labels (which have focusProxy set)
	if(mask & DialogCommand::option_picture)
		{
			if(!widget->focusProxy() && (property=metaobj->property(metaobj->indexOfProperty("pixmap"))).isWritable())
				{
					sanitize_label(widget, DialogBox::pixmap);
					property.write(widget, QVariant(QPixmap(options & DialogCommand::option_picture ? text : NULL)));
				}
		}

	if(mask & DialogCommand::option_title)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("title"))).isWritable() ||	// for QGroupBox objects
				(property=metaobj->property(metaobj->indexOfProperty("text"))).isWritable() ||	// for the rest widgets
				(property=metaobj->property(metaobj->indexOfProperty("windowTitle"))).isWritable())	// for the main window (QDialog object)
					{
						// avoid to format QLineEdit labels (which have focusProxy set)
						if(!widget->focusProxy()) sanitize_label(widget, DialogBox::text);
						property.write(widget, QVariant(QString(options & DialogCommand::option_title ? text : NULL)));

					}
		}

	if(mask & DialogCommand::option_icon)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("icon"))).isWritable() ||
				(property=metaobj->property(metaobj->indexOfProperty("windowIcon"))).isWritable())
					property.write(widget, QVariant(QIcon(options & DialogCommand::option_icon ? text : NULL)));
		}

	// apply and exit make sense for QPushButton objects only
	if(mask & DialogCommand::option_apply || mask & DialogCommand::option_exit)
		{
			if(!strcmp(metaobj->className(), "QPushButton"))
				{
					int pboptions=0;
					int loptions=options & (DialogCommand::option_apply | DialogCommand::option_exit);
					int lmask=mask & (DialogCommand::option_apply | DialogCommand::option_exit);

					if(disconnect(widget, SIGNAL(clicked()), this, SLOT(report())))
						{
							pboptions|=DialogCommand::option_apply;
							if(disconnect(widget, SIGNAL(clicked()), this, SLOT(accept())))
								pboptions|=DialogCommand::option_exit;
						}

					if(disconnect(widget, SIGNAL(clicked()), this, SLOT(reject())))
						pboptions|=DialogCommand::option_exit;

					pboptions &= (~lmask) | loptions;	// reset 1s where needed
					pboptions |= lmask & loptions;		// set 1s where needed

					if(pboptions & DialogCommand::option_apply)
						{
							connect(widget, SIGNAL(clicked()), this, SLOT(report()));
							if(pboptions & DialogCommand::option_exit)
								connect(widget, SIGNAL(clicked()), this, SLOT(accept()));
						}
					else if(pboptions & DialogCommand::option_exit)
							connect(widget, SIGNAL(clicked()), this, SLOT(reject()));
				}
		}

	// iconsize makes sense for set command only
	if(mask & DialogCommand::option_iconsize && options & DialogCommand::option_iconsize)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("iconSize"))).isWritable())
				{
					int size;
					sscanf(text, "%d", &size);
					property.write(widget, QVariant(QSize(size, size)));
				}
		}

	// see http://doc.qt.io/qt-4.8/stylesheet.html for reference
	if(mask & DialogCommand::option_stylesheet)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("styleSheet"))).isWritable())
				property.write(widget, QVariant(QString(options & DialogCommand::option_stylesheet ? text : NULL)));
		}
}

/*******************************************************************************
 *
 *	Slot function. Reports the pushbutton is clicked.
 *
 * ****************************************************************************/
void DialogBox::pbclicked()
{
	QPushButton* pb=(QPushButton*)sender();
	const char* objectname=pb->objectName().toLocal8Bit().constData();

	if(strlen(objectname)>0)
		{
			fprintf(output, "%s=clicked\n", objectname);
			fflush(output);
		}
}

/*******************************************************************************
 *
 *	Slot function. Translates command object recevied from the parser thread
 * 	to appropriate function call.
 *
 * ****************************************************************************/
void DialogBox::ExecuteCommand(DialogCommand command)
{
	if(empty)
		{
			RemoveWidget(about_label);
			empty=false;
		}
	switch(command.widgetcommand)
		{
			case DialogCommand::add:
				switch(command.widgettype)
					{
						case DialogCommand::label:
							AddLabel(command.GetTitle(), command.GetName(),
								command.widgetoptions & DialogCommand::option_picture ? DialogBox::pixmap :
										command.widgetoptions & DialogCommand::option_animation ? DialogBox::movie :
										DialogBox::text);
							break;
						case DialogCommand::groupbox:
							AddGroupbox(command.GetTitle(), command.GetName(),
								command.widgetoptions & DialogCommand::option_vertical,
								command.widgetoptions & DialogCommand::option_checkable,
								command.widgetoptions & DialogCommand::option_checked);
							break;
						case DialogCommand::pushbutton:
							AddPushbutton(command.GetTitle(), command.GetName(),
								command.widgetoptions & DialogCommand::option_apply,
								command.widgetoptions & DialogCommand::option_exit,
								command.widgetoptions & DialogCommand::option_default);
							break;
						case DialogCommand::checkbox:
							AddCheckbox(command.GetTitle(), command.GetName(),
								command.widgetoptions & DialogCommand::option_checked);
							break;
						case DialogCommand::radiobutton:
							AddRadiobutton(command.GetTitle(), command.GetName(),
								command.widgetoptions & DialogCommand::option_checked);
							break;
						case DialogCommand::textbox:
							AddTextbox(command.GetTitle(), command.GetName(),
								command.GetText(),
								command.GetAuxText(),
								command.widgetoptions & DialogCommand::option_password);
							break;
						case DialogCommand::stretch:
							AddStretch();
							break;
						case DialogCommand::space:
							 // Seems sscanf %d in some versions of standard C library has a bug
							 // returning 32k on sero-size strings
							if(command.GetTitle()[0])
								{
									int size;
									sscanf(command.GetTitle(), "%d", &size);
									AddSpace(size);
								}
							else AddSpace();
							break;
						case DialogCommand::separator:
							AddSeparator(command.GetTitle(),
								command.widgetoptions & DialogCommand::option_vertical);
							break;
					}
				break;
			case DialogCommand::end:
				switch(command.widgettype)
					{
						case DialogCommand::groupbox:
							EndGroupbox();
							break;
					}
				break;
			case DialogCommand::step:
				switch(command.widgettype)
					{
						case DialogCommand::none:
							if(command.widgetoptions & DialogCommand::option_vertical) StepVertically();
							else StepHorizontally();
							break;
					}
				break;
			case DialogCommand::set:
				if(command.GetName()[0])
					{
						QWidget* widget;

						if(widget=FindWidget(command.GetName()))
							SetOptions(widget, command.widgetoptions, command.widgetoptions, command.GetText());
					}
				else SetOptions(this, command.widgetoptions, command.widgetoptions, command.GetText());
				break;
			case DialogCommand::unset:
				if(command.GetName()[0])
					{
						QWidget* widget;

						if(widget=FindWidget(command.GetName()))
							SetOptions(widget, 0, command.widgetoptions, command.GetText());
					}
				else SetOptions(this, 0, command.widgetoptions, command.GetText());
				break;
			case DialogCommand::enable:
				if(command.GetName()[0])
					{
						QWidget* widget;

						if(widget=FindWidget(command.GetName()))
							SetOptions(widget, DialogCommand::option_enabled, DialogCommand::option_enabled, NULL);
					}
				else SetOptions(this, DialogCommand::option_enabled, DialogCommand::option_enabled, NULL);
				break;
			case DialogCommand::disable:
				if(command.GetName()[0])
					{
						QWidget* widget;

						if(widget=FindWidget(command.GetName()))
							SetOptions(widget, 0, DialogCommand::option_enabled, NULL);
					}
				else SetOptions(this, 0, DialogCommand::option_enabled, NULL);
				break;
			case DialogCommand::remove:
				RemoveWidget(command.GetName());
				break;
			case DialogCommand::position:
				Position(command.GetText(),
							command.widgetoptions & DialogCommand::option_behind,
							command.widgetoptions & DialogCommand::option_onto);
				break;
			case DialogCommand::print:
				print_structure_recursively();
				break;
			case DialogCommand::noop:
			default:
				;
		}
}




/*******************************************************************************
 *
 *	PRIVATE FUNCTIONS
 *
 * ****************************************************************************/

/*******************************************************************************
 *	update_tab_order sets widgets focus order in way they are shown on the
 * 	dialog, not they are created.
 * ****************************************************************************/
void DialogBox::update_tab_order()
{
	QBoxLayout* main_layout=(QBoxLayout*)layout();
	QWidget* prev_widget=0;
	QWidget* widget;

	// All widgets are chained. Their focus policy defines the focus move.
	for(int i=0, j=main_layout->count(); i<j; i++)
		{
			QHBoxLayout* hlayout=(QHBoxLayout*)main_layout->itemAt(i)->layout();
			for(int i=0, j=hlayout->count(); i<j; i++)
				{
					QVBoxLayout* vlayout=(QVBoxLayout*)hlayout->itemAt(i)->layout();
					for(int i=0, j=vlayout->count(); i<j; i++)
						{
							QLayout* layout;
							QLayoutItem* li=vlayout->itemAt(i);

							if(widget=li->widget());
							else if(layout=li->layout()) widget=layout->itemAt(1)->widget(); // for QLineEdit objects

							if(widget)
								{
									if(prev_widget) setTabOrder(prev_widget, widget);
									prev_widget=widget;

									if(QBoxLayout* glayout=(QBoxLayout*)widget->layout())	// for QGroupBox objects
										for(int i=0, j=glayout->count(); i<j; i++)
											{
												QLayout* layout;
												QLayoutItem* li=glayout->itemAt(i);

												if(widget=li->widget());
												else if(layout=li->layout()) widget=layout->itemAt(1)->widget();

												if(widget)
													{
														if(prev_widget) setTabOrder(prev_widget, widget);
														prev_widget=widget;
													}
											}
								}
						}
				}
		}
}

/*******************************************************************************
 *	The layout is empty if it is not the current one, has no child widgets and
 * 	the same is true for all its downlinks.
 * ****************************************************************************/
bool DialogBox::is_empty(QLayout* layout)
{
	// layout->isEmpty() returns true even if it contains labels or groupboxes

	if(!layout || layout==current_layout) return(false);
	for(int i=0, j=layout->count(); i<j; i++)
		{
			QLayoutItem* li;
			QLayout* child;
			if((li=layout->itemAt(i))->widget()) return(false);
			if((child=li->layout()) && !is_empty(child)) return(false);
		}
	return(true);
}

/*******************************************************************************
 *	remove_if_empty removes branch of empty layouts from the tree.
 * 	It removes layout (with its downlinks) if it:
 * 		- has no widgets downstream
 * 		- is not the current one (this also prevents removing the last one)
 * 		- is not installed on a widget (groupbox in particular)
 * ****************************************************************************/
bool DialogBox::remove_if_empty(QLayout* layout)
{
	QLayout* parent;

	if(is_empty(layout) && (!(parent=(QLayout*)layout->parent())->isWidgetType()))
		{
			if(!remove_if_empty(parent))
				{
					parent->removeItem(layout);
					delete layout;
				}
			return(true);
		}
	return(false);
}

/*******************************************************************************
 *	sanitize_layout prevents fantom layouts. Must be called for end layouts ONLY.
 * ****************************************************************************/
void DialogBox::sanitize_layout(QLayout* layout)
{
	if(is_empty(layout) && layout->count())	// remove all QSpacerItem items
		while(QLayoutItem* li=layout->takeAt(0))
			delete li;

	remove_if_empty(layout);
}

/*******************************************************************************
 *	sanitize_label prepares label for changing its content type
 * ****************************************************************************/
void DialogBox::sanitize_label(QWidget* label, enum Content content)
{
	if(!strcmp(label->metaObject()->className(), "QLabel"))
		{
			QMovie* mv;
			QLayout* layout;

			if(mv=((QLabel*)label)->movie()) delete mv;

			if(layout=FindLayout(label))
				switch(content)
					{
						case pixmap:
							layout->setAlignment(GRAPHICS_ALIGNMENT);
							//reset to defaults
							((QLabel*)label)->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
							((QLabel*)label)->setOpenExternalLinks(false);
							((QLabel*)label)->setWordWrap(false);
							break;
						case movie:
							layout->setAlignment(GRAPHICS_ALIGNMENT);
							//reset to defaults
							((QLabel*)label)->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
							((QLabel*)label)->setOpenExternalLinks(false);
							((QLabel*)label)->setWordWrap(false);
							break;
						case text:
							layout->setAlignment(TEXT_ALIGNMENT);
							// Qt::TextBrowserInteraction includes:
							//		TextSelectableByMouse | LinksAccessibleByMouse | LinksAccessibleByKeyboard
							// to make the text label not interactive use stylesheets
							// (e.g. qproperty-textInteractionFlags: NoTextInteraction;)
							((QLabel*)label)->setTextInteractionFlags(Qt::TextBrowserInteraction);
							((QLabel*)label)->setOpenExternalLinks(true);
							((QLabel*)label)->setWordWrap(true);
							label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
							break;
					}
		}
}

QWidget* DialogBox::find_widget_recursively(QLayoutItem* item, char* name)
{
	QLayout* layout;
	QWidget* widget;

    if((layout=item->layout()))
		{
			for(int i=0, j=layout->count(); i<j; i++)
				if(widget=find_widget_recursively(layout->itemAt(i), name)) return(widget);
		}
    else
		if((widget=item->widget()))
			{
				if(!strcmp(widget->objectName().toLocal8Bit().constData(),name)) return(widget);
				if((layout=widget->layout())) return(find_widget_recursively(layout, name));
			}
	return(NULL);
}

QLayout* DialogBox::find_layout_recursively(QLayout* layout, QWidget* widget)
{

    if(widget && layout)
		{
			for(int i=0, j=layout->count(); i<j; i++)
				{
					QLayoutItem* item;
					QWidget* w;
					QLayout* retlayout;

					if((w=(item=layout->itemAt(i))->widget()) == widget) return(layout);
					else  if(retlayout=find_layout_recursively(w ? w->layout() : item->layout(), widget)) return(retlayout);
				}
		}
	return(NULL);
}

void DialogBox::print_widgets_recursively(QLayoutItem* item)
{
    QLayout* layout;
	QWidget* widget;

    if((layout=item->layout()))
		for(int i=0, j=layout->count(); i<j; i++) print_widgets_recursively(layout->itemAt(i));
    else
		if((widget=item->widget()))
			{
				print_widget(widget);
				if((layout=widget->layout())) print_widgets_recursively(layout);
			}
}

void DialogBox::print_widget(QWidget* object)
{
	if(object->isEnabled())
		{
			const char* classname;
			const char* objectname=object->objectName().toLocal8Bit().constData();
			const char* objectvalue=0;
			int len=strlen(objectname);
			char* tmp;

			if(len>0 && (tmp=new char[len+1]))
				{
					QWidget* widget;
					if(!(widget=object->focusProxy())) widget=object;
					strcpy(tmp,objectname);
					objectname=tmp;
					classname=widget->metaObject()->className();
					if(!strcmp(classname,"QCheckBox")) objectvalue=((QCheckBox*)widget)->isChecked()?"1":"0";
					else if(!strcmp(classname,"QRadioButton")) objectvalue=((QRadioButton*)widget)->isChecked()?"1":"0";
					else if(!strcmp(classname,"QGroupBox")) objectvalue=((QGroupBox*)widget)->isChecked()?"1":"0";
					else if(!strcmp(classname,"QLineEdit")) objectvalue=((QLineEdit*)widget)->text().toLocal8Bit().constData();

					if(objectvalue)
						{
							fprintf(output, "%s=%s\n", objectname, objectvalue);
							fflush(output);
						}
					delete tmp;
				}
		}
}

void DialogBox::print_structure_recursively(QLayoutItem* item)
{
	static int level;
	const char* name;
    QLayout* layout;
	QWidget* widget;
	QSpacerItem* space;

	if(!item)
		{
			item=this->layout();
			level=0;
		}
	else level++;

	//print level spaces plus either objectName or className
    if((layout=item->layout()))
		{
			name=layout->objectName().toLocal8Bit().constData();
			if(!name[0]) name=layout->metaObject()->className();
			fprintf(stderr, "%*.*s%s <%s>\n", level, level, "", name, is_empty(layout)?"empty":"not empty");
			fflush(stderr);

			for(int i=0, j=layout->count(); i<j; i++) print_structure_recursively(layout->itemAt(i));
		}
    else
		if((widget=item->widget()))
			{
				name=widget->objectName().toLocal8Bit().constData();
				if(!name[0]) name=widget->metaObject()->className();
				fprintf(stderr, "%*.*s%s\n", level, level, "", name);
				fflush(stderr);

				if((layout=widget->layout())) print_structure_recursively(layout);
			}
		else
			if((space=item->spacerItem()))
				{
					fprintf(stderr, "%*.*s%s\n", level, level, "", "spacer");
					fflush(stderr);
				}
	level--;
}
