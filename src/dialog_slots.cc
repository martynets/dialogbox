/*
 * GUI widgets for shell scripts - dialogbox version 1.0
 *
 * Copyright (C) 2015-2016, 2020 Andriy Martynets <andy.martynets@gmail.com>
 *------------------------------------------------------------------------------
 * This file is part of dialogbox.
 *
 * Dialogbox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Dialogbox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dialogbox. If not, see http://www.gnu.org/licenses/.
 *------------------------------------------------------------------------------
 */

#include "dialogbox.h"

using namespace DialogCommandTokens;

/*******************************************************************************
 *  Slot function. Reimplemented. Simply terminates the QCoreApplication::exec()
 *  function with return value of the r which in turn is returned by the main
 *  function.
 ******************************************************************************/
void DialogBox::done(int r)
{
    QCoreApplication::exit(r);
}

/*******************************************************************************
 *  Slot function. Removes page widget from pages list.
 ******************************************************************************/
void DialogBox::removePage(QObject *page)
{
    for (int i = 0, j = pages.count(); i < j; i++) {
        if ((QObject *)pages.at(i) == page) {
            pages.removeAt(i);
            break;
        }
    }
}

/*******************************************************************************
 *  Slot function. Reports values of all reportable enabled widgets.
 ******************************************************************************/
void DialogBox::report()
{
    for (int i = 0, j = pages.count(); i < j; i++)
        printWidgetsRecursively(pages.at(i)->layout());
}

/*******************************************************************************
 *  Slot function. Reports the pushbutton is clicked.
 ******************************************************************************/
void DialogBox::pushButtonClicked()
{
    QPushButton *pb = (QPushButton *)sender();
    const char *objectName = pb->objectName().toLocal8Bit().constData();

    if (objectName[0] && !pb->isCheckable()) {
        fprintf(output, "%s=clicked\n", objectName);
        fflush(output);
    }
}

/*******************************************************************************
 *  Slot function. Reports the pushbutton is toggled (pressed or released).
 *  Only checkable pushbutton can te toggled.
 ******************************************************************************/
void DialogBox::pushButtonToggled(bool checked)
{
    const char *objectName = sender()->objectName().toLocal8Bit().constData();

    if (objectName[0]) {
        fprintf(output, "%s=%s\n", objectName,
                checked ? "pressed" : "released");
        fflush(output);
    }
}

/*******************************************************************************
 *  Slot function. Reports the listbox item is activated.
 ******************************************************************************/
void DialogBox::listBoxItemActivated(const QModelIndex &index)
{
    ListBox *list = (ListBox *)sender();
    QLayout *layout;
    QWidget *label;

    if ((layout = findLayout(list)) && (label = layout->itemAt(0)->widget())) {
        const char *objectName = label->objectName().toLocal8Bit().constData();

        if (objectName[0]) {
            fprintf(output, "%s=", objectName);
            fprintf(output, "%s\n",
                    index.data().toString().toLocal8Bit().constData());
            fflush(output);
        }
    }
}

/*******************************************************************************
 *  Slot function. Reports the current item of the listbox is changed.
 ******************************************************************************/
void DialogBox::listBoxItemSelected(QListWidgetItem *current)
{
    ListBox *list = (ListBox *)sender();
    QLayout *layout;
    QWidget *label;

    if ((layout = findLayout(list)) && (label = layout->itemAt(0)->widget())) {
        const char *objectName = label->objectName().toLocal8Bit().constData();

        if (objectName[0]) {
            fprintf(output, "%s=", objectName);
            fprintf(output, "%s\n",
                    current ? current->text().toLocal8Bit().constData() : "");
            fflush(output);
        }
    }
}

/*******************************************************************************
 *  Slot function. Reports the current item of the combobox is changed.
 ******************************************************************************/
void DialogBox::comboBoxItemSelected(int index)
{
    QComboBox *list = (QComboBox *)sender();
    QLayout *layout;
    QWidget *label;

    if ((layout = findLayout(list)) && (label = layout->itemAt(0)->widget())) {
        const char *objectName = label->objectName().toLocal8Bit().constData();

        if (objectName[0]) {
            fprintf(output, "%s=", objectName);
            fprintf(output, "%s\n",
                    list->itemText(index).toLocal8Bit().constData());
            fflush(output);
        }
    }
}

/*******************************************************************************
 *  Slot function. Reports new value of the slider.
 ******************************************************************************/
void DialogBox::sliderValueChanged(int value)
{
    const char *objectName = sender()->objectName().toLocal8Bit().constData();

    if (objectName[0]) {
        fprintf(output, "%s=%d\n", objectName, value);
        fflush(output);
    }
}

/*******************************************************************************
 *  Slot function. Updates tickInterval and pageStep values of the slider.
 ******************************************************************************/
void DialogBox::sliderRangeChanged(int min, int max)
{
    QSlider *slider = (QSlider *)sender();
    int ps;
    int ss;

    ps = (max - min) / 10;
    if (!ps)
        ps = 1;

    ss = ps / 10;
    if (!ss)
        ss = 1;

    slider->setSingleStep(ss);
    slider->setPageStep(ps);
    slider->setTickInterval(ps);
}

/*******************************************************************************
 *  Slot function. Translates command object recevied from the parser thread
 *  to appropriate function call.
 ******************************************************************************/
void DialogBox::executeCommand(DialogCommand command)
{
    QWidget *widget = nullptr;

    if (empty) {
        clearDialog();
        empty = false;
    }
    switch (command.command & CommandMask) {
    case AddCommand:
        if (command.command & OptionSpace & OptionMask) {
            // Seems sscanf %d in some versions of standard C library has a bug
            // returning 32k on sero-size strings
            if (command.getText()[0]) {
                int size;
                sscanf(command.getText(), "%d", &size);
                addSpace(size);
            } else {
                addSpace();
            }
            break;
        }

        if (command.command & OptionStretch & OptionMask) {
            addStretch();
            break;
        }

        switch (command.control & ~PropertyMask) {
        case LabelWidget:
            addLabel(command.getTitle(), command.getName(),
                     command.control & PropertyPicture & PropertyMask
                     ? PixmapContent
                     : command.control & PropertyAnimation & PropertyMask
                     ? MovieContent : TextContent);
            break;
        case GroupBoxWidget:
            addGroupBox(command.getTitle(), command.getName(),
                        command.control & PropertyVertical & PropertyMask,
                        command.control & PropertyCheckable & PropertyMask,
                        command.control & PropertyChecked & PropertyMask);
            break;
        case FrameWidget:
            addFrame(command.getTitle(),
                     command.control & PropertyVertical & PropertyMask,
                     command.control);
            break;
        case PushButtonWidget:
            addPushButton(command.getTitle(), command.getName(),
                          command.control & PropertyApply & PropertyMask,
                          command.control & PropertyExit & PropertyMask,
                          command.control & PropertyDefault & PropertyMask);
            break;
        case CheckBoxWidget:
            addCheckBox(command.getTitle(), command.getName(),
                        command.control & PropertyChecked & PropertyMask);
            break;
        case RadioButtonWidget:
            addRadioButton(command.getTitle(), command.getName(),
                           command.control & PropertyChecked & PropertyMask);
            break;
        case TextBoxWidget:
            addTextBox(command.getTitle(), command.getName(), command.getText(),
                       command.getAuxText(),
                       command.control & PropertyPassword & PropertyMask);
            break;
        case ListBoxWidget:
            addListBox(command.getTitle(), command.getName(),
                       command.control & PropertyActivation & PropertyMask,
                       command.control & PropertySelection & PropertyMask);
            break;
        case ComboBoxWidget:
            addComboBox(command.getTitle(), command.getName(),
                        command.control & PropertyEditable & PropertyMask,
                        command.control & PropertySelection & PropertyMask);
            break;
        case ItemWidget:
            addItem(command.getTitle(), command.getName(),
                    command.control & PropertyCurrent & PropertyMask);
            break;
        case SeparatorWidget:
            addSeparator(command.getTitle(),
                         command.control & PropertyVertical & PropertyMask,
                         command.control);
            break;
        case ProgressBarWidget:
            addProgressBar(command.getTitle(),
                           command.control & PropertyVertical & PropertyMask,
                           command.control & PropertyBusy & PropertyMask);
            break;
        case SliderWidget: {
            int min = 0;
            int max = 100;

            if (command.getName()[0])
                sscanf(command.getName(), "%d", &min);
            if (command.getText()[0])
                sscanf(command.getText(), "%d", &max);
            addSlider(command.getTitle(),
                      command.control & PropertyVertical & PropertyMask,
                      min, max);
            break;
        }
        case TextViewWidget:
            addTextView(command.getTitle(), command.getName());
            break;
        case TabsWidget:
            addTabs(command.getTitle(), command.control);
            break;
        case PageWidget:
            addPage(command.getTitle(), command.getName(), command.getText(),
                    command.control & PropertyCurrent & PropertyMask);
            break;
        }
        break;
    case ClearCommand:
        clear(command.getName());
        break;
    case EndCommand:
        switch (command.control & ~PropertyMask) {
        case GroupBoxWidget:
        case FrameWidget:
            endGroup();
            break;
        case ListBoxWidget:
        case ComboBoxWidget:
            endList();
            break;
        case TabsWidget:
            endTabs();
            break;
        case PageWidget:
            endPage();
            break;
        case WidgetMask:
            // None type mentioned
            if (currentView) {
                endList();
            } else if (groupLayout) {
                endGroup();
            } else if (currentTabsWidget) {
                if (currentTabsWidget->indexOf(currentLayout->parentWidget())
                    == -1) {
                    endTabs();
                } else {
                    endPage();
                }
            }
            break;
        }
        break;
    case StepCommand:
        if (command.command & OptionVertical & OptionMask)
            stepVertical();
        else
            stepHorizontal();
        break;
    case SetCommand:
        if (command.getName()[0]) {
            if (!(widget = findWidget(command.getName())))
                break;
        } else {
            widget = this;
        }

        if (command.command & OptionEnabled & OptionMask)
            setEnabled(widget, true);

        if (command.command & OptionFocus & OptionMask) {
            QTimer::singleShot(0, widget, SLOT(setFocus()));

            // Select text for QLineEdit objects.
            // selectedText property is not writable and this must be done in
            // the class specific way.
            if (QWidget *proxyWidget = widget->focusProxy()) {
                switch (widgetType(proxyWidget)) {
                case ComboBoxWidget:
                    proxyWidget = ((QComboBox *)proxyWidget)->lineEdit();
                    break;
                case TextBoxWidget:
                    break;
                default:
                    proxyWidget = nullptr;
                    break;
                }
                if (proxyWidget)
                    ((QLineEdit *)proxyWidget)->selectAll();
            }
        }

        // See http://doc.qt.io/qt-4.8/stylesheet.html for reference
        if (command.command & OptionStyleSheet & OptionMask) {
            widget->setStyleSheet(command.getText());
            if (QWidget *proxyWidget = widget->focusProxy())
                proxyWidget->setStyleSheet(command.getText());
        }

        if (command.command & OptionVisible & OptionMask) {
            widget->show();
            if (QWidget *proxyWidget = widget->focusProxy())
                proxyWidget->show();
        }

        if (command.control) {
            setOptions(widget, command.control, command.control,
                       command.getText());
        }

        // Setting of some properties (calls like show and hide) generates
        // events which are optimised next or sets widget attributes which might
        // impact next calls. To avoid races and to ensure the command is
        // executed as expected we process all events that have been generated:
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents
                                        | QEventLoop::ExcludeSocketNotifiers);
        break;
    case UnsetCommand:
        if (command.getName()[0]) {
            if (!(widget = findWidget(command.getName())))
                break;
        } else {
            widget = this;
        }

        if (command.command & OptionEnabled & OptionMask)
            setEnabled(widget, false);

        // See http://doc.qt.io/qt-4.8/stylesheet.html for reference
        if (command.command & OptionStyleSheet & OptionMask) {
            // Rarely it was seen this fails (unset stylesheet or set it to
            // empty string). Hopefully this was caused by the race which is now
            // fixed (queued signaling between threads and optimisation of
            // queued GUI events).
            widget->setStyleSheet(QString());
            if (QWidget *proxywidget = widget->focusProxy())
                proxywidget->setStyleSheet(QString());
        }

        if (command.command & OptionVisible & OptionMask) {
            widget->hide();
            if (QWidget *proxywidget = widget->focusProxy())
                proxywidget->hide();
        }

        if (command.control)
            setOptions(widget, 0, command.control, nullptr);

        // Setting of some properties (calls like show and hide) generates
        // events which are optimised next or sets widget attributes which might
        // impact next calls. To avoid races and to ensure the command is
        // executed as expected we process all events that have been generated:
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents
                                        | QEventLoop::ExcludeSocketNotifiers);
        break;
    case RemoveCommand:
        removeWidget(command.getName());
        break;
    case PositionCommand:
        position(command.getText(), command.command & OptionBehind & OptionMask,
                 command.command & OptionOnto & OptionMask);
        break;
    case QueryCommand:
        report();
        break;
    case NoopCommand:
    default:
        break;
    }
    // Clean up after possible findWidget call
    chosenView = nullptr;
    chosenRowFlag = false;
}
