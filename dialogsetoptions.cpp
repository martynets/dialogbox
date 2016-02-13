/*
 * GUI widgets for shell scripts - dialogbox version 1.0
 *
 * Copyright (C) 2015, 2016 Andriy Martynets <martynets@volia.ua>
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

#include "dialogbox.hpp"


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

	unsigned int type=WidgetType(widget);

	options&=type | DialogCommand::property_mask;
	mask&=type | DialogCommand::property_mask;

	if(!(mask & type) || !(mask & DialogCommand::property_mask)) return;

	QWidget* proxywidget;
	const QMetaObject* metaobj=widget->metaObject();
	const QMetaObject* proxymetaobj;
	QMetaProperty property;

	if( (proxywidget=widget->focusProxy()) ) proxymetaobj=proxywidget->metaObject();
	else proxywidget=widget, proxymetaobj=metaobj;

	if(DialogCommand::property_default & type &&
		mask & DialogCommand::property_default & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("default"))).isWritable())
				{
					property.write(widget, QVariant((bool)(options & DialogCommand::property_default & DialogCommand::property_mask)));
					if(options & DialogCommand::property_default & DialogCommand::property_mask)
						default_pb=(QPushButton*)widget;
					else if(default_pb==(QPushButton*)widget) default_pb=NULL;
				}
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
			// changing checkable status for some widgets might change their focus policy to/from Qt::NoFocus (e.g. for QGroupBox)
			QWidget* page=widget->parentWidget();
			if(WidgetType(page)!=DialogCommand::page) page=page->parentWidget();
			update_tab_order(page);
		}

	// password makes sense for QLineEdit objects only
	if(DialogCommand::property_password & type &&
		mask & DialogCommand::property_password & DialogCommand::property_mask)
		{
			if((property=proxymetaobj->property(proxymetaobj->indexOfProperty("echoMode"))).isWritable())
				property.write(proxywidget, QVariant(options & DialogCommand::property_password & DialogCommand::property_mask ? QLineEdit::Password : QLineEdit::Normal));
		}

	// placeholder makes sense for QLineEdit objects only
	if(DialogCommand::property_placeholder & type &&
		mask & DialogCommand::property_placeholder & DialogCommand::property_mask)
		{
			if((property=proxymetaobj->property(proxymetaobj->indexOfProperty("placeholderText"))).isWritable())
				property.write(proxywidget, QVariant(QString(options & DialogCommand::property_placeholder & DialogCommand::property_mask ? text : NULL)));
		}

	// text makes difference for QLineEdit objects only (editable combobox includes QLineEdit object)
	// for the rest it is the same as title
	if(DialogCommand::property_text & type &&
		mask & DialogCommand::property_text & DialogCommand::property_mask)
		{
			QWidget* lewidget=proxywidget;

			if(proxywidget!=widget &&
					(type==DialogCommand::textbox ||
						(type==DialogCommand::combobox && (lewidget=((QComboBox*)proxywidget)->lineEdit())) ))
				{
					const QMetaObject* lemetaobj=lewidget->metaObject();
					if((property=lemetaobj->property(lemetaobj->indexOfProperty("text"))).isWritable())
						property.write(lewidget, QVariant(QString(options & DialogCommand::property_text & DialogCommand::property_mask ? text : NULL)));
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
			switch(type)
				{
					case DialogCommand::item:
						{
							QAbstractItemModel* model=chosen_view->model();
							if(chosen_row>=0)
								model->setData(model->index(chosen_row,0), QString(options & DialogCommand::property_title & DialogCommand::property_mask ? text : NULL), Qt::DisplayRole);
						}
						break;
					case DialogCommand::page:
						{
							QTabWidget* tabs=(QTabWidget*)widget->parent()->parent();
							tabs->setTabText(tabs->indexOf(widget), QString(options & DialogCommand::property_title & DialogCommand::property_mask ? text : NULL));
						}
						break;
					default:
						if((property=metaobj->property(metaobj->indexOfProperty("title"))).isWritable() ||	// for QGroupBox objects
							(property=metaobj->property(metaobj->indexOfProperty("text"))).isWritable() ||	// for the rest widgets
							(property=metaobj->property(metaobj->indexOfProperty("windowTitle"))).isWritable())	// for the main window (QDialog object)
								{
									// avoid to format labels of coupled widgets (which have focusProxy set)
									if(widget==proxywidget) sanitize_label(widget, DialogBox::text);
									property.write(widget, QVariant(QString(options & DialogCommand::property_title & DialogCommand::property_mask ? text : NULL)));
								}
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
			switch(type)
				{
					case DialogCommand::item:
						{
							QAbstractItemModel* model=chosen_view->model();
							if(chosen_row>=0)
								model->setData(model->index(chosen_row,0), QIcon(options & DialogCommand::property_icon & DialogCommand::property_mask ? text : NULL), Qt::DecorationRole);
						}
						break;
					case DialogCommand::page:
						{
							QTabWidget* tabs=(QTabWidget*)widget->parent()->parent();
							tabs->setTabIcon(tabs->indexOf(widget), QIcon(options & DialogCommand::property_icon & DialogCommand::property_mask ? text : NULL));
						}
						break;
					default:
						if((property=metaobj->property(metaobj->indexOfProperty("icon"))).isWritable() ||
							(property=metaobj->property(metaobj->indexOfProperty("windowIcon"))).isWritable())
								property.write(widget, QVariant(QIcon(options & DialogCommand::property_icon & DialogCommand::property_mask ? text : NULL)));
				}
		}

	// iconsize makes sense for set command only
	if(DialogCommand::property_iconsize & type &&
		mask & DialogCommand::property_iconsize & DialogCommand::property_mask &&
		options & DialogCommand::property_iconsize & DialogCommand::property_mask)
		{
			if((property=proxymetaobj->property(proxymetaobj->indexOfProperty("iconSize"))).isWritable())
				{
					if(text[0])
						{
							int size;
							sscanf(text, "%d", &size);
							property.write(proxywidget, QVariant(QSize(size, size)));
						}
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

			if(disconnect(widget, SIGNAL(clicked()), this, SLOT(Report())))
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
					connect(widget, SIGNAL(clicked()), this, SLOT(Report()));
					if(pboptions & property_exit)
						connect(widget, SIGNAL(clicked()), this, SLOT(accept()));
				}
			else if(pboptions & property_exit)
					connect(widget, SIGNAL(clicked()), this, SLOT(reject()));
		}

	if(DialogCommand::property_activation & type &&
		mask & DialogCommand::property_activation & DialogCommand::property_mask)
		{
			disconnect(proxywidget, SIGNAL(activated(const QModelIndex&)), this, SLOT(ListboxItemActivated(const QModelIndex&)));
			((Listbox*)proxywidget)->SetActivateFlag(false);
			if(options & DialogCommand::property_activation & DialogCommand::property_mask)
				{
					connect(proxywidget, SIGNAL(activated(const QModelIndex&)), this, SLOT(ListboxItemActivated(const QModelIndex&)));
					((Listbox*)proxywidget)->SetActivateFlag(true);
				}
		}

	if(DialogCommand::property_selection & type &&
		mask & DialogCommand::property_selection & DialogCommand::property_mask)
		{
			switch(type)
				{
					case DialogCommand::combobox:
						disconnect(proxywidget, SIGNAL(currentIndexChanged(int)), this, SLOT(ComboboxItemSelected(int)));
						if(options & DialogCommand::property_selection & DialogCommand::property_mask)
							connect(proxywidget, SIGNAL(currentIndexChanged(int)), this, SLOT(ComboboxItemSelected(int)));
						break;
					case DialogCommand::listbox:
						disconnect(proxywidget, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
							this, SLOT(ListboxItemSelected(QListWidgetItem*)));
						if(options & DialogCommand::property_selection & DialogCommand::property_mask)
							connect(proxywidget, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
								this, SLOT(ListboxItemSelected(QListWidgetItem*)));
						break;
				}
		}

	// current option makes sense for set command only
	if(DialogCommand::property_current & type &&
		mask & DialogCommand::property_current & DialogCommand::property_mask &&
		options & DialogCommand::property_current & DialogCommand::property_mask)
		{
			switch(type)
				{
					case DialogCommand::item:
						if(chosen_row>=0)
							{
								if(chosen_view != (Listbox*)chosen_list_widget)	// QComboBox widget
									((QComboBox*)chosen_list_widget)->setCurrentIndex(chosen_row);
								else	// Listbox widget
									chosen_view->setCurrentIndex(chosen_view->model()->index(chosen_row,0));
							}
						break;
					case DialogCommand::page:
						((QTabWidget*)widget->parent()->parent())->setCurrentWidget(widget);
						break;
				}
		}

	if(DialogCommand::property_minimum & type &&
		mask & DialogCommand::property_minimum & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("minimum"))).isWritable())
				{
					int min(0);

					if(options & DialogCommand::property_minimum & DialogCommand::property_mask && text[0])
						sscanf(text, "%d", &min);
					property.write(widget, QVariant(min));
				}
		}

	if(DialogCommand::property_maximum & type &&
		mask & DialogCommand::property_maximum & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("maximum"))).isWritable())
				{
					int max(100);

					if(options & DialogCommand::property_maximum & DialogCommand::property_mask && text[0])
						sscanf(text, "%d", &max);
					property.write(widget, QVariant(max));
				}
		}
	// reset() for QProgressBar objects must be done in the class specific way
	if(DialogCommand::property_value & type &&
		mask & DialogCommand::property_value & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("value"))).isWritable())
				{

					if(!(options & DialogCommand::property_value & DialogCommand::property_mask) &&
						type==DialogCommand::progressbar)
						{
							((QProgressBar*)widget)->reset();
						}
					else
						{
							int value(0);
							if(text[0]) sscanf(text, "%d", &value);
//							((QProgressBar*)widget)->setValue(value);
							property.write(widget, QVariant(value));
						}
				}
		}

	// busy makes sense for QProgressBar objects only
	if(DialogCommand::property_busy & type &&
		mask & DialogCommand::property_busy & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("maximum"))).isWritable())
				property.write(widget, QVariant(options & DialogCommand::property_busy & DialogCommand::property_mask ? 0 : 100));
		}

	// file makes sense for QTextEdit objects only
	if(DialogCommand::property_file & type &&
		mask & DialogCommand::property_file & DialogCommand::property_mask)
		{
			if(options & DialogCommand::property_file & DialogCommand::property_mask)
				{
					QFile txt(text);
					if(txt.open(QFile::ReadOnly)) ((QTextEdit*)widget)->setText(QTextStream(&txt).readAll());
				}
			else
				((QTextEdit*)widget)->clear();
		}

	// below four position options make sense for set command only and for QTabWidget objects only
	if(DialogCommand::property_position_top & type &&
		mask & DialogCommand::property_position_top & DialogCommand::property_mask &&
		options & DialogCommand::property_position_top & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("tabPosition"))).isWritable())
				property.write(widget, QVariant(QTabWidget::North));
		}

	if(DialogCommand::property_position_bottom & type &&
		mask & DialogCommand::property_position_bottom & DialogCommand::property_mask &&
		options & DialogCommand::property_position_bottom & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("tabPosition"))).isWritable())
				property.write(widget, QVariant(QTabWidget::South));
		}

	if(DialogCommand::property_position_left & type &&
		mask & DialogCommand::property_position_left & DialogCommand::property_mask &&
		options & DialogCommand::property_position_left & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("tabPosition"))).isWritable())
				property.write(widget, QVariant(QTabWidget::West));
		}

	if(DialogCommand::property_position_right & type &&
		mask & DialogCommand::property_position_right & DialogCommand::property_mask &&
		options & DialogCommand::property_position_right & DialogCommand::property_mask)
		{
			if((property=metaobj->property(metaobj->indexOfProperty("tabPosition"))).isWritable())
				property.write(widget, QVariant(QTabWidget::East));
		}
}
