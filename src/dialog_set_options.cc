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
 *  Sets options for the given widget.
 *  Options and mask are DialogCommandTokens::Control values.
 *  Mask indicates which bits to process when options indicates whether the
 *  option is to set or to unset.
 *  Text is the string used for options which require string values.
 ******************************************************************************/
void DialogBox::setOptions(QWidget *widget, unsigned int options,
                           unsigned int mask, const char *text)
{
    if (!widget)
        return;

    unsigned int type = widgetType(widget);

    options &= type | PropertyMask;
    mask &= type | PropertyMask;

    if (!(mask & type) || !(mask & PropertyMask))
        return;

    QWidget *proxyWidget;
    const QMetaObject *metaObj = widget->metaObject();
    const QMetaObject *proxyMetaObj;
    QMetaProperty property;

    if ( (proxyWidget = widget->focusProxy()) ) {
        proxyMetaObj = proxyWidget->metaObject();
    } else {
        proxyWidget = widget;
        proxyMetaObj = metaObj;
    }

    if (type & PropertyDefault && mask & PropertyDefault & PropertyMask) {
        property = metaObj->property(metaObj->indexOfProperty("default"));
        if (property.isWritable()) {
            property.write(widget,
                    QVariant((bool)(options & PropertyDefault & PropertyMask)));

            if (options & PropertyDefault & PropertyMask)
                defaultPushButton = (QPushButton *)widget;
            else if (defaultPushButton == (QPushButton *)widget)
                defaultPushButton = nullptr;
        }
    }

    if (type & PropertyChecked && mask & PropertyChecked & PropertyMask) {
        property = metaObj->property(metaObj->indexOfProperty("checked"));
        if (property.isWritable()) {
            property.write(widget,
                           QVariant(options & PropertyChecked & PropertyMask));
        }
    }

    if (type & PropertyCheckable && mask & PropertyCheckable & PropertyMask) {
        property = metaObj->property(metaObj->indexOfProperty("checkable"));
        if (property.isWritable()) {
            property.write(widget,
                    QVariant(options & PropertyCheckable & PropertyMask));
        }
        // Changing checkable status for some widgets might change their focus
        // policy to/from Qt::NoFocus (e.g. for QGroupBox)
        QWidget *page = widget->parentWidget();
        if (widgetType(page) != PageWidget)
            page = page->parentWidget();
        updateTabsOrder(page);
    }

    // Password makes sense for QLineEdit objects only
    if (type & PropertyPassword && mask & PropertyPassword & PropertyMask) {
        property = proxyMetaObj->property(
                proxyMetaObj->indexOfProperty("echoMode"));
        if (property.isWritable()) {
            property.write(proxyWidget,
                           QVariant(options & PropertyPassword & PropertyMask
                                    ? QLineEdit::Password
                                    : QLineEdit::Normal));
        }
    }

    // Placeholder makes sense for QLineEdit objects only
    if (type & PropertyPlaceholder
        && mask & PropertyPlaceholder & PropertyMask) {
        property = proxyMetaObj->property(
                proxyMetaObj->indexOfProperty("placeholderText"));
        if (property.isWritable()) {
            property.write(proxyWidget,
                           QVariant(QString(options & PropertyPlaceholder
                                            & PropertyMask ? text : nullptr)));
        }
    }

    // Text makes difference for QLineEdit objects only (editable combobox
    // includes QLineEdit object). For the rest it is the same as title.
    if (type & PropertyText && mask & PropertyText & PropertyMask) {
        QWidget *leWidget = proxyWidget;

        if (proxyWidget != widget
            && (type == TextBoxWidget || (type == ComboBoxWidget
            && (leWidget = ((QComboBox *)proxyWidget)->lineEdit())) )) {
            const QMetaObject *leMetaObj = leWidget->metaObject();
            property = leMetaObj->property(leMetaObj->indexOfProperty("text"));
            if (property.isWritable()) {
                property.write(leWidget,
                               QVariant(QString(options & PropertyText
                                            & PropertyMask ? text : nullptr)));
            }
        } else {
            // This is not a QLineEdit object
            if (PropertyTitle & type) {
                // This option will be applied by the code below
                mask |= PropertyTitle & PropertyMask;
                options |= options & PropertyText & PropertyMask
                           ? PropertyTitle & PropertyMask : 0;
            }
            // Else there is a bug in DialogCommandTokens::Control enum
        }
    }

    if (type & PropertyTitle && mask & PropertyTitle & PropertyMask) {
        switch (type) {
        case ItemWidget: {
            QAbstractItemModel *model = chosenView->model();
            if (chosenRow >= 0) {
                model->setData(model->index(chosenRow, 0),
                               QString(options & PropertyTitle & PropertyMask
                                       ? text : nullptr), Qt::DisplayRole);
            }
            break;
        }
        case PageWidget: {
            QTabWidget *tabs = (QTabWidget *)widget->parent()->parent();
            tabs->setTabText(tabs->indexOf(widget),
                             QString(options & PropertyTitle & PropertyMask
                                     ? text : nullptr));
            break;
        }
        default:
            if (// QGroupBox objects
                (property = metaObj->property(
                    metaObj->indexOfProperty("title"))).isWritable()
                // the rest widgets
                || (property = metaObj->property(
                    metaObj->indexOfProperty("text"))).isWritable()
                // the main window (QDialog object)
                || (property = metaObj->property(
                    metaObj->indexOfProperty("windowTitle"))).isWritable()) {
                // Avoid to format labels of joint widgets (which have
                // focusProxy set)
                if (widget == proxyWidget)
                    sanitizeLabel(widget, TextContent);

                property.write(widget,
                               QVariant(QString(options & PropertyTitle
                                                & PropertyMask
                                                ? text : nullptr)));
            }
        }
    }

    // Animation makes sense for QLabel objects only.
    // Also avoid to set animation for QLineEdit labels (which have focusProxy
    // set).
    // DialogCommandTokens::Control enum distinguishes between QLabel and label
    // of QLineEdit.
    if (type & PropertyAnimation && mask & PropertyAnimation & PropertyMask) {
        // There is no movie property for QLabel objects
        sanitizeLabel(widget, MovieContent);
        if (QMovie *mv = new QMovie(options & PropertyAnimation & PropertyMask
                                    ? text : nullptr)) {
            ((QLabel *)widget)->setMovie(mv);
            mv->start();
            mv->setParent(widget);
        }
    }

    // Picture makes sense for QLabel objects only.
    // Also avoid to set picture for QLineEdit labels (which have focusProxy
    // set).
    // DialogCommandTokens::Control enum distinguishes between QLabel and label
    // of QLineEdit.
    if (type & PropertyPicture && mask & PropertyPicture & PropertyMask) {
        property = metaObj->property(metaObj->indexOfProperty("pixmap"));
        if (property.isWritable()) {
            sanitizeLabel(widget, PixmapContent);
            property.write(widget,
                           QVariant(QPixmap(options & PropertyPicture
                                            & PropertyMask ? text : nullptr)));
        }
    }

    if (type & PropertyIcon && mask & PropertyIcon & PropertyMask) {
        switch (type) {
        case ItemWidget: {
            QAbstractItemModel *model = chosenView->model();
            if (chosenRow >= 0) {
                model->setData(model->index(chosenRow, 0),
                               QIcon(options & PropertyIcon & PropertyMask
                                     ? text : nullptr), Qt::DecorationRole);
            }
            break;
        }
        case PageWidget: {
            QTabWidget *tabs = (QTabWidget *)widget->parent()->parent();
            tabs->setTabIcon(tabs->indexOf(widget),
                             QIcon(options & PropertyIcon & PropertyMask
                                   ? text : nullptr));
            break;
        }
        default:
            if ((property = metaObj->property(
                    metaObj->indexOfProperty("icon"))).isWritable()
                || (property = metaObj->property(
                    metaObj->indexOfProperty("windowIcon"))).isWritable()) {
                property.write(widget,
                               QVariant(QIcon(options & PropertyIcon
                                              & PropertyMask
                                              ? text : nullptr)));
            }
        }
    }

    // Iconsize makes sense for set command only
    if (type & PropertyIconSize && mask & PropertyIconSize & PropertyMask
        && options & PropertyIconSize & PropertyMask) {
        property = proxyMetaObj->property(
                proxyMetaObj->indexOfProperty("iconSize"));
        if (property.isWritable()) {
            if (text[0]) {
                int size;
                sscanf(text, "%d", &size);
                property.write(proxyWidget, QVariant(QSize(size, size)));
            }
        }
    }

    // below three shadow options make sense for set command only
    if (type & PropertyRaised && mask & PropertyRaised & PropertyMask
        && options & PropertyRaised & PropertyMask) {
        property = metaObj->property(metaObj->indexOfProperty("frameShadow"));
        if (property.isWritable())
            property.write(widget, QVariant(QFrame::Raised));
    }

    if (type & PropertySunken && mask & PropertySunken & PropertyMask
        && options & PropertySunken & PropertyMask) {
        property = metaObj->property(metaObj->indexOfProperty("frameShadow"));
        if (property.isWritable())
            property.write(widget, QVariant(QFrame::Sunken));
    }

    if (type & PropertyPlain && mask & PropertyPlain & PropertyMask
        && options & PropertyPlain & PropertyMask) {
        property = metaObj->property(metaObj->indexOfProperty("frameShadow"));
        if (property.isWritable())
            property.write(widget, QVariant(QFrame::Plain));
    }

    if (type & PropertyBox && mask & PropertyBox & PropertyMask) {
        property = metaObj->property(metaObj->indexOfProperty("frameShape"));
        if (property.isWritable()) {
            property.write(widget, QVariant(options & PropertyBox & PropertyMask
                                            ? QFrame::Box : QFrame::NoFrame));
        }
    }

    if (type & PropertyPanel && mask & PropertyPanel & PropertyMask) {
        property = metaObj->property(metaObj->indexOfProperty("frameShape"));
        if (property.isWritable()) {
            property.write(widget,
                           QVariant(options & PropertyPanel & PropertyMask
                                    ? QFrame::Panel : QFrame::NoFrame));
        }
    }

    if (type & PropertyStyled && mask & PropertyStyled & PropertyMask) {
        property = metaObj->property(metaObj->indexOfProperty("frameShape"));
        if (property.isWritable()) {
            property.write(widget,
                           QVariant(options & PropertyStyled & PropertyMask
                                    ? QFrame::StyledPanel : QFrame::NoFrame));
        }
    }

    // Noframe shape option makes sense for set command only
    if (type & PropertyNoframe && mask & PropertyNoframe & PropertyMask
        && options & PropertyNoframe & PropertyMask) {
        property = metaObj->property(metaObj->indexOfProperty("frameShape"));
        if (property.isWritable())
            property.write(widget, QVariant(QFrame::NoFrame));
    }

    // Apply and exit make sense for QPushButton objects only
    if (type & PropertyApply & PropertyExit
        && (mask & PropertyApply & PropertyMask
        || mask & PropertyExit & PropertyMask)) {
        const unsigned int propertyApply = PropertyApply & PropertyMask;
        const unsigned int propertyExit = PropertyExit & PropertyMask;
        unsigned int pbOptions = 0;
        unsigned int lOptions = options & (propertyApply | propertyExit);
        unsigned int lMask = mask & (propertyApply | propertyExit);

        if (disconnect(widget, SIGNAL(clicked()), this, SLOT(report()))) {
            pbOptions |= propertyApply;
            if (disconnect(widget, SIGNAL(clicked()), this, SLOT(accept())))
                pbOptions |= propertyExit;
        }

        if (disconnect(widget, SIGNAL(clicked()), this, SLOT(reject())))
            pbOptions |= propertyExit;

        pbOptions &= (~lMask) | lOptions; // Reset 1s where needed
        pbOptions |= lMask & lOptions;    // Set 1s where needed

        if (pbOptions & propertyApply) {
            connect(widget, SIGNAL(clicked()), this, SLOT(report()));
            if (pbOptions & propertyExit)
                connect(widget, SIGNAL(clicked()), this, SLOT(accept()));
        } else if (pbOptions & propertyExit) {
            connect(widget, SIGNAL(clicked()), this, SLOT(reject()));
        }
    }

    if (type & PropertyActivation && mask & PropertyActivation & PropertyMask) {
        disconnect(proxyWidget, SIGNAL(activated(const QModelIndex &)), this,
                   SLOT(listBoxItemActivated(const QModelIndex &)));
        ((ListBox *)proxyWidget)->setActivateFlag(false);
        if (options & PropertyActivation & PropertyMask) {
            connect(proxyWidget, SIGNAL(activated(const QModelIndex &)), this,
                    SLOT(listBoxItemActivated(const QModelIndex &)));
            ((ListBox *)proxyWidget)->setActivateFlag(true);
        }
    }

    if (type & PropertySelection && mask & PropertySelection & PropertyMask) {
        switch (type) {
        case ComboBoxWidget:
            // Note: Qt::QueuedConnection type is used for this type widget
            disconnect(proxyWidget, SIGNAL(currentIndexChanged(int)), this,
                       SLOT(comboBoxItemSelected(int)));
            if (options & PropertySelection & PropertyMask) {
                connect(proxyWidget, SIGNAL(currentIndexChanged(int)), this,
                        SLOT(comboBoxItemSelected(int)), Qt::QueuedConnection);
            }
            break;
        case ListBoxWidget:
            disconnect(proxyWidget,
                       SIGNAL(currentItemChanged(QListWidgetItem *,
                                                 QListWidgetItem *)),
                       this, SLOT(listBoxItemSelected(QListWidgetItem *)));
            if (options & PropertySelection & PropertyMask) {
                connect(proxyWidget,
                        SIGNAL(currentItemChanged(QListWidgetItem *,
                                                  QListWidgetItem *)),
                        this, SLOT(listBoxItemSelected(QListWidgetItem *)));
            }
            break;
        }
    }

    // Current option makes sense for set command only
    if (type & PropertyCurrent && mask & PropertyCurrent & PropertyMask
        && options & PropertyCurrent & PropertyMask) {
        switch (type) {
        case ItemWidget:
            if (chosenRow >= 0) {
                if (chosenView != (ListBox *)chosenListWidget) {
                    // QComboBox widget
                    ((QComboBox *)chosenListWidget)->setCurrentIndex(chosenRow);
                } else {
                    // ListBox widget
                    chosenView->setCurrentIndex(
                            chosenView->model()->index(chosenRow, 0));
                }
            }
            break;
        case PageWidget:
            ((QTabWidget *)widget->parent()->parent())->setCurrentWidget(
                    widget);
            break;
        }
    }

    if (type & PropertyMinimum && mask & PropertyMinimum & PropertyMask) {
        property = metaObj->property(metaObj->indexOfProperty("minimum"));
        if (property.isWritable()) {
            int min = 0;

            if (options & PropertyMinimum & PropertyMask && text[0])
                sscanf(text, "%d", &min);
            property.write(widget, QVariant(min));
        }
    }

    if (type & PropertyMaximum && mask & PropertyMaximum & PropertyMask) {
        property = metaObj->property(metaObj->indexOfProperty("maximum"));
        if (property.isWritable()) {
            int max = 100;

            if (options & PropertyMaximum & PropertyMask && text[0])
                sscanf(text, "%d", &max);
            property.write(widget, QVariant(max));
        }
    }

    // reset() for QProgressBar objects must be done in the class specific way
    if (type & PropertyValue && mask & PropertyValue & PropertyMask) {
        property = metaObj->property(metaObj->indexOfProperty("value"));
        if (property.isWritable()) {
            if (!(options & PropertyValue & PropertyMask)
                && type == ProgressBarWidget) {
                ((QProgressBar *)widget)->reset();
            } else {
                int value = 0;
                if (text[0])
                    sscanf(text, "%d", &value);
                property.write(widget, QVariant(value));
            }
        }
    }

    // Busy makes sense for QProgressBar objects only
    if (type & PropertyBusy && mask & PropertyBusy & PropertyMask) {
        property = metaObj->property(metaObj->indexOfProperty("maximum"));
        if (property.isWritable()) {
            property.write(widget,
                           QVariant(options & PropertyBusy & PropertyMask
                                    ? 0 : 100));
        }
    }

    // File makes sense for QTextEdit objects only
    if (type & PropertyFile && mask & PropertyFile & PropertyMask) {
        if (options & PropertyFile & PropertyMask) {
            QFile txt(text);
            if (txt.open(QFile::ReadOnly))
                ((QTextEdit *)widget)->setText(QTextStream(&txt).readAll());
        } else {
            ((QTextEdit *)widget)->clear();
        }
    }

    // Below four position options make sense for set command only and for
    // QTabWidget objects only
    if (type & PropertyPositionTop && mask & PropertyPositionTop & PropertyMask
        && options & PropertyPositionTop & PropertyMask) {
        property = metaObj->property(metaObj->indexOfProperty("tabPosition"));
        if (property.isWritable())
            property.write(widget, QVariant(QTabWidget::North));
    }

    if (type & PropertyPositionBottom
        && mask & PropertyPositionBottom & PropertyMask
        && options & PropertyPositionBottom & PropertyMask) {
        property = metaObj->property(metaObj->indexOfProperty("tabPosition"));
        if (property.isWritable())
            property.write(widget, QVariant(QTabWidget::South));
    }

    if (type & PropertyPositionLeft
        && mask & PropertyPositionLeft & PropertyMask
        && options & PropertyPositionLeft & PropertyMask) {
        property = metaObj->property(metaObj->indexOfProperty("tabPosition"));
        if (property.isWritable())
            property.write(widget, QVariant(QTabWidget::West));
    }

    if (type & PropertyPositionRight
        && mask & PropertyPositionRight & PropertyMask
        && options & PropertyPositionRight & PropertyMask) {
        property = metaObj->property(metaObj->indexOfProperty("tabPosition"));
        if (property.isWritable())
            property.write(widget, QVariant(QTabWidget::East));
    }
}
