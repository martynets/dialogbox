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

#include <QApplication>
#include "dialogbox.hpp"

static const char* about_label="_dbabout_";

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
	mainLayout->setAlignment(LAYOUTS_ALIGNMENT);
	current_layout->setAlignment(LAYOUTS_ALIGNMENT);

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

    group_layout=(vertical ? (QBoxLayout*)new QVBoxLayout : (QBoxLayout*)new QHBoxLayout);
    group_index=0;
    gb->setLayout(group_layout);
	group_layout->setAlignment(LAYOUTS_ALIGNMENT);
	current_layout->insertWidget(current_index++,gb);

	update_tab_order();
}

void DialogBox::AddFrame(const char* name, bool vertical, unsigned int style)
{
	unsigned int shape, shadow;
	QFrame* frm=new QFrame;

    frm->setObjectName(QString(name));

	// style is a DialogCommand::Controls value
	if(style & DialogCommand::frame)
		{
			style&=DialogCommand::property_mask;
			shape=style & DialogCommand::property_box ? QFrame::Box :
					style & DialogCommand::property_panel ? QFrame::Panel :
						style & DialogCommand::property_styled ? QFrame::StyledPanel : QFrame::NoFrame;
			shadow=style & DialogCommand::property_raised ? QFrame::Raised :
						style & DialogCommand::property_sunken ? QFrame::Sunken : QFrame::Plain;
			frm->setFrameStyle(shape | shadow);
		}

    group_layout=(vertical ? (QBoxLayout*)new QVBoxLayout : (QBoxLayout*)new QHBoxLayout);
    group_index=0;
    frm->setLayout(group_layout);
	group_layout->setAlignment(LAYOUTS_ALIGNMENT);
	current_layout->insertWidget(current_index++,frm);
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

void DialogBox::AddSeparator(const char* name, bool vertical, unsigned int style)
{
	unsigned int shadow=QFrame::Sunken;
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

	// style is a DialogCommand::Controls value
	if(style & DialogCommand::separator)
		{
			style&=DialogCommand::property_mask;
			shadow=style & DialogCommand::property_raised ? QFrame::Raised :
						style & DialogCommand::property_plain ? QFrame::Plain : QFrame::Sunken;
		}

	separator->setFrameShadow(QFrame::Shadow(shadow));

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

void DialogBox::StepHorizontal()
{
	QBoxLayout* oldlayout=current_layout;
	QVBoxLayout* vbox=new QVBoxLayout;

	((QBoxLayout*)current_layout->parent())->insertLayout(indexOf(current_layout)+1,vbox);
	current_layout=vbox;
	current_index=0;
	current_layout->setAlignment(LAYOUTS_ALIGNMENT);
	EndGroup();
	sanitize_layout(oldlayout);
}

void DialogBox::StepVertical()
{
	QBoxLayout* oldlayout=current_layout;
	QVBoxLayout* vbox=new QVBoxLayout;
	QHBoxLayout* hbox=new QHBoxLayout;
	QBoxLayout* rootlayout=(QBoxLayout*)current_layout->parent()->parent();

	rootlayout->insertLayout(indexOf((QLayout*)current_layout->parent())+1,hbox);
	hbox->addLayout(vbox);
	current_layout=vbox;
	current_index=0;
	current_layout->setAlignment(LAYOUTS_ALIGNMENT);
	EndGroup();
	sanitize_layout(oldlayout);
}

void DialogBox::RemoveWidget(const char* name)
{
	QWidget* widget;

	if( (widget=FindWidget(name)) )
		{
			if(QLayout* layout=FindLayout(widget))
				{
					if(widget->layout()==group_layout) EndGroup();	// for QGroupBox/QFrame objects
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

void DialogBox::Position(const char* name, bool behind, bool onto)
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

			if(parent->isWidgetType())	// for widgets installed onto QGroupBox/QFrame
				{
					group_index=index;
					if(behind) group_index++;

					group_layout=layout;

					if( (layout=(QBoxLayout*)FindLayout( widget=(QWidget*)parent )) )
						{
							current_index=indexOf(widget, layout)+1;
							current_layout=layout;
						}
				}
			else
				{
					EndGroup();
					current_index=index;
					if(behind) current_index++;

					current_layout=layout;

					if(onto && (layout=(QBoxLayout*)widget->layout()))	// position onto QGroupBox/QFrame object
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
 *	options and mask are DialogCommand::Controls values.
 *	mask indicates which bits to process when options indicates whether the
 *	option is to set or to unset.
 *	text is the string used for options which require string values.
 *
 * ****************************************************************************/
void DialogBox::SetOptions(QWidget* widget, unsigned int options, unsigned int mask, const char* text)
{
	if(!widget) return;

	const QMetaObject* metaobj=widget->metaObject();
	QMetaProperty property;
	unsigned int type=WidgetType(widget);

	options&=type | DialogCommand::property_mask;
	mask&=type | DialogCommand::property_mask;

	if(!(mask & type) || !(mask & DialogCommand::property_mask)) return;

	// default makes sense for set command only
	if(DialogCommand::property_default & type &&
		mask & DialogCommand::property_default & DialogCommand::property_mask &&
		options & DialogCommand::property_default & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("default"))).isWritable())
				property.write(widget, QVariant(true));
		}

	if(DialogCommand::property_checked & type &&
		mask & DialogCommand::property_checked & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("checked"))).isWritable())
				property.write(widget, QVariant(options & DialogCommand::property_checked & DialogCommand::property_mask));
		}

	if(DialogCommand::property_checkable & type &&
		mask & DialogCommand::property_checkable & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("checkable"))).isWritable())
				property.write(widget, QVariant(options & DialogCommand::property_checkable & DialogCommand::property_mask));
		}

	// password makes sense for QLineEdit objects only
	if(DialogCommand::property_password & type &&
		mask & DialogCommand::property_password & DialogCommand::property_mask)
		{
			if(QWidget* proxywidget=widget->focusProxy())
				{
					const QMetaObject* proxymetaobj=proxywidget->metaObject();
					if((property=proxymetaobj->property(proxymetaobj->indexOfProperty("echoMode"))).isWritable())
						property.write(proxywidget, QVariant(options & DialogCommand::property_password & DialogCommand::property_mask ? QLineEdit::Password : QLineEdit::Normal));
				}
		}

	// placeholder makes sense for QLineEdit objects only
	if(DialogCommand::property_placeholder & type &&
		mask & DialogCommand::property_placeholder & DialogCommand::property_mask)
		{
			if(QWidget* proxywidget=widget->focusProxy())
				{
					const QMetaObject* proxymetaobj=proxywidget->metaObject();
					if((property=proxymetaobj->property(proxymetaobj->indexOfProperty("placeholderText"))).isWritable())
						property.write(proxywidget, QVariant(QString(options & DialogCommand::property_placeholder & DialogCommand::property_mask ? text : NULL)));
				}
		}

	// text makes difference for QLineEdit objects only
	// for the rest it is the same as title
	if(DialogCommand::property_text & type &&
		mask & DialogCommand::property_text & DialogCommand::property_mask)
		{
			if(QWidget* proxywidget=widget->focusProxy())
				{
					const QMetaObject* proxymetaobj=proxywidget->metaObject();
					if((property=proxymetaobj->property(proxymetaobj->indexOfProperty("text"))).isWritable())
						property.write(proxywidget, QVariant(QString(options & DialogCommand::property_text & DialogCommand::property_mask ? text : NULL)));
				}
			else
				{	// this is not a QLineEdit object
					if(DialogCommand::property_title & type)
						{
							// this option will be applied by code below
							mask|=DialogCommand::property_title & DialogCommand::property_mask;
							options|=options & DialogCommand::property_text & DialogCommand::property_mask ? DialogCommand::property_title & DialogCommand::property_mask : 0;
						}
					// else there is a bug in DialogCommand::Controls enum
				}
		}

	if(DialogCommand::property_title & type &&
		mask & DialogCommand::property_title & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("title"))).isWritable() ||	// for QGroupBox objects
				(property=metaobj->property(metaobj->indexOfProperty("text"))).isWritable() ||	// for the rest widgets
				(property=metaobj->property(metaobj->indexOfProperty("windowTitle"))).isWritable())	// for the main window (QDialog object)
					{
						// avoid to format QLineEdit labels (which have focusProxy set)
						if(!widget->focusProxy()) sanitize_label(widget, DialogBox::text);
						property.write(widget, QVariant(QString(options & DialogCommand::property_title & DialogCommand::property_mask ? text : NULL)));

					}
		}

	// animation makes sense for QLabel objects only
	// also avoid to set animation for QLineEdit labels (which have focusProxy set)
	// DialogCommand::Controls enum distinguishes between QLabel and label of QLineEdit
	if(DialogCommand::property_animation & type &&
		mask & DialogCommand::property_animation & DialogCommand::property_mask)
		{
			// there is no movie property for QLabel objects
			sanitize_label(widget, DialogBox::movie);
			if(QMovie* mv=new QMovie(options & DialogCommand::property_animation & DialogCommand::property_mask ? text : NULL))
				{
					((QLabel*)widget)->setMovie(mv);
					mv->start();
					mv->setParent(widget);
				}
		}

	// picture makes sense for QLabel objects only
	// also avoid to set picture for QLineEdit labels (which have focusProxy set)
	// DialogCommand::Controls enum distinguishes between QLabel and label of QLineEdit
	if(DialogCommand::property_picture & type &&
		mask & DialogCommand::property_picture & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("pixmap"))).isWritable())
				{
					sanitize_label(widget, DialogBox::pixmap);
					property.write(widget, QVariant(QPixmap(options & DialogCommand::property_picture & DialogCommand::property_mask ? text : NULL)));
				}
		}

	if(DialogCommand::property_icon & type &&
		mask & DialogCommand::property_icon & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("icon"))).isWritable() ||
				(property=metaobj->property(metaobj->indexOfProperty("windowIcon"))).isWritable())
					property.write(widget, QVariant(QIcon(options & DialogCommand::property_icon & DialogCommand::property_mask ? text : NULL)));
		}

	// iconsize makes sense for set command only
	if(DialogCommand::property_iconsize & type &&
		mask & DialogCommand::property_iconsize & DialogCommand::property_mask &&
		options & DialogCommand::property_iconsize & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("iconSize"))).isWritable())
				{
					int size;
					sscanf(text, "%d", &size);
					property.write(widget, QVariant(QSize(size, size)));
				}
		}

	// below three shadow options make sense for set command only
	if(DialogCommand::property_raised & type &&
		mask & DialogCommand::property_raised & DialogCommand::property_mask &&
		options & DialogCommand::property_raised & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("frameShadow"))).isWritable())
				property.write(widget, QVariant(QFrame::Raised));
		}

	if(DialogCommand::property_sunken & type &&
		mask & DialogCommand::property_sunken & DialogCommand::property_mask &&
		options & DialogCommand::property_sunken & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("frameShadow"))).isWritable())
				property.write(widget, QVariant(QFrame::Sunken));
		}

	if(DialogCommand::property_plain & type &&
		mask & DialogCommand::property_plain & DialogCommand::property_mask &&
		options & DialogCommand::property_plain & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("frameShadow"))).isWritable())
				property.write(widget, QVariant(QFrame::Plain));
		}

	if(DialogCommand::property_box & type &&
		mask & DialogCommand::property_box & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("frameShape"))).isWritable())
				property.write(widget, QVariant(options & DialogCommand::property_box & DialogCommand::property_mask ? QFrame::Box : QFrame::NoFrame));
		}

	if(DialogCommand::property_panel & type &&
		mask & DialogCommand::property_panel & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("frameShape"))).isWritable())
				property.write(widget, QVariant(options & DialogCommand::property_panel & DialogCommand::property_mask ? QFrame::Panel : QFrame::NoFrame));
		}

	if(DialogCommand::property_styled & type &&
		mask & DialogCommand::property_styled & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("frameShape"))).isWritable())
				property.write(widget, QVariant(options & DialogCommand::property_styled & DialogCommand::property_mask ? QFrame::StyledPanel : QFrame::NoFrame));
		}

	// noframe shape option makes sense for set command only
	if(DialogCommand::property_noframe & type &&
		mask & DialogCommand::property_noframe & DialogCommand::property_mask &&
		options & DialogCommand::property_noframe & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("frameShape"))).isWritable())
				property.write(widget, QVariant(QFrame::NoFrame));
		}

	// apply and exit make sense for QPushButton objects only
	if(DialogCommand::property_apply & DialogCommand::property_exit & type &&
		(mask & DialogCommand::property_apply & DialogCommand::property_mask ||
		mask & DialogCommand::property_exit & DialogCommand::property_mask))
		{
			const unsigned int property_apply=DialogCommand::property_apply & DialogCommand::property_mask;
			const unsigned int property_exit=DialogCommand::property_exit & DialogCommand::property_mask;
			unsigned int pboptions=0;
			unsigned int loptions=options & (property_apply | property_exit);
			unsigned int lmask=mask & (property_apply | property_exit);

			if(disconnect(widget, SIGNAL(clicked()), this, SLOT(report())))
				{
					pboptions|=property_apply;
					if(disconnect(widget, SIGNAL(clicked()), this, SLOT(accept())))
						pboptions|=property_exit;
				}

			if(disconnect(widget, SIGNAL(clicked()), this, SLOT(reject())))
				pboptions|=property_exit;

			pboptions &= (~lmask) | loptions;	// reset 1s where needed
			pboptions |= lmask & loptions;		// set 1s where needed

			if(pboptions & property_apply)
				{
					connect(widget, SIGNAL(clicked()), this, SLOT(report()));
					if(pboptions & property_exit)
						connect(widget, SIGNAL(clicked()), this, SLOT(accept()));
				}
			else if(pboptions & property_exit)
					connect(widget, SIGNAL(clicked()), this, SLOT(reject()));
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
	QWidget* widget;

	if(empty)
		{
			RemoveWidget(about_label);
			empty=false;
		}
	switch(command.command & DialogCommand::command_mask)
		{
			case DialogCommand::add:
				switch(command.control & ~DialogCommand::property_mask)
					{
						case DialogCommand::label:
							AddLabel(command.GetTitle(), command.GetName(),
								command.control & DialogCommand::property_picture & DialogCommand::property_mask ? DialogBox::pixmap :
										command.control & DialogCommand::property_animation & DialogCommand::property_mask ? DialogBox::movie :
										DialogBox::text);
							break;
						case DialogCommand::groupbox:
							AddGroupbox(command.GetTitle(), command.GetName(),
								command.control & DialogCommand::property_vertical & DialogCommand::property_mask,
								command.control & DialogCommand::property_checkable & DialogCommand::property_mask,
								command.control & DialogCommand::property_checked & DialogCommand::property_mask);
							break;
						case DialogCommand::frame:
							AddFrame(command.GetTitle(),
								command.control & DialogCommand::property_vertical & DialogCommand::property_mask,
								command.control);
							break;
						case DialogCommand::pushbutton:
							AddPushbutton(command.GetTitle(), command.GetName(),
								command.control & DialogCommand::property_apply & DialogCommand::property_mask,
								command.control & DialogCommand::property_exit & DialogCommand::property_mask,
								command.control & DialogCommand::property_default & DialogCommand::property_mask);
							break;
						case DialogCommand::checkbox:
							AddCheckbox(command.GetTitle(), command.GetName(),
								command.control & DialogCommand::property_checked & DialogCommand::property_mask);
							break;
						case DialogCommand::radiobutton:
							AddRadiobutton(command.GetTitle(), command.GetName(),
								command.control & DialogCommand::property_checked & DialogCommand::property_mask);
							break;
						case DialogCommand::textbox:
							AddTextbox(command.GetTitle(), command.GetName(),
								command.GetText(),
								command.GetAuxText(),
								command.control & DialogCommand::property_password & DialogCommand::property_mask);
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
								command.control & DialogCommand::property_vertical & DialogCommand::property_mask,
								command.control);
							break;
					}
				break;
			case DialogCommand::end:
				switch(command.control)
					{
						case DialogCommand::groupbox:
							EndGroup();
							break;
						case DialogCommand::frame:
							EndGroup();
							break;
						case DialogCommand::widget_mask:	// none type mentioned
							EndGroup();
							break;
					}
				break;
			case DialogCommand::step:
				if(command.command & DialogCommand::option_vertical & DialogCommand::option_mask) StepVertical();
				else StepHorizontal();
				break;
			case DialogCommand::set:
				if(command.GetName()[0])
					{
						if(!(widget=FindWidget(command.GetName()))) break;
					}
				else widget=this;

				if(command.command & DialogCommand::option_enabled & DialogCommand::option_mask)
					set_enabled(widget, true);

				if(command.command & DialogCommand::option_focus & DialogCommand::option_mask)
					{
						QTimer::singleShot(0, widget, SLOT(setFocus()));

						// select text for QLineEdit objects
						// selectedText property is not writable and this must be done in the class specific way
						if(QWidget* proxywidget=widget->focusProxy())
							((QLineEdit*)proxywidget)->selectAll();
					}

				// see http://doc.qt.io/qt-4.8/stylesheet.html for reference
				if(command.command & DialogCommand::option_stylesheet & DialogCommand::option_mask)
					widget->setStyleSheet(command.GetText());

				if(command.control) SetOptions(widget, command.control, command.control, command.GetText());
				break;
			case DialogCommand::unset:
				if(command.GetName()[0])
					{
						if(!(widget=FindWidget(command.GetName()))) break;
					}
				else widget=this;

				if(command.command & DialogCommand::option_enabled & DialogCommand::option_mask)
					set_enabled(widget, false);

				// see http://doc.qt.io/qt-4.8/stylesheet.html for reference
				if(command.command & DialogCommand::option_stylesheet & DialogCommand::option_mask)
					widget->setStyleSheet(NULL);

				if(command.control) SetOptions(widget, 0, command.control, NULL);
				break;
			case DialogCommand::enable:
				if(command.GetName()[0])
					{
						if(!(widget=FindWidget(command.GetName()))) break;
					}
				else widget=this;
				set_enabled(widget, true);
				break;
			case DialogCommand::disable:
				if(command.GetName()[0])
					{
						if(!(widget=FindWidget(command.GetName()))) break;
					}
				else widget=this;
				set_enabled(widget, false);
				break;
			case DialogCommand::remove:
				RemoveWidget(command.GetName());
				break;
			case DialogCommand::position:
				Position(command.GetText(),
							command.command & DialogCommand::option_behind & DialogCommand::option_mask,
							command.command & DialogCommand::option_onto & DialogCommand::option_mask);
				break;
			case DialogCommand::print: // print command is temporary for debuging purposes
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

	// All widgets are chained. Their focus policies define the focus move.
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

							if( !(widget=li->widget()) )
								if( (layout=li->layout()) )
									widget=layout->itemAt(1)->widget(); // for QLineEdit objects

							if(widget)
								{
									if(prev_widget) setTabOrder(prev_widget, widget);
									prev_widget=widget;

									if(QBoxLayout* glayout=(QBoxLayout*)widget->layout())	// for QGroupBox/QFrame objects
										for(int i=0, j=glayout->count(); i<j; i++)
											{
												QLayout* layout;
												QLayoutItem* li=glayout->itemAt(i);

												if( !(widget=li->widget()) )
													if( (layout=li->layout()) )
														widget=layout->itemAt(1)->widget();

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
	if(WidgetType(label) == DialogCommand::label)
		{
			QMovie* mv;
			QLayout* layout;

			if( (mv=((QLabel*)label)->movie()) ) delete mv;

			if( (layout=FindLayout(label)) )
				switch(content)
					{
						case pixmap:
							layout->setAlignment(label, GRAPHICS_ALIGNMENT);	// set widget alignment
							layout->setAlignment(GRAPHICS_ALIGNMENT);			// set layout alignment
							label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
							//reset to defaults
							((QLabel*)label)->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
							((QLabel*)label)->setOpenExternalLinks(false);
							((QLabel*)label)->setWordWrap(false);
							break;
						case movie:
							layout->setAlignment(label, GRAPHICS_ALIGNMENT);
							layout->setAlignment(GRAPHICS_ALIGNMENT);
							label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
							//reset to defaults
							((QLabel*)label)->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
							((QLabel*)label)->setOpenExternalLinks(false);
							((QLabel*)label)->setWordWrap(false);
							break;
						case text:
							layout->setAlignment(label, DEFAULT_ALIGNMENT); // layout default alignment for proper text displaying
							layout->setAlignment(DEFAULT_ALIGNMENT);
							label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
							// Qt::TextBrowserInteraction includes:
							//		TextSelectableByMouse | LinksAccessibleByMouse | LinksAccessibleByKeyboard
							// to make the text label not interactive use stylesheets
							// (e.g. qproperty-textInteractionFlags: NoTextInteraction;)
							((QLabel*)label)->setTextInteractionFlags(Qt::TextBrowserInteraction);
							((QLabel*)label)->setOpenExternalLinks(true);
							((QLabel*)label)->setWordWrap(true);
							break;
					}
		}
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

void DialogBox::print_widget(QWidget* widget)
{
	if(widget->isEnabled())
		{
			const char* name=widget->objectName().toLocal8Bit().constData();

			if(name[0])
				{
					const QMetaObject* metaobj=widget->metaObject();

					if(metaobj->property(metaobj->indexOfProperty("checkable")).read(widget).toBool())
						{
							fprintf(output, "%s=%s\n", name,
								metaobj->property(metaobj->indexOfProperty("checked")).read(widget).toBool() ? "1" : "0");
							fflush(output);
						}

					if( (widget=widget->focusProxy()) )
						{
							metaobj=widget->metaObject();
							fprintf(output, "%s=", name);
							fprintf(output, "%s\n",
								metaobj->property(metaobj->indexOfProperty("text")).read(widget).toString().toLocal8Bit().constData());
							fflush(output);
						}

				}
		}
}

/*******************************************************************************
 *	below function is temporary for debuging purposes, accessible via print command
 * ****************************************************************************/
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


/*******************************************************************************
 *
 *	NON-CLASS MEMBER FUNCTIONS
 *
 * ****************************************************************************/

/*******************************************************************************
 *	WidgetType returns the type of the widget as value of
 *  DialogCommand::Controls enum
 * ****************************************************************************/
DialogCommand::Controls WidgetType(QWidget* widget)
{
	const char* name=widget->metaObject()->className();

	if(!strcmp(name, "DialogBox")) return(DialogCommand::dialog);
	if(!strcmp(name, "QFrame"))
		{
			int shape=((QFrame *)widget)->frameStyle() & QFrame::Shape_Mask;

			if(shape==QFrame::HLine || shape==QFrame::VLine) return(DialogCommand::separator);
			return(DialogCommand::frame);
		}
	if(!strcmp(name, "QLabel"))
		{
			if(QWidget* proxywidget=widget->focusProxy())
				{
					if(!strcmp(proxywidget->metaObject()->className(), "QLineEdit")) return(DialogCommand::textbox);
				}
			return(DialogCommand::label);
		}
	if(!strcmp(name, "QGroupBox")) return(DialogCommand::groupbox);
	if(!strcmp(name, "QPushButton")) return(DialogCommand::pushbutton);
	if(!strcmp(name, "QRadioButton")) return(DialogCommand::radiobutton);
	if(!strcmp(name, "QCheckBox")) return(DialogCommand::checkbox);
	if(!strcmp(name, "QLineEdit")) return(DialogCommand::textbox); // if the object queried directly

	return(DialogCommand::none);
}

void set_enabled(QWidget* widget, bool enable)
{
	widget->setEnabled(enable);
	if(QWidget* proxywidget=widget->focusProxy()) proxywidget->setEnabled(enable);
}

QWidget* find_widget_recursively(QLayoutItem* item, const char* name)
{
	QLayout* layout;
	QWidget* widget;

    if((layout=item->layout()))
		{
			for(int i=0, j=layout->count(); i<j; i++)
				if( (widget=find_widget_recursively(layout->itemAt(i), name)) ) return(widget);
		}
    else
		if((widget=item->widget()))
			{
				if(!strcmp(widget->objectName().toLocal8Bit().constData(),name)) return(widget);
				if((layout=widget->layout())) return(find_widget_recursively(layout, name));
			}
	return(NULL);
}

QLayout* find_layout_recursively(QLayout* layout, QWidget* widget)
{

    if(widget && layout)
		{
			for(int i=0, j=layout->count(); i<j; i++)
				{
					QLayoutItem* item;
					QWidget* w;
					QLayout* retlayout;

					if((w=(item=layout->itemAt(i))->widget()) == widget) return(layout);
					else
						if( (retlayout=find_layout_recursively(w ? w->layout() : item->layout(), widget)) )
							return(retlayout);
				}
		}
	return(NULL);
}
