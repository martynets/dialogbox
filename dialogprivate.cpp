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
									widget=layout->itemAt(1)->widget(); // for coupled objects

							if(widget)
								{
									if(widget->focusPolicy()!=Qt::NoFocus)
										{
											if(prev_widget) setTabOrder(prev_widget, widget);
											prev_widget=widget;
										}

									if(QBoxLayout* glayout=(QBoxLayout*)widget->layout())	// for QGroupBox/QFrame objects
										for(int i=0, j=glayout->count(); i<j; i++)
											{
												QLayout* layout;
												QLayoutItem* li=glayout->itemAt(i);

												if( !(widget=li->widget()) )
													if( (layout=li->layout()) )
														widget=layout->itemAt(1)->widget();

												if(widget && widget->focusPolicy()!=Qt::NoFocus)
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
					QWidget* proxywidget;
					const QMetaObject* metaobj=widget->metaObject();
					int propindex;

					if(metaobj->property(metaobj->indexOfProperty("checkable")).read(widget).toBool())
						{
							fprintf(output, "%s=%s\n", name,
								metaobj->property(metaobj->indexOfProperty("checked")).read(widget).toBool() ? "1" : "0");
							fflush(output);
							return;
						}

					if((propindex=metaobj->indexOfProperty("value"))>=0 &&
						WidgetType(widget)!=DialogCommand::progressbar)
						{
							fprintf(output, "%s=%d\n", name, metaobj->property(propindex).read(widget).toInt());
							fflush(output);
							return;
						}

					if( (proxywidget=widget->focusProxy()) )
						{
							QListWidgetItem* item;

							fprintf(output, "%s=", name);
							switch((unsigned)WidgetType(proxywidget))
								{
									case DialogCommand::combobox:
										fprintf(output, "%s\n",
											((QComboBox*)proxywidget)->currentText().toLocal8Bit().constData());
										break;
									case DialogCommand::listbox:
										item=((QListWidget*)proxywidget)->currentItem();
										fprintf(output, "%s\n", item ? item->text().toLocal8Bit().constData() : "");
										break;
									default:
										metaobj=proxywidget->metaObject();
										fprintf(output, "%s\n",
											metaobj->property(metaobj->indexOfProperty("text")).read(proxywidget).toString().toLocal8Bit().constData());
										break;
								}
							fflush(output);
							return;
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

