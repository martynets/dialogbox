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
 *	Slot function. Reimplemented. Simply terminates the QCoreApplication::exec()
 * 	function with return value of the r which in turn is returned by the main
 * 	function.
 * ****************************************************************************/
void DialogBox::done(int r)
{
	QCoreApplication::exit(r);
}

/*******************************************************************************
 *	Slot function. Removes page widget from pages list.
 * ****************************************************************************/
void DialogBox::RemovePage(QObject* page)
{
	for(int i=0, j=pages.count(); i<j; i++)
		if((QObject*)pages.at(i) == page)
			{
				pages.removeAt(i);
				break;
			}
}

/*******************************************************************************
 *	Slot function. Reports values of all reportable enabled widgets.
 * ****************************************************************************/
void DialogBox::Report()
{
	for(int i=0, j=pages.count(); i<j; i++)
		print_widgets_recursively(pages.at(i)->layout());
}

/*******************************************************************************
 *	Slot function. Reports the pushbutton is clicked.
 * ****************************************************************************/
void DialogBox::PushbuttonClicked()
{
	QPushButton* pb=(QPushButton*)sender();
	const char* objectname=pb->objectName().toLocal8Bit().constData();

	if(objectname[0] && !pb->isCheckable())
		{
			fprintf(output, "%s=clicked\n", objectname);
			fflush(output);
		}
}

/*******************************************************************************
 *	Slot function. Reports the pushbutton is toggled (pressed or released).
 * 	Only checkable pushbutton can te toggled.
 * ****************************************************************************/
void DialogBox::PushbuttonToggled(bool checked)
{
	const char* objectname=sender()->objectName().toLocal8Bit().constData();

	if(objectname[0])
		{
			fprintf(output, "%s=%s\n", objectname, checked ? "pressed" : "released");
			fflush(output);
		}
}

/*******************************************************************************
 *	Slot function. Reports the listbox item is activated.
 * ****************************************************************************/
void DialogBox::ListboxItemActivated(const QModelIndex& index)
{
	Listbox* list=(Listbox*)sender();
	QLayout* layout;
	QWidget* label;

	if( (layout=FindLayout(list)) && (label=layout->itemAt(0)->widget()) )
		{
			const char* objectname=label->objectName().toLocal8Bit().constData();

			if(objectname[0])
				{
					fprintf(output, "%s=", objectname);
					fprintf(output, "%s\n", index.data().toString().toLocal8Bit().constData());
					fflush(output);
				}
		}
}

/*******************************************************************************
 *	Slot function. Reports the current item of the listbox is changed.
 * ****************************************************************************/
void DialogBox::ListboxItemSelected(QListWidgetItem* current)
{
	Listbox* list=(Listbox*)sender();
	QLayout* layout;
	QWidget* label;

	if( (layout=FindLayout(list)) && (label=layout->itemAt(0)->widget()) )
		{
			const char* objectname=label->objectName().toLocal8Bit().constData();

			if(objectname[0])
				{
					fprintf(output, "%s=", objectname);
					fprintf(output, "%s\n", current->text().toLocal8Bit().constData());
					fflush(output);
				}
		}
}

/*******************************************************************************
 *	Slot function. Reports the current item of the combobox is changed.
 * ****************************************************************************/
void DialogBox::ComboboxItemSelected(int index)
{
	QComboBox* list=(QComboBox*)sender();
	QLayout* layout;
	QWidget* label;

	if( (layout=FindLayout(list)) && (label=layout->itemAt(0)->widget()) )
		{
			const char* objectname=label->objectName().toLocal8Bit().constData();

			if(objectname[0])
				{
					fprintf(output, "%s=", objectname);
					fprintf(output, "%s\n", list->itemText(index).toLocal8Bit().constData());
					fflush(output);
				}
		}
}

/*******************************************************************************
 *	Slot function. Reports new value of the slider.
 * ****************************************************************************/
void DialogBox::SliderValueChanged(int value)
{
	const char* objectname=sender()->objectName().toLocal8Bit().constData();

	if(objectname[0])
		{
			fprintf(output, "%s=%d\n", objectname, value);
			fflush(output);
		}
}

/*******************************************************************************
 *	Slot function. Updates tickInterval and pageStep values of the slider.
 * ****************************************************************************/
void DialogBox::SliderRangeChanged(int min, int max)
{
	QSlider* slider=(QSlider*)sender();
	int ps, ss;

	ps=(max-min)/10;
	if(!ps) ps=1;

	ss=ps/10;
	if(!ss) ss=1;

	slider->setSingleStep(ss);
	slider->setPageStep(ps);
	slider->setTickInterval(ps);
}

/*******************************************************************************
 *	Slot function. Translates command object recevied from the parser thread
 * 	to appropriate function call.
 * ****************************************************************************/
void DialogBox::ExecuteCommand(DialogCommand command)
{
	QWidget* widget=NULL;

	if(empty)
		{
			ClearDialog();
			empty=false;
		}
	switch(command.command & DialogCommand::command_mask)
		{
			case DialogCommand::add:

				if(command.command & DialogCommand::option_space & DialogCommand::option_mask)
					{
						// Seems sscanf %d in some versions of standard C library has a bug
						// returning 32k on sero-size strings
						if(command.GetText()[0])
							{
								int size;
								sscanf(command.GetText(), "%d", &size);
								AddSpace(size);
							}
						else AddSpace();
						break;
					}

				if(command.command & DialogCommand::option_stretch & DialogCommand::option_mask)
					{
						AddStretch();
						break;
					}

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
						case DialogCommand::listbox:
							AddListbox(command.GetTitle(), command.GetName(),
							command.control & DialogCommand::property_activation & DialogCommand::property_mask,
							command.control & DialogCommand::property_selection & DialogCommand::property_mask);
							break;
						case DialogCommand::combobox:
							AddCombobox(command.GetTitle(), command.GetName(),
							command.control & DialogCommand::property_editable & DialogCommand::property_mask,
							command.control & DialogCommand::property_selection & DialogCommand::property_mask);
							break;
						case DialogCommand::item:
							AddItem(command.GetTitle(), command.GetName(),
							command.control & DialogCommand::property_current & DialogCommand::property_mask);
							break;
						case DialogCommand::separator:
							AddSeparator(command.GetTitle(),
								command.control & DialogCommand::property_vertical & DialogCommand::property_mask,
								command.control);
							break;
						case DialogCommand::progressbar:
							AddProgressbar(command.GetTitle(),
								command.control & DialogCommand::property_vertical & DialogCommand::property_mask,
								command.control & DialogCommand::property_busy & DialogCommand::property_mask);
							break;
						case DialogCommand::slider:
							{
								int min(0), max(100);
								if(command.GetName()[0]) sscanf(command.GetName(), "%d", &min);
								if(command.GetText()[0]) sscanf(command.GetText(), "%d", &max);
								AddSlider(command.GetTitle(),
									command.control & DialogCommand::property_vertical & DialogCommand::property_mask,
									min, max);
							}
							break;
						case DialogCommand::textview:
							AddTextview(command.GetTitle(), command.GetName());
							break;
						case DialogCommand::tabs:
							AddTabs(command.GetTitle(), command.control);
							break;
						case DialogCommand::page:
							AddPage(command.GetTitle(), command.GetName(), command.GetText(),
							command.control & DialogCommand::property_current & DialogCommand::property_mask);
							break;
					}
				break;
			case DialogCommand::clear:
				Clear(command.GetName());
				break;
			case DialogCommand::end:
				switch(command.control & ~DialogCommand::property_mask)
					{
						case DialogCommand::groupbox:
						case DialogCommand::frame:
							EndGroup();
							break;
						case DialogCommand::listbox:
						case DialogCommand::combobox:
							EndList();
							break;
						case DialogCommand::tabs:
							EndTabs();
							break;
						case DialogCommand::page:
							EndPage();
							break;
						case DialogCommand::widget_mask:	// none type mentioned
							if(current_view) EndList();
							else if(group_layout) EndGroup();
							else if(current_tab_widget)
								{
									if(current_tab_widget->indexOf(current_layout->parentWidget())==-1) EndTabs();
									else EndPage();
								}
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
					SetEnabled(widget, true);

				if(command.command & DialogCommand::option_focus & DialogCommand::option_mask)
					{
						QTimer::singleShot(0, widget, SLOT(setFocus()));

						// select text for QLineEdit objects
						// selectedText property is not writable and this must be done in the class specific way
						if(QWidget* proxywidget=widget->focusProxy())
							{
								switch(WidgetType(proxywidget))
									{
										case DialogCommand::combobox:
											proxywidget=((QComboBox*)proxywidget)->lineEdit();
											break;
										case DialogCommand::textbox:
											break;
										default:
											proxywidget=NULL;
											break;
									}
								if(proxywidget) ((QLineEdit*)proxywidget)->selectAll();
							}
					}

				// see http://doc.qt.io/qt-4.8/stylesheet.html for reference
				if(command.command & DialogCommand::option_stylesheet & DialogCommand::option_mask)
					{
						widget->setStyleSheet(command.GetText());
						if(QWidget* proxywidget=widget->focusProxy())
							proxywidget->setStyleSheet(command.GetText());
					}

				if(command.command & DialogCommand::option_visible & DialogCommand::option_mask)
					{
						QTimer::singleShot(0, widget, SLOT(show()));	// calling slot from slot works not always
						if(QWidget* proxywidget=widget->focusProxy()) QTimer::singleShot(0, proxywidget, SLOT(show()));
					}

				if(command.control) SetOptions(widget, command.control, command.control, command.GetText());
				break;
			case DialogCommand::unset:
				if(command.GetName()[0])
					{
						if(!(widget=FindWidget(command.GetName()))) break;
					}
				else widget=this;

				if(command.command & DialogCommand::option_enabled & DialogCommand::option_mask)
					SetEnabled(widget, false);

				// see http://doc.qt.io/qt-4.8/stylesheet.html for reference
				if(command.command & DialogCommand::option_stylesheet & DialogCommand::option_mask)
					{
						// rarely this fails (unset stylesheet or set it to empty string)
						// could be the same reason: calling slot from slot but all tricks to avoid this e.g.
						// QMetaObject::invokeMethod(widget, "setStyleSheet", Qt::QueuedConnection, Q_ARG(QString, ""));
						// gave 0 results
						// there must be some issue in style sheet subsystem
						widget->setStyleSheet(QString());
						if(QWidget* proxywidget=widget->focusProxy())
							proxywidget->setStyleSheet(QString());
					}

				if(command.command & DialogCommand::option_visible & DialogCommand::option_mask)
					{
						QTimer::singleShot(0, widget, SLOT(hide()));	// calling slot from slot works not always
						if(QWidget* proxywidget=widget->focusProxy()) QTimer::singleShot(0, proxywidget, SLOT(hide()));
					}

				if(command.control) SetOptions(widget, 0, command.control, NULL);
				break;
			case DialogCommand::remove:
				RemoveWidget(command.GetName());
				break;
			case DialogCommand::position:
				Position(command.GetText(),
							command.command & DialogCommand::option_behind & DialogCommand::option_mask,
							command.command & DialogCommand::option_onto & DialogCommand::option_mask);
				break;
			case DialogCommand::query:
				Report();
				break;
			case DialogCommand::noop:
			default:
				;
		}
	// clean up after possible FindWidget call
	chosen_view=NULL;
	chosen_row_flag=false;
}
