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
 *
 *  PRIVATE FUNCTIONS
 *
 ******************************************************************************/

/*******************************************************************************
 *  updateTabsOrder sets widgets focus order in way they are shown on the
 *  dialog, not they are created.
 *  It walks through the given or current (the currentLayout is on) page only.
 ******************************************************************************/
void DialogBox::updateTabsOrder(QWidget *page)
{
    QBoxLayout *mainLayout = (QBoxLayout *)(page ? page->layout() :
                                            currentLayout->parent()->parent());
    QWidget *prevWidget = nullptr;
    QWidget *widget;

    // All widgets are chained. Their focus policies define the focus move.
    for (int i = 0, j = mainLayout->count(); i < j; i++) {
        QHBoxLayout *hLayout = (QHBoxLayout *)mainLayout->itemAt(i)->layout();
        for (int i = 0, j = hLayout->count(); i < j; i++) {
            QVBoxLayout *vLayout = (QVBoxLayout *)hLayout->itemAt(i)->layout();
            for (int i = 0, j = vLayout->count(); i < j; i++) {
                QLayout *layout;
                QLayoutItem *li = vLayout->itemAt(i);

                if (!(widget = li->widget())) {
                    if ((layout = li->layout())) {
                        // For joint objects
                        widget = layout->itemAt(1)->widget();
                    }
                }

                if (widget) {
                    if (widget->focusPolicy() != Qt::NoFocus) {
                        if (prevWidget)
                            setTabOrder(prevWidget, widget);
                        prevWidget = widget;
                    }

                    if (QBoxLayout *gLayout = (QBoxLayout *)widget->layout()) {
                        // For QGroupBox/QFrame objects
                        for (int i = 0, j = gLayout->count(); i < j; i++) {
                            QLayout *layout;
                            QLayoutItem *li = gLayout->itemAt(i);

                            if (!(widget = li->widget())) {
                                if ((layout = li->layout()))
                                    widget = layout->itemAt(1)->widget();
                            }

                            if (widget
                                && widget->focusPolicy() != Qt::NoFocus) {
                                if (prevWidget)
                                    setTabOrder(prevWidget, widget);
                                prevWidget = widget;
                            }
                        }
                    }
                }
            }
        }
    }
}

/*******************************************************************************
 *  The layout is empty if it is not the current one, has no child widgets and
 *  the same is true for all its downlinks.
 ******************************************************************************/
bool DialogBox::isEmpty(QLayout *layout)
{
    // Note:
    // layout->isEmpty() returns true even if it contains labels or groupboxes

    if (!layout || layout == currentLayout)
        return false;
    for (int i = 0, j = layout->count(); i < j; i++) {
        QLayoutItem *li;
        QLayout *child;

        if ((li = layout->itemAt(i))->widget())
            return false;
        if ((child = li->layout()) && !isEmpty(child))
            return false;
    }
    return true;
}

/*******************************************************************************
 *  removeIfEmpty removes branch of empty layouts from the tree.
 *  It removes layout (with its downlinks) if it:
 *    - has no widgets
 *    - is not installed on a widget (groupbox in particular)
 *    - is not the current one
 *    - is not the last one
 ******************************************************************************/
bool DialogBox::removeIfEmpty(QLayout *layout)
{
    QLayout *parent;

    if (isEmpty(layout)
        && (!(parent = (QLayout *)layout->parent())->isWidgetType())
        && (parent->count() > 1 || (!parent->parent()->isWidgetType()
        && ((QLayout *)parent->parent())->count() > 1))) {
        if (!removeIfEmpty(parent)) {
            parent->removeItem(layout);
            delete layout;
        }
        return true;
    }
    return false;
}

/*******************************************************************************
 *  sanitizeLayout prevents fantom layouts. Must be called for end layouts ONLY
 *  (3rd or 4th level). Actually for the 4th level layout it removes spacer
 *  items only.
 ******************************************************************************/
void DialogBox::sanitizeLayout(QLayout *layout)
{
    if (isEmpty(layout) && layout->count()) {
        // Remove all QSpacerItem items
        while (QLayoutItem *li = layout->takeAt(0))
            delete li;
    }

    removeIfEmpty(layout);
}

/*******************************************************************************
 *  sanitizeLabel prepares label for changing its content type
 ******************************************************************************/
void DialogBox::sanitizeLabel(QWidget *label, enum ContentType content)
{
    if (widgetType(label) == LabelWidget) {
        QMovie *mv;
        QLayout *layout;

        if ((mv = ((QLabel *)label)->movie()))
            delete mv;
        ((QLabel *)label)->clear();

        if ((layout = findLayout(label))) {
            switch (content) {
            case PixmapContent:
                // Set widget alignment
                layout->setAlignment(label, GRAPHICS_ALIGNMENT);
                // Set layout alignment
                layout->setAlignment(GRAPHICS_ALIGNMENT);
                label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                // Reset to defaults
                ((QLabel *)label)->setTextInteractionFlags(
                        Qt::LinksAccessibleByMouse);
                ((QLabel *)label)->setOpenExternalLinks(false);
                ((QLabel *)label)->setWordWrap(false);
                break;
            case MovieContent:
                layout->setAlignment(label, GRAPHICS_ALIGNMENT);
                layout->setAlignment(GRAPHICS_ALIGNMENT);
                label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                // Reset to defaults
                ((QLabel *)label)->setTextInteractionFlags(
                        Qt::LinksAccessibleByMouse);
                ((QLabel *)label)->setOpenExternalLinks(false);
                ((QLabel *)label)->setWordWrap(false);
                break;
            case TextContent:
                // Layout default alignment for proper text displaying
                layout->setAlignment(label, DEFAULT_ALIGNMENT);
                layout->setAlignment(DEFAULT_ALIGNMENT);
                label->setSizePolicy(QSizePolicy::MinimumExpanding,
                                     QSizePolicy::Minimum);
                // Qt::TextBrowserInteraction includes:
                //    TextSelectableByMouse | LinksAccessibleByMouse
                //    | LinksAccessibleByKeyboard
                // To make the text label not interactive use stylesheets
                // (e.g. qproperty-textInteractionFlags: NoTextInteraction;).
                ((QLabel *)label)->setTextInteractionFlags(
                        Qt::TextBrowserInteraction);
                ((QLabel *)label)->setOpenExternalLinks(true);
                ((QLabel *)label)->setWordWrap(true);
                break;
            }
        }
    }
}

void DialogBox::printWidgetsRecursively(QLayoutItem *item)
{
    QLayout *layout;
    QWidget *widget;

    if ((layout = item->layout())) {
        for (int i = 0, j = layout->count(); i < j; i++)
            printWidgetsRecursively(layout->itemAt(i));
    } else if ((widget = item->widget())) {
        printWidget(widget);
        if ((layout = widget->layout()))
            printWidgetsRecursively(layout);
    }
}

void DialogBox::printWidget(QWidget *widget)
{
    if (widget->isEnabled()) {
        const char *name = widget->objectName().toLocal8Bit().constData();

        if (name[0]) {
            QWidget *proxyWidget;
            const QMetaObject *metaObj = widget->metaObject();
            int propertyIndex;

            if (metaObj->property(metaObj->indexOfProperty("checkable"))
                .read(widget).toBool()) {
                fprintf(output, "%s=%s\n", name,
                        metaObj->property(metaObj->indexOfProperty("checked"))
                        .read(widget).toBool() ? "1" : "0");
                fflush(output);
                return;
            }

            if ((propertyIndex = metaObj->indexOfProperty("value")) >= 0
                && widgetType(widget) != ProgressBarWidget) {
                fprintf(output, "%s=%d\n", name,
                        metaObj->property(propertyIndex).read(widget).toInt());
                fflush(output);
                return;
            }

            if ((proxyWidget = widget->focusProxy())) {
                QListWidgetItem *item;

                fprintf(output, "%s=", name);
                switch ((unsigned)widgetType(proxyWidget)) {
                case ComboBoxWidget:
                    fprintf(output, "%s\n",
                            ((QComboBox *)proxyWidget)->currentText()
                            .toLocal8Bit().constData());
                    break;
                case ListBoxWidget:
                    item = ((QListWidget *)proxyWidget)->currentItem();
                    fprintf(output, "%s\n",
                            item ? item->text().toLocal8Bit().constData() : "");
                    break;
                default:
                    metaObj = proxyWidget->metaObject();
                    fprintf(output, "%s\n",
                            metaObj->property(metaObj->indexOfProperty("text"))
                            .read(proxyWidget).toString().toLocal8Bit()
                            .constData());
                    break;
                }
                fflush(output);
                return;
            }
        }
    }
}
