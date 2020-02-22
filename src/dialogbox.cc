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

static const char *aboutLabel = "_dbabout_";

/*******************************************************************************
 *  DialogBox constructor
 ******************************************************************************/
DialogBox::DialogBox(const char *title, const char *about, bool resizable,
                     FILE *out):
    defaultPushButton(nullptr),
    currentLayout(new QVBoxLayout),
    currentIndex(0),
    groupLayout(nullptr),
    currentView(nullptr),
    currentListWidget(nullptr),
    currentTabsWidget(nullptr),
    output(out),
    empty(true)
{

    QVBoxLayout *mainLayout = new QVBoxLayout;
    QHBoxLayout *hl = new QHBoxLayout;

    setLayout(mainLayout);
    mainLayout->addLayout(hl);
    hl->addLayout(currentLayout);

    if (!resizable)
        mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    else
        setSizeGripEnabled(true);

    mainLayout->setAlignment(LAYOUTS_ALIGNMENT);
    currentLayout->setAlignment(LAYOUTS_ALIGNMENT);

    pages.append(this);

    setWindowTitle(title);
    if (about)
        addLabel(about, aboutLabel);
}

/*******************************************************************************
 *  Widget management methods
 ******************************************************************************/

void DialogBox::addPushButton(const char *title, const char *name, bool apply,
                              bool exit, bool def)
{
    QPushButton *pb = new QPushButton(title);

    pb->setObjectName(QString(name));

    if (groupLayout)
        groupLayout->insertWidget(groupIndex++, pb);
    else
        currentLayout->insertWidget(currentIndex++, pb);

    connect(pb, SIGNAL(clicked()), this, SLOT(pushButtonClicked()));
    connect(pb, SIGNAL(toggled(bool)), this, SLOT(pushButtonToggled(bool)));
    if (apply) {
        connect(pb, SIGNAL(clicked()), this, SLOT(report()));
        if (exit)
            connect(pb, SIGNAL(clicked()), this, SLOT(accept()));
    } else {
        if (exit)
            connect(pb, SIGNAL(clicked()), this, SLOT(reject()));
    }

    if (def) {
        pb->setDefault(true);
        defaultPushButton = pb;
    }

    updateTabsOrder();
}

void DialogBox::addCheckBox(const char *title, const char *name, bool checked)
{
    QCheckBox *cb = new QCheckBox(title);

    cb->setObjectName(QString(name));
    cb->setChecked(checked);

    if (groupLayout)
        groupLayout->insertWidget(groupIndex++, cb);
    else
        currentLayout->insertWidget(currentIndex++, cb);

    updateTabsOrder();
}

void DialogBox::addRadioButton(const char *title, const char *name,
                               bool checked)
{
    QRadioButton *rb = new QRadioButton(title);

    rb->setObjectName(QString(name));
    rb->setChecked(checked);

    if (groupLayout)
        groupLayout->insertWidget(groupIndex++, rb);
    else
        currentLayout->insertWidget(currentIndex++, rb);

    updateTabsOrder();
}

void DialogBox::addLabel(const char *title, const char *name,
                         enum ContentType content)
{
    QLabel *lb = new QLabel;

    lb->setObjectName(QString(name));

    if (groupLayout)
        groupLayout->insertWidget(groupIndex++, lb);
    else
        currentLayout->insertWidget(currentIndex++, lb);

    sanitizeLabel(lb, content);

    switch (content) {
    case PixmapContent:
        lb->setPixmap(QPixmap(title)); // QLabel copies QPixmap object
        break;
    case MovieContent: {
        QMovie *mv = new QMovie(title);

        lb->setMovie(mv); // QLabel stores pointer to QMovie object
        mv->setParent(lb);
        mv->start();
        break;
    }
    default:
        lb->setText(title); // QLabel copies QString object
        break;
    }
}

void DialogBox::addGroupBox(const char *title, const char *name, bool vertical,
                            bool checkable, bool checked)
{
    QGroupBox *gb = new QGroupBox(title);

    gb->setObjectName(QString(name));
    gb->setCheckable(checkable);
    gb->setChecked(checked);

    groupLayout = (vertical ? (QBoxLayout *)new QVBoxLayout
                  : (QBoxLayout *)new QHBoxLayout);
    groupIndex = 0;
    gb->setLayout(groupLayout);
    groupLayout->setAlignment(LAYOUTS_ALIGNMENT);
    currentLayout->insertWidget(currentIndex++, gb);

    updateTabsOrder();
}

void DialogBox::addFrame(const char *name, bool vertical, unsigned int style)
{
    unsigned int shape, shadow;
    QFrame *frame = new QFrame;

    frame->setObjectName(QString(name));

    // Style is a DialogCommandTokens::Control value
    style &= PropertyMask;
    // Reset this bit instead of masking another 7 bits
    style &= ~PropertyVertical;
    if (style) {
        shape = style & PropertyBox ? QFrame::Box
                : style & PropertyPanel ? QFrame::Panel
                : style & PropertyStyled ? QFrame::StyledPanel
                : QFrame::NoFrame;
        shadow = style & PropertyRaised ? QFrame::Raised
                 : style & PropertySunken ? QFrame::Sunken
                 : QFrame::Plain;
        frame->setFrameStyle(shape | shadow);
    }

    groupLayout = vertical ? (QBoxLayout *)new QVBoxLayout
                  : (QBoxLayout *)new QHBoxLayout;
    groupIndex = 0;
    frame->setLayout(groupLayout);
    groupLayout->setAlignment(LAYOUTS_ALIGNMENT);
    currentLayout->insertWidget(currentIndex++, frame);
}

void DialogBox::addTextBox(const char *title, const char *name,
                           const char *text, const char *placeholder,
                           bool password)
{
    QHBoxLayout *box = new QHBoxLayout;
    QLabel *label = new QLabel(title);
    QLineEdit *edit = new QLineEdit(text);

    edit->setPlaceholderText(placeholder);
    edit->setEchoMode(password ? QLineEdit::Password : QLineEdit::Normal);

    label->setObjectName(name);
    label->setBuddy(edit);
    label->setFocusProxy(edit);

    box->addWidget(label);
    box->addWidget(edit);

    if (groupLayout)
        ((QBoxLayout *)groupLayout)->insertLayout(groupIndex++, box);
    else
        ((QBoxLayout *)currentLayout)->insertLayout(currentIndex++, box);

    updateTabsOrder();
}

void DialogBox::addListBox(const char *title, const char *name, bool activation,
                           bool selection)
{
    QVBoxLayout *box = new QVBoxLayout;
    QLabel *label = new QLabel(title);
    ListBox *list = new ListBox;

    label->setObjectName(QString(name));
    label->setBuddy(list);
    label->setFocusProxy(list);

    box->addWidget(label);
    box->addWidget(list);

    currentListWidget = currentView = list;
    viewIndex = 0;

    if (groupLayout)
        groupLayout->insertLayout(groupIndex++, box);
    else
        currentLayout->insertLayout(currentIndex++, box);

    if (activation) {
        connect(list, SIGNAL(activated(const QModelIndex &)), this,
                SLOT(listBoxItemActivated(const QModelIndex &)));
        list->setActivateFlag(true);
    }

    if (selection) {
        connect(list,
            SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
            this, SLOT(listBoxItemSelected(QListWidgetItem *)));
    }

    updateTabsOrder();
}

void DialogBox::addComboBox(const char *title, const char *name, bool editable,
                            bool selection)
{
    QHBoxLayout *box = new QHBoxLayout;
    QLabel *label = new QLabel(title);
    QComboBox *list = new QComboBox;

    label->setObjectName(QString(name));
    label->setBuddy(list);
    label->setFocusProxy(list);

    list->setEditable(editable);

    // Prevent insertions as there is no way to report them. Instead an apply
    // pushbutton must be used to retrieve and process current values.
    list->setInsertPolicy(QComboBox::NoInsert);

    box->addWidget(label);
    box->addWidget(list);

    currentView = list->view();
    currentListWidget = list;
    viewIndex = 0;

    if (groupLayout)
        groupLayout->insertLayout(groupIndex++, box);
    else
        currentLayout->insertLayout(currentIndex++, box);

    // Qt::QueuedConnection is used for this type widget to fix the bug when
    // the first added item made current and is reported as an emty one
    if (selection) {
        connect(list, SIGNAL(currentIndexChanged(int)), this,
                SLOT(comboBoxItemSelected(int)), Qt::QueuedConnection);
    }

    updateTabsOrder();
}

void DialogBox::addItem(const char *title, const char *icon, bool current)
{
    // It seems the insertRows call triggers signals (e.g.
    // rowsAboutToBeInserted, rowsInserted or so) which are queued or cause
    // delays in some other way for widgets' currentIndexChanged and
    // currentItemChanged signals (probably for setData calls...).
    // That is why QComboBox object's index update is queued/delayed and we have
    // to use Qt::QueuedConnection for connections for objects of this type.

    if (currentView) {
        QAbstractItemModel *model = currentView->model();

        model->insertRows(viewIndex, 1);
        model->setData(model->index(viewIndex, 0), QIcon(icon),
                       Qt::DecorationRole);
        model->setData(model->index(viewIndex, 0), QString(title),
                       Qt::DisplayRole);
        if (current || model->rowCount() == 1) {
            if (widgetType(currentListWidget) == ComboBoxWidget)
                ((QComboBox *)currentListWidget)->setCurrentIndex(viewIndex);
            else
                currentView->setCurrentIndex(model->index(viewIndex, 0));
        }
        viewIndex++;
    }
}

void DialogBox::addSeparator(const char *name, bool vertical,
                             unsigned int style)
{
    unsigned int shadow = QFrame::Sunken;
    QFrame *separator = new QFrame;

    separator->setObjectName(name);

    if (vertical) {
        separator->setFrameShape(QFrame::VLine);
        separator->setSizePolicy(QSizePolicy::Fixed,
                                 QSizePolicy::MinimumExpanding);
    } else {
        separator->setFrameShape(QFrame::HLine);
        separator->setSizePolicy(QSizePolicy::MinimumExpanding,
                                 QSizePolicy::Fixed);
    }

    // Style is a DialogCommandTokens::Control value
    if (style & SeparatorWidget) {
        style &= PropertyMask;
        shadow = style & PropertyRaised ? QFrame::Raised
                 : style & PropertyPlain ? QFrame::Plain
                 : QFrame::Sunken;
    }

    separator->setFrameShadow(QFrame::Shadow(shadow));

    if (groupLayout)
        groupLayout->insertWidget(groupIndex++, separator);
    else
        currentLayout->insertWidget(currentIndex++, separator);
}

void DialogBox::addProgressBar(const char *name, bool vertical, bool busy)
{
    QProgressBar *pb = new QProgressBar;

    pb->setObjectName(QString(name));

    if (vertical) {
        pb->setOrientation(Qt::Vertical);
        pb->setTextDirection(QProgressBar::BottomToTop);
    }

    if (busy)
        pb->setRange(0, 0);
    else
        pb->setRange(0, 100);

    if (groupLayout)
        groupLayout->insertWidget(groupIndex++, pb);
    else
        currentLayout->insertWidget(currentIndex++, pb);
}

void DialogBox::addSlider(const char *name, bool vertical, int min, int max)
{
    QSlider *slider = new QSlider;

    slider->setObjectName(QString(name));
    if (!vertical)
        slider->setOrientation(Qt::Horizontal);
    slider->setTickPosition(QSlider::TicksAbove);
    slider->setTracking(false);
    connect(slider, SIGNAL(valueChanged(int)), this,
            SLOT(sliderValueChanged(int)));
    connect(slider, SIGNAL(rangeChanged(int, int)), this,
            SLOT(sliderRangeChanged(int, int)));

    slider->setRange(min, max);

    if (groupLayout)
        groupLayout->insertWidget(groupIndex++, slider);
    else
        currentLayout->insertWidget(currentIndex++, slider);

    updateTabsOrder();
}

void DialogBox::addTextView(const char *name, const char *file)
{
    QTextEdit *viewer = new QTextEdit;
    QFile txt(file);

    viewer->setObjectName(QString(name));
    viewer->setReadOnly(true);
    if (txt.open(QFile::ReadOnly))
        viewer->setText(QTextStream(&txt).readAll());

    // Below includes TextSelectableByMouse | LinksAccessibleByMouse
    // | LinksAccessibleByKeyboard.
    // This allows to select by mouse and copy text or link by context menu.
    // Qt::TextSelectableByKeyboard adds keyboard cursor which makes scrolling
    // inconvenient.
    viewer->setTextInteractionFlags(Qt::TextBrowserInteraction);

    if (groupLayout)
        groupLayout->insertWidget(groupIndex++, viewer);
    else
        currentLayout->insertWidget(currentIndex++, viewer);

    updateTabsOrder();
}

void DialogBox::addTabs(const char *name, unsigned int position)
{
    QTabWidget *tabs = new QTabWidget;

    tabs->setObjectName(QString(name));

    position &= PropertyMask;
    // Reset this bit instead of masking another 4 bits
    position &= ~PropertyIconSize;
    if (position) {
        tabs->setTabPosition(position & PropertyPositionTop
                             ? QTabWidget::North
                             : position & PropertyPositionBottom
                             ? QTabWidget::South
                             : position & PropertyPositionLeft
                             ? QTabWidget::West : QTabWidget::East);
    }

    currentTabsWidget = tabs;
    tabsIndex = 0;

    if (groupLayout)
        groupLayout->insertWidget(groupIndex++, tabs);
    else
        currentLayout->insertWidget(currentIndex++, tabs);

    updateTabsOrder();
}

void DialogBox::addPage(const char *title, const char *name, const char *icon,
                        bool current)
{
    if (currentTabsWidget) {
        endGroup();

        QWidget *page = new QWidget;

        page->setObjectName(QString(name));

        QVBoxLayout *ml = new QVBoxLayout;
        QHBoxLayout *hl = new QHBoxLayout;
        currentLayout = new QVBoxLayout;
        currentIndex = 0;

        page->setLayout(ml);
        ml->addLayout(hl);
        hl->addLayout(currentLayout);

        ml->setSizeConstraint(QLayout::SetFixedSize);
        ml->setAlignment(LAYOUTS_ALIGNMENT);
        currentLayout->setAlignment(LAYOUTS_ALIGNMENT);

        pages.append(page);

        connect(page, SIGNAL(destroyed(QObject *)), this,
                SLOT(removePage(QObject *)));

        currentTabsWidget->insertTab(tabsIndex, page, QIcon(icon),
                                     QString(title));
        if (current)
            currentTabsWidget->setCurrentIndex(tabsIndex);
        tabsIndex++;
    }
}

void DialogBox::endPage()
{
    QWidget *page = currentLayout->parentWidget();

    if (page != (QWidget *)this
        && currentTabsWidget == page->parent()->parent()) {
        QLayout *layout;
        QObject *parent;

        endGroup();
        layout = findLayout(currentTabsWidget);
        parent = layout->parent();
        if (parent->isWidgetType()) {
            groupLayout = (QBoxLayout *)layout;
            groupIndex = layout->indexOf(currentTabsWidget) + 1;
            currentLayout = (QBoxLayout *)findLayout((QWidget *)parent);
            currentIndex = currentLayout->indexOf((QWidget *)parent) + 1;
        } else {
            currentLayout = (QBoxLayout *)layout;
            currentIndex = layout->indexOf(currentTabsWidget) + 1;
        }
    }
}

void DialogBox::endTabs()
{
    endPage();

    QWidget *page = currentLayout->parentWidget();

    if (page != (QWidget *)this) {
        currentTabsWidget = (QTabWidget *)page->parent()->parent();
        tabsIndex = currentTabsWidget->count();
    } else {
        currentTabsWidget = nullptr;
    }
}

bool DialogBox::isLayoutOnPage(QWidget *page, QLayout *layout)
{
    if (!layout)
        return (false);

    QObject *parent = layout->parent();
    QWidget *parentWidget = parent->isWidgetType()
                            ? ((QWidget *)parent)->parentWidget()
                            : layout->parentWidget();

    if (parentWidget == page)
        return true;

    if (parentWidget == this)
        return false;

    return isLayoutOnPage(page,
            findLayout((QWidget *)parentWidget->parent()->parent()));
}

bool DialogBox::isWidgetOnPage(QWidget *page, QWidget *widget)
{
    if (!widget)
        return false;

    return isLayoutOnPage(page, findLayout(widget));
}

bool DialogBox::isLayoutOnContainer(QWidget *container, QLayout *layout)
{
    if (!layout)
        return false;

    QObject *parent = layout->parent();
    QWidget *page;

    if (parent->isWidgetType()) {
        if (parent == container)
            return true;

        page = ((QWidget *)parent)->parentWidget();
    } else {
        page = layout->parentWidget();
    }

    if (page == this)
        return false;

    return isLayoutOnContainer(container,
            findLayout((QWidget *)page->parent()->parent()));
}

bool DialogBox::isWidgetOnContainer(QWidget *container, QWidget *widget)
{
    if (!widget)
        return false;

    return isLayoutOnContainer(container, findLayout(widget));
}

static int indexOf(QLayout *layout)
{
    int i = -1;
    int j;

    if (QLayout *parent = (QLayout *)layout->parent()) {
        for (i = 0, j = parent->count(); i < j; i++) {
            if (parent->itemAt(i)->layout() == layout)
                return i;
        }
    }
    return i;
}

void DialogBox::stepHorizontal()
{
    QBoxLayout *oldLayout = currentLayout;
    QVBoxLayout *vBox = new QVBoxLayout;

    ((QBoxLayout *)currentLayout->parent())->insertLayout(
            indexOf(currentLayout) + 1, vBox);
    currentLayout = vBox;
    currentIndex = 0;
    currentLayout->setAlignment(LAYOUTS_ALIGNMENT);
    endGroup();
    sanitizeLayout(oldLayout);
}

void DialogBox::stepVertical()
{
    QBoxLayout *oldLayout = currentLayout;
    QVBoxLayout *vBox = new QVBoxLayout;
    QHBoxLayout *hBox = new QHBoxLayout;
    QBoxLayout *rootLayout = (QBoxLayout *)currentLayout->parent()->parent();

    rootLayout->insertLayout(indexOf((QLayout *)currentLayout->parent()) + 1,
                             hBox);
    hBox->addLayout(vBox);
    currentLayout = vBox;
    currentIndex = 0;
    currentLayout->setAlignment(LAYOUTS_ALIGNMENT);
    endGroup();
    sanitizeLayout(oldLayout);
}

void DialogBox::clear(char *name)
{
    QWidget *widget;

    if (name[0]) {
        if ( (widget = findWidget(name)) ) {
            switch ((unsigned)widgetType(widget)) {
            case ListBoxWidget:
            case ComboBoxWidget:
                clearChosenList();
                break;
            case PageWidget:
                clearPage(widget);
                break;
            case TabsWidget:
                clearTabs((QTabWidget *)widget);
                break;
            }
        }
    } else {
        clearDialog();
    }
}

void DialogBox::clearChosenList()
{
    if (chosenView && !chosenRowFlag) {
        QAbstractItemModel *model = chosenView->model();

        model->removeRows(0, model->rowCount());
        if (chosenView == currentView)
            viewIndex = model->rowCount();
    }
}

void DialogBox::clearTabs(QTabWidget *widget)
{
    for (int i = widget->count(); i > 0; i--) {
        QWidget *page = widget->widget(i - 1);

        while (isWidgetOnPage(page, currentTabsWidget))
            endTabs();
        if (isWidgetOnPage(page, currentListWidget))
            endList();
        if (isWidgetOnPage(page, defaultPushButton))
            defaultPushButton = nullptr;

        if (isLayoutOnPage(page, currentLayout)) {
            endGroup();
            QLayout *layout = findLayout(widget);
            QObject *parent = layout->parent();
            if (parent->isWidgetType()) {
                groupLayout = (QBoxLayout *)layout;
                groupIndex = layout->indexOf(widget) + 1;
                currentLayout = (QBoxLayout *)findLayout((QWidget *)parent);
                currentIndex = currentLayout->indexOf((QWidget *)parent) + 1;
            } else {
                currentLayout = (QBoxLayout *)layout;
                currentIndex = layout->indexOf(widget) + 1;
            }
        }

        delete page;
    }
}

void DialogBox::clearPage(QWidget *widget)
{
    QLayout *layout0;
    QLayout *layout1;
    QLayout *layout2;
    QLayout *layout3;

    layout0 = widget->layout(); // main (vertical one)

    while (isWidgetOnPage(widget, currentTabsWidget))
        endTabs();
    if (isWidgetOnPage(widget, currentListWidget))
        endList();
    if (isWidgetOnPage(widget, defaultPushButton))
        defaultPushButton = nullptr;

    if (isLayoutOnPage(widget, currentLayout)) {
        endGroup();
        currentLayout = (QBoxLayout *)layout0->itemAt(0)->layout()->itemAt(0)
                        ->layout();
        currentIndex = 0;
    }

    for (int i = layout0->count() - 1; i >= 0; i--) {
        layout1 = layout0->itemAt(i)->layout(); // Horizontal ones

        for (int j = layout1->count() - 1; j >= 0; j--) {
            layout2 = layout1->itemAt(j)->layout(); // Vertical ones

            QLayoutItem *li;
            QLayoutItem *wi;
            QWidget *w;

            while ( (li = layout2->takeAt(0)) ) {
                if ((layout3 = li->layout())) { // Joint widgets' layout
                    while ( (wi = layout3->takeAt(0)) ) {
                        if ( (w = wi->widget()) ) {
                            // QWidget isn't inherited by QWidgetItem and must
                            // be deleted separately
                            delete w;
                        }
                        delete wi;
                    }
                }
                if ( (w = li->widget()) ) {
                    // Container widget owns installed layout and its widgets
                    // and deletes them
                    delete w;
                }
                delete li;
            }
            if (i != 0 || j != 0)
                delete layout1->takeAt(j);
            // else that is the last one layout made the current one
            // (assigned to currentLayout)
        }
        if (i != 0)
            delete layout0->takeAt(i);
    }
}

void DialogBox::clearDialog()
{
    defaultPushButton = nullptr;
    currentView = nullptr;
    currentListWidget = nullptr;
    currentTabsWidget = nullptr;
    groupLayout = nullptr;
    currentLayout = (QBoxLayout *)layout()->itemAt(0)->layout()->itemAt(0)
                    ->layout();
    currentIndex = 0;

    while (pages.count() > 1)
        delete pages.takeLast();

    clearPage(this);
}

void DialogBox::removeWidget(char *name)
{
    QWidget *widget;

    if ( (widget = findWidget(name)) ) {
        int type = widgetType(widget);

        if (type == ItemWidget) {
            if (chosenRow >= 0) {
                if (chosenView == currentView && chosenRow < viewIndex)
                    viewIndex--;
                chosenView->model()->removeRows(chosenRow, 1);
            }
            chosenRowFlag = false;
        } else if (QLayout *layout = findLayout(widget)) {
            switch (type) {
            case TabsWidget:
                for (int i = 0, j = ((QTabWidget *)widget)->count(); i < j;
                     i++) {
                    QWidget *page = ((QTabWidget *)widget)->widget(i);

                    while (isWidgetOnPage(page, currentTabsWidget))
                        endTabs();
                    if (currentTabsWidget == widget)
                        endTabs();
                    if (isWidgetOnPage(page, currentListWidget))
                        endList();
                    if (isWidgetOnPage(page, defaultPushButton))
                        defaultPushButton = nullptr;
                    if (isLayoutOnPage(page, currentLayout)) {
                        // Position focus behind the parent QTabWidget
                        endGroup();
                        QObject *parent = layout->parent();
                        if (parent->isWidgetType()) {
                            groupLayout = (QBoxLayout *)layout;
                            groupIndex = layout->indexOf(widget);
                            currentLayout = (QBoxLayout *)findLayout(
                                    (QWidget *)parent);
                            currentIndex = currentLayout->indexOf(
                                    (QWidget *)parent) + 1;
                        } else {
                            currentLayout = (QBoxLayout *)layout;
                            currentIndex = layout->indexOf(widget);
                        }
                    }
                }
                break;
            case FrameWidget:
            case GroupBoxWidget:
                while (isWidgetOnContainer(widget, currentTabsWidget))
                    endTabs();
                if (isWidgetOnContainer(widget, currentListWidget))
                    endList();
                if (isWidgetOnContainer(widget, defaultPushButton))
                    defaultPushButton = nullptr;
                if (isLayoutOnContainer(widget, currentLayout)) {
                    endGroup();
                    currentLayout = (QBoxLayout *)layout;
                    currentIndex = layout->indexOf(widget);
                }
                if (groupLayout && widget->layout() == groupLayout)
                    endGroup();
                break;
            case ListBoxWidget:
            case ComboBoxWidget:
                if (currentView && chosenView == currentView)
                    endList();
                break;
            case PushButtonWidget:
                if ((QPushButton *)widget == defaultPushButton)
                    defaultPushButton = nullptr;
                break;
            }

            if (layout == groupLayout && layout->indexOf(widget) < groupIndex)
                groupIndex--;
            if (layout == currentLayout
                && layout->indexOf(widget) < currentIndex) {
                currentIndex--;
            }

            if (QWidget *proxywidget = widget->focusProxy()) {
                // For joint widgets
                layout->removeWidget(proxywidget);
                delete proxywidget;
            }

            layout->removeWidget(widget);
            delete widget;  // This also deletes child layouts and widgets
            // (parented to parentWidget by addWidget and addLayout) and so
            // forth. For QTabWidget it deletes child pages which triggers
            // removePage slot.

            sanitizeLayout(layout);
        } else {
            // This is page
            while (isWidgetOnPage(widget, currentTabsWidget))
                endTabs();
            if (isWidgetOnPage(widget, currentListWidget))
                endList();
            if (isWidgetOnPage(widget, defaultPushButton))
                defaultPushButton = nullptr;

            if (isLayoutOnPage(widget, currentLayout)) {
                // position focus behind the parent QTabWidget
                endGroup();

                QWidget *tabs = (QWidget *)widget->parent()->parent();
                layout = (QBoxLayout *)findLayout(tabs);
                QObject *parent = layout->parent();

                if (parent->isWidgetType()) {
                    groupLayout = (QBoxLayout *)layout;
                    groupIndex = layout->indexOf(tabs) + 1;
                    currentLayout = (QBoxLayout *)findLayout((QWidget *)parent);
                    currentIndex = currentLayout->indexOf((QWidget *)parent) + 1;
                } else {
                    currentLayout = (QBoxLayout *)layout;
                    currentIndex = layout->indexOf(tabs) + 1;
                }
            }
            delete widget;
        }
    }
}

void DialogBox::position(char *name, bool behind, bool onto)
{
    QWidget *widget;
    QBoxLayout *layout;
    bool tabs_set = false;

    if ( (widget = findWidget(name)) ) {
        if ( !(layout = (QBoxLayout *)findLayout(widget)) ) {
            // This is page.
            // widget != this isn't tested as root page has no name.
            currentTabsWidget = (QTabWidget *)widget->parent()->parent();
            tabsIndex = currentTabsWidget->indexOf(widget);
            if (behind) {
                tabsIndex++;
                behind = false;
            }
            if (onto) {
                endGroup();
                layout = (QBoxLayout *)widget->layout();
                layout = (QBoxLayout *)layout->itemAt(layout->count() - 1)
                         ->layout();
                currentLayout = (QBoxLayout *)layout
                                ->itemAt(layout->count() - 1)->layout();
                currentIndex = currentLayout->count();
                return;
            }
            widget = currentTabsWidget;
            layout = (QBoxLayout *)findLayout(widget);
            tabs_set = true;
        }

        if (layout) {
            QWidget *page;
            QObject *parent;
            int index;
            int type = widgetType(widget);

            if (widget->focusProxy() && !(type & TabsWidget)) {
                // Joint widget
                parent = layout->parent()->parent();
                index = indexOf(layout);
                layout = (QBoxLayout *)layout->parent();
            } else {
                parent = layout->parent();
                index = layout->indexOf(widget);
            }

            if (onto && type & (ListBoxWidget | ComboBoxWidget)) {
                currentView = chosenView;
                currentListWidget = chosenListWidget;
                viewIndex = currentView->model()->rowCount();
            }
            if (type & ItemWidget) {
                currentView = chosenView;
                currentListWidget = chosenListWidget;
                viewIndex = chosenRow >= 0 ? chosenRow
                            : currentView->model()->rowCount();
                if (behind) {
                    viewIndex++;
                    behind = false;
                }
            }

            if (parent->isWidgetType()) {
                // The widget is installed onto QGroupBox/QFrame
                page = ((QWidget *)parent)->parentWidget();

                groupLayout = layout;
                groupIndex = index;
                if (behind)
                    groupIndex++;

                layout = (QBoxLayout *)findLayout((QWidget *)parent);
                if (layout) {
                    currentLayout = layout;
                    currentIndex = layout->indexOf((QWidget *)parent) + 1;
                }
            } else {
                page = widget->parentWidget();
                endGroup();

                currentLayout = layout;
                currentIndex = index;
                if (behind)
                    currentIndex++;

                if (onto && (layout = (QBoxLayout *)widget->layout())) {
                    // Position onto QGroupBox/QFrame object
                    groupLayout = layout;
                    groupIndex = layout->count();
                }
            }

            if (!tabs_set) {
                if (onto && type & TabsWidget) {
                    currentTabsWidget = (QTabWidget *)widget;
                    tabsIndex = ((QTabWidget *)widget)->count();
                } else {
                    if (page == this) {
                        currentTabsWidget = nullptr;
                    } else {
                        currentTabsWidget = (QTabWidget *)page->parent()
                                            ->parent();
                        tabsIndex = currentTabsWidget->indexOf(page);
                    }
                }
            }
        }
    }
}

void DialogBox::setEnabled(QWidget *widget, bool enable)
{
    switch ((unsigned)widgetType(widget)) {
    case PageWidget: {
        QTabWidget *tabs = (QTabWidget *)widget->parent()->parent();
        tabs->setTabEnabled(tabs->indexOf(widget), enable);
        break;
    }
    default:
        widget->setEnabled(enable);
        if (QWidget *proxywidget = widget->focusProxy())
            proxywidget->setEnabled(enable);
    }
}

/*******************************************************************************
 *  findWidget searches for the widget with the given name. It does some
 *  analysis of the name to recognize list item references (#number and :text)
 *  and sets internal pointer to the QAbstractItemView and to that item.
 ******************************************************************************/
QWidget *DialogBox::findWidget(char *name)
{
    QWidget *widget = nullptr;

    chosenView = nullptr;
    chosenRowFlag = false;

    if (name) {
        int li_row = -1;
        char *li_name = nullptr;

        for (int i = 0; name[i] != 0; i++) {
            if (name[i] == '#') {
                name[i] = 0;
                if (name[i + 1])
                    sscanf(name + i + 1, "%d", &li_row);
                break;
            }
            if (name[i] == ':') {
                name[i] = 0;
                li_name = name + i + 1;
                break;
            }
        }

        if (name[0])
            for (int i = 0, j = pages.count(); i < j; i++) {
                widget = pages.at(i);
                if (!strcmp(widget->objectName().toLocal8Bit().constData(),
                            name)) {
                    // The widget is page
                    break;
                }
                if ( (widget = findWidgetRecursively(widget->layout(), name)) )
                    break;
            }

        switch ((unsigned)widgetType(widget)) {
        case ListBoxWidget:
            chosenListWidget = widget->focusProxy();
            chosenView = (ListBox *)chosenListWidget;
            break;
        case ComboBoxWidget:
            chosenListWidget = widget->focusProxy();
            chosenView = ((QComboBox *)chosenListWidget)->view();
            break;
        }
        if (chosenView) {
            if (li_row >= 0) {
                chosenRow = li_row;
                chosenRowFlag = true;
            }
            if (li_name) {
                QAbstractItemModel *model = chosenView->model();
                int i;
                int j;
                for (i = 0, j = model->rowCount(); i < j; i++) {
                    if (!strcmp(model->data(model->index(i, 0), Qt::DisplayRole)
                                .toString().toLocal8Bit().constData(),
                                li_name)) {
                        chosenRow = i;
                        chosenRowFlag = true;
                        break;
                    }
                }
                if (i == j) {
                    chosenRow = i;
                    chosenRowFlag = true;
                }
            }
        }
    }

    return widget;
}

/*******************************************************************************
 *  findLayout searches for the layout the given widget is laid on.
 ******************************************************************************/
QLayout *DialogBox::findLayout(QWidget *widget)
{
    QLayout *layout = nullptr;
    QWidget *page;

    for (int i = 0, j = pages.count(); i < j; i++) {
        if ( (page = pages.at(i)) == widget ) {
            // The widget is page
            break;
        }
        if ( (layout = findLayoutRecursively(page->layout(), widget)) )
            break;
    }

    return layout;
}

/*******************************************************************************
 *  Duplicate of void QDialogPrivate::hideDefault() which is made private.
 *  Is called by listbox widget with activation option set when it gets focus.
 ******************************************************************************/
void DialogBox::holdDefaultPushButton()
{
    QList<QPushButton *> list = findChildren<QPushButton *>();
    for (int i = 0; i < list.size(); i++)
        list.at(i)->setDefault(false);
}

/*******************************************************************************
 *  Is called by listbox widget with activation option set when it loses focus.
 *  Unfortunately QDialog keeps all properties/methods for default/autodefault
 *  pushbuttons management private. Don't see a graceful way to restore default
 *  indicator for an autodefault pushbutton...
 ******************************************************************************/
void DialogBox::unholdDefaultPushButton()
{
    if (defaultPushButton)
        defaultPushButton->setDefault(true);
}

/*******************************************************************************
 *  widgetType returns the type of the widget as value of
 *  DialogCommandTokens::Control enum
 ******************************************************************************/
DialogCommandTokens::Control DialogBox::widgetType(QWidget *widget)
{
    if (widget) {
        const char *name = widget->metaObject()->className();

        if (!strcmp(name, "DialogBox"))
            return DialogWidget;
        if (!strcmp(name, "QPushButton"))
            return PushButtonWidget;
        if (!strcmp(name, "QRadioButton"))
            return RadioButtonWidget;
        if (!strcmp(name, "QCheckBox"))
            return CheckBoxWidget;
        if (!strcmp(name, "QLabel")) {
            if (QWidget *proxywidget = widget->focusProxy()) {
                const char *proxyname = proxywidget->metaObject()->className();

                if (!strcmp(proxyname, "QLineEdit"))
                    return TextBoxWidget;
                if (!strcmp(proxyname, "QComboBox")) {
                    if (chosenRowFlag)
                        return ItemWidget;
                    return ComboBoxWidget;
                }
                if (!strcmp(proxyname, "ListBox")) {
                    if (chosenRowFlag)
                        return ItemWidget;
                    return ListBoxWidget;
                }
            }
            return LabelWidget;
        }
        if (!strcmp(name, "QGroupBox"))
            return GroupBoxWidget;
        if (!strcmp(name, "QFrame")) {
            int shape = ((QFrame *)widget)->frameStyle() & QFrame::Shape_Mask;

            if (shape == QFrame::HLine || shape == QFrame::VLine)
                return SeparatorWidget;
            return FrameWidget;
        }
        if (!strcmp(name, "QLineEdit")) {
            // If the object queried directly
            return TextBoxWidget;
        }
        if (!strcmp(name, "ListBox")) {
            // If the object queried directly
            if (chosenRowFlag)
                return ItemWidget;
            return ListBoxWidget;
        }
        if (!strcmp(name, "QComboBox")) {
            // If the object queried directly
            if (chosenRowFlag)
                return ItemWidget;
            return ComboBoxWidget;
        }

        if (!strcmp(name, "QTabWidget"))
            return TabsWidget;
        if (!strcmp(name, "QWidget"))
            return PageWidget;

        if (!strcmp(name, "QProgressBar"))
            return ProgressBarWidget;
        if (!strcmp(name, "QSlider"))
            return SliderWidget;
        if (!strcmp(name, "QTextEdit"))
            return TextViewWidget;
    }

    return NoneWidget;
}

/*******************************************************************************
 *
 *  ListBox class is introduced to prevent Enter key hit propagation to default
 *  pushbutton when listbox activation option is on or combobox widget is
 *  editable. In other words to prevent two controls to respond to a single key.
 *
 ******************************************************************************/

void ListBox::focusInEvent(QFocusEvent *event)
{
    if (activateFlag) {
        DialogBox *dialog = (DialogBox *)window();
        if (dialog)
            dialog->holdDefaultPushButton();
    }
    QListWidget::focusInEvent(event);
}

void ListBox::focusOutEvent(QFocusEvent *event)
{
    if (activateFlag) {
        DialogBox *dialog = (DialogBox *)window();
        if (dialog)
            dialog->unholdDefaultPushButton();
    }
    QListWidget::focusOutEvent(event);
}

void ListBox::setActivateFlag(bool flag)
{
    DialogBox *dialog;

    if (activateFlag != flag && hasFocus()
        && (dialog = (DialogBox *)window())) {
        if (flag)
            dialog->holdDefaultPushButton();
        else
            dialog->unholdDefaultPushButton();
    }
    activateFlag = flag;
}

/*******************************************************************************
 *
 *  NON-CLASS MEMBER FUNCTIONS
 *
 ******************************************************************************/

QWidget *findWidgetRecursively(QLayoutItem *item, const char *name)
{
    QLayout *layout;
    QWidget *widget;

    if ((layout = item->layout())) {
        for (int i = 0, j = layout->count(); i < j; i++)
            if ((widget = findWidgetRecursively(layout->itemAt(i), name)))
                return widget;
    } else if ((widget = item->widget())) {
        if (!strcmp(widget->objectName().toLocal8Bit().constData(), name))
            return widget;
        if ((layout = widget->layout()))
            return findWidgetRecursively(layout, name);
    }
    return nullptr;
}

QLayout *findLayoutRecursively(QLayout *layout, QWidget *widget)
{
    if (widget && layout) {
        for (int i = 0, j = layout->count(); i < j; i++) {
            QLayoutItem *item = layout->itemAt(i);
            QWidget *w = item->widget();
            QLayout *retlayout;

            if (w == widget) {
                return layout;
            } else {
                retlayout = findLayoutRecursively(w ? w->layout()
                                                  : item->layout(), widget);
                if (retlayout)
                    return retlayout;
            }
        }
    }
    return nullptr;
}
