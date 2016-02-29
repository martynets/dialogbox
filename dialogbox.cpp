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

static const char* about_label="_dbabout_";

/*******************************************************************************
 *
 * DialogBox constructor
 *
 * ****************************************************************************/

DialogBox::DialogBox(const char* title, const char* about, bool resizable, FILE* out):
										default_pb(NULL),
										current_layout(new QVBoxLayout),
										current_index(0),
										group_layout(NULL),
										current_view(NULL),
										current_list_widget(NULL),
										current_tab_widget(NULL),
										output(out),
										empty(true)
{

    QVBoxLayout *mainLayout=new QVBoxLayout;
    QHBoxLayout *hl=new QHBoxLayout;

    setLayout(mainLayout);
    mainLayout->addLayout(hl);
    hl->addLayout(current_layout);

	if(!resizable) mainLayout->setSizeConstraint(QLayout::SetFixedSize);
	else setSizeGripEnabled(true);

	mainLayout->setAlignment(LAYOUTS_ALIGNMENT);
	current_layout->setAlignment(LAYOUTS_ALIGNMENT);

	pages.append(this);

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

	connect(pb, SIGNAL(clicked()), this, SLOT(PushbuttonClicked()));
	connect(pb, SIGNAL(toggled(bool)), this, SLOT(PushbuttonToggled(bool)));
    if(apply)
		{
			connect(pb, SIGNAL(clicked()), this, SLOT(Report()));
			if(exit) connect(pb, SIGNAL(clicked()), this, SLOT(accept()));
		}
	else if(exit) connect(pb, SIGNAL(clicked()), this, SLOT(reject()));

	if(def)
		{
			pb->setDefault(true);
			default_pb=pb;
		}

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
	style&=DialogCommand::property_mask;
	style&=~DialogCommand::property_vertical;	// reset this bit instead of masking another 7 bits
	if(style)
		{
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

void DialogBox::AddListbox(const char* title, const char* name, bool activation, bool selection)
{
	QVBoxLayout* box=new QVBoxLayout;
	QLabel* label=new QLabel(title);
	Listbox* list=new Listbox;

	label->setObjectName(QString(name));
	label->setBuddy(list);
	label->setFocusProxy(list);

	box->addWidget(label);
	box->addWidget(list);

	current_list_widget=current_view=list;
	view_index=0;

	if(group_layout) group_layout->insertLayout(group_index++,box);
	else current_layout->insertLayout(current_index++,box);

    if(activation)
		{
			connect(list, SIGNAL(activated(const QModelIndex&)), this, SLOT(ListboxItemActivated(const QModelIndex&)));
			list->SetActivateFlag(true);
		}

    if(selection)
		{
			connect(list, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
				this, SLOT(ListboxItemSelected(QListWidgetItem*)));
		}

	update_tab_order();
}

void DialogBox::AddCombobox(const char* title, const char* name, bool editable, bool selection)
{
	QHBoxLayout* box=new QHBoxLayout;
	QLabel* label=new QLabel(title);
	QComboBox* list=new QComboBox;

	label->setObjectName(QString(name));
	label->setBuddy(list);
	label->setFocusProxy(list);

	list->setEditable(editable);
	list->setInsertPolicy(QComboBox::NoInsert);	// Prevent insertions as there is no way to report them.
												// Instead an apply pushbutton must be used to collect
												// and process current values.

	box->addWidget(label);
	box->addWidget(list);

	current_view=list->view();
	current_list_widget=list;
	view_index=0;

	if(group_layout) group_layout->insertLayout(group_index++,box);
	else current_layout->insertLayout(current_index++,box);

    if(selection)
		connect(list, SIGNAL(currentIndexChanged(int)), this, SLOT(ComboboxItemSelected(int)));

	update_tab_order();
}

void DialogBox::AddItem(const char* title, const char* icon, bool current)
{
	if(current_view)
		{
			QAbstractItemModel* model=current_view->model();

			model->insertRows(view_index, 1);
			model->setData(model->index(view_index, 0), QIcon(icon), Qt::DecorationRole);
			model->setData(model->index(view_index, 0), QString(title), Qt::DisplayRole);
			if(current || model->rowCount() == 1)
				{
					if(WidgetType(current_list_widget)==DialogCommand::combobox)
						((QComboBox*)current_list_widget)->setCurrentIndex(view_index);
					else
						current_view->setCurrentIndex(model->index(view_index, 0));
				}
			view_index++;
		}
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

void DialogBox::AddProgressbar(const char* name, bool vertical, bool busy)
{
	QProgressBar* pb=new QProgressBar;

	pb->setObjectName(QString(name));

	if(vertical)
		{
			pb->setOrientation(Qt::Vertical);
			pb->setTextDirection(QProgressBar::BottomToTop);
		}

	if(busy) pb->setRange(0, 0);
	else pb->setRange(0, 100);

	if(group_layout) group_layout->insertWidget(group_index++,pb);
	else current_layout->insertWidget(current_index++,pb);
}

void DialogBox::AddSlider(const char* name, bool vertical, int min, int max)
{
	QSlider* slider=new QSlider;

	slider->setObjectName(QString(name));
	if(!vertical) slider->setOrientation(Qt::Horizontal);
	slider->setTickPosition(QSlider::TicksAbove);
	slider->setTracking(false);
	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(SliderValueChanged(int)));
	connect(slider, SIGNAL(rangeChanged(int,int)), this, SLOT(SliderRangeChanged(int,int)));

	slider->setRange(min, max);

	if(group_layout) group_layout->insertWidget(group_index++,slider);
	else current_layout->insertWidget(current_index++,slider);

	update_tab_order();
}

void DialogBox::AddTextview(const char* name, const char* file)
{
	QTextEdit* viewer=new QTextEdit;
	QFile txt(file);

	viewer->setObjectName(QString(name));
	viewer->setReadOnly(true);
	if(txt.open(QFile::ReadOnly))
		viewer->setText(QTextStream(&txt).readAll());

	// below includes TextSelectableByMouse | LinksAccessibleByMouse | LinksAccessibleByKeyboard
	// this allows to select by mouse and copy text or link by context menu
	// Qt::TextSelectableByKeyboard adds keyboard cursor which makes scrolling inconvenient
	viewer->setTextInteractionFlags(Qt::TextBrowserInteraction);

	if(group_layout) group_layout->insertWidget(group_index++,viewer);
	else current_layout->insertWidget(current_index++,viewer);

	update_tab_order();
}

void DialogBox::AddTabs(const char* name, unsigned int position)
{
	QTabWidget* tabs=new QTabWidget;

	tabs->setObjectName(QString(name));

	position&=DialogCommand::property_mask;
	position&=~DialogCommand::property_iconsize;	// reset this bit instead of masking another 4 bits
	if(position)
		tabs->setTabPosition(position & DialogCommand::property_position_top ? QTabWidget::North :
								position & DialogCommand::property_position_bottom ? QTabWidget::South :
								position & DialogCommand::property_position_left ? QTabWidget::West :
								QTabWidget::East);

	current_tab_widget=tabs;
	tab_index=0;

	if(group_layout) group_layout->insertWidget(group_index++,tabs);
	else current_layout->insertWidget(current_index++,tabs);

	update_tab_order();
}

void DialogBox::AddPage(const char* title, const char* name, const char* icon, bool current)
{
	if(current_tab_widget)
		{
			EndGroup();

			QWidget* page=new QWidget;

			page->setObjectName(QString(name));

			QVBoxLayout* ml=new QVBoxLayout;
			QHBoxLayout* hl=new QHBoxLayout;
			current_layout=new QVBoxLayout;
			current_index=0;

			page->setLayout(ml);
			ml->addLayout(hl);
			hl->addLayout(current_layout);

			ml->setSizeConstraint(QLayout::SetFixedSize);
			ml->setAlignment(LAYOUTS_ALIGNMENT);
			current_layout->setAlignment(LAYOUTS_ALIGNMENT);

			pages.append(page);

			connect(page, SIGNAL(destroyed(QObject*)), this, SLOT(RemovePage(QObject*)));

			current_tab_widget->insertTab(tab_index, page, QIcon(icon), QString(title));
			if(current)
				current_tab_widget->setCurrentIndex(tab_index);
			tab_index++;
		}
}

void DialogBox::EndPage()
{
	QWidget* page=current_layout->parentWidget();

	if(page != (QWidget*)this && current_tab_widget == page->parent()->parent())
		{
			QLayout* layout;
			QObject* parent;

			EndGroup();
			layout=FindLayout(current_tab_widget);
			parent=layout->parent();
			if(parent->isWidgetType())
				{
					group_layout=(QBoxLayout*)layout;
					group_index=layout->indexOf(current_tab_widget)+1;
					current_layout=(QBoxLayout*)FindLayout((QWidget*)parent);
					current_index=current_layout->indexOf((QWidget*)parent)+1;
				}
			else
				{
					current_layout=(QBoxLayout*)layout;
					current_index=layout->indexOf(current_tab_widget)+1;
				}
		}
}

void DialogBox::EndTabs()
{
	EndPage();

	QWidget* page=current_layout->parentWidget();

	if(page != (QWidget*)this)
		{
			current_tab_widget=(QTabWidget*)page->parent()->parent();
			tab_index=current_tab_widget->count();
		}
	else
		current_tab_widget=NULL;
}

bool DialogBox::IsLayoutOnPage(QWidget* page, QLayout* layout)
{
	if(!layout) return(false);
	QObject* parent=layout->parent();
	QWidget* parentWidget=parent->isWidgetType() ? ((QWidget*)parent)->parentWidget() : layout->parentWidget();

	if(parentWidget==page) return(true);
	if(parentWidget==this) return(false);
	return(IsLayoutOnPage(page, FindLayout((QWidget*)parentWidget->parent()->parent())));
}

bool DialogBox::IsWidgetOnPage(QWidget* page, QWidget* widget)
{
	if(!widget) return(false);
	return(IsLayoutOnPage(page, FindLayout(widget)));
}

bool DialogBox::IsLayoutOnContainer(QWidget* container, QLayout* layout)
{
	if(!layout) return(false);
	QObject* parent=layout->parent();
	QWidget* page;

	if(parent->isWidgetType())
		{
			if(parent==container) return(true);
			page=((QWidget*)parent)->parentWidget();
		}
	else page=layout->parentWidget();

	if(page==this) return(false);
	return(IsLayoutOnContainer(container, FindLayout((QWidget*)page->parent()->parent())));
}

bool DialogBox::IsWidgetOnContainer(QWidget* container, QWidget* widget)
{
	if(!widget) return(false);
	return(IsLayoutOnContainer(container, FindLayout(widget)));
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

void DialogBox::Clear(char* name)
{
	QWidget* widget;

	if(name[0])
		{
			if( (widget=FindWidget(name)) )
				{
					switch((unsigned)WidgetType(widget))
						{
							case DialogCommand::listbox:
							case DialogCommand::combobox:
								ClearChosenList();
								break;
							case DialogCommand::page:
								ClearPage(widget);
								break;
							case DialogCommand::tabs:
								ClearTabs((QTabWidget*)widget);
								break;
						}
				}
		}
	else ClearDialog();
}

void DialogBox::ClearChosenList()
{
	if(chosen_view && !chosen_row_flag)
		{
			QAbstractItemModel* model=chosen_view->model();
			if(model->removeRows(0,model->rowCount())) view_index=0;
			else view_index=model->rowCount();
		}
}

void DialogBox::ClearTabs(QTabWidget* widget)
{
	for(int i=widget->count(); i>0; i--)
		{
			QWidget* page=widget->widget(i-1);

			while(IsWidgetOnPage(page, current_tab_widget)) EndTabs();
			if(IsWidgetOnPage(page, current_list_widget)) EndList();
			if(IsWidgetOnPage(page, default_pb)) default_pb=NULL;

			if(IsLayoutOnPage(page, current_layout))
				{
					EndGroup();
					QLayout* layout=FindLayout(widget);
					QObject* parent=layout->parent();
					if(parent->isWidgetType())
						{
							group_layout=(QBoxLayout*)layout;
							group_index=layout->indexOf(widget)+1;
							current_layout=(QBoxLayout*)FindLayout((QWidget*)parent);
							current_index=current_layout->indexOf((QWidget*)parent)+1;
						}
					else
						{
							current_layout=(QBoxLayout*)layout;
							current_index=layout->indexOf(widget)+1;
						}
				}

			delete page;
		}
}

void DialogBox::ClearPage(QWidget* widget)
{
	QLayout *layout0, *layout1, *layout2, *layout3;
	layout0=widget->layout();	// main (vertical one)

	while(IsWidgetOnPage(widget, current_tab_widget)) EndTabs();
	if(IsWidgetOnPage(widget, current_list_widget)) EndList();
	if(IsWidgetOnPage(widget, default_pb)) default_pb=NULL;

	if(IsLayoutOnPage(widget, current_layout))
		{
			EndGroup();
			current_layout=(QBoxLayout*)layout0->itemAt(0)->layout()->itemAt(0)->layout();
			current_index=0;
		}

	for(int i=layout0->count()-1; i>=0; i--)
		{
			layout1=layout0->itemAt(i)->layout();	// horizontal ones
			for(int j=layout1->count()-1; j>=0; j--)
				{
					layout2=layout1->itemAt(j)->layout();	// vertical ones

					QLayoutItem *li, *wi;
					QWidget* w;
					while( (li=layout2->takeAt(0)) )
						{
							if((layout3=li->layout()))	// joint widgets' layout
								{
									while( (wi=layout3->takeAt(0)) )
										{
											if( (w=wi->widget()) ) delete w;	// QWidget isn't inherited by QWidgetItem and must be deleted separately
											delete wi;
										}
								}
							if( (w=li->widget()) ) delete w;	// Container widget owns installed layout and its widgets and deletes them
							delete li;
						}
					if(i!=0 || j!=0) delete layout1->takeAt(j);
					//else that is the last one layout made the current one (assigned to current_layout)
				}
			if(i!=0) delete layout0->takeAt(i);
		}
}

void DialogBox::ClearDialog()
{
	default_pb=NULL;
	current_view=NULL;
	current_list_widget=NULL;
	current_tab_widget=NULL;
	group_layout=NULL;
	current_layout=(QBoxLayout*)layout()->itemAt(0)->layout()->itemAt(0)->layout();
	current_index=0;

	while(pages.count()>1) delete pages.takeLast();

	ClearPage(this);
}

void DialogBox::RemoveWidget(char* name)
{
	QWidget* widget;

	if( (widget=FindWidget(name)) )
		{
			int type=WidgetType(widget);

			if(type==DialogCommand::item)
				{
					if(chosen_row>=0)
						{
							if(chosen_view==current_view && chosen_row<view_index) view_index--;
							chosen_view->model()->removeRows(chosen_row,1);
						}
					chosen_row_flag=false;
				}
			else if(QLayout* layout=FindLayout(widget))
				{
					switch(type)
						{
							case DialogCommand::tabs:
								for(int i=0, j=((QTabWidget*)widget)->count(); i<j; i++)
									{
										QWidget* page=((QTabWidget*)widget)->widget(i);

										while(IsWidgetOnPage(page, current_tab_widget)) EndTabs();
										if(current_tab_widget==widget) EndTabs();
										if(IsWidgetOnPage(page, current_list_widget)) EndList();
										if(IsWidgetOnPage(page, default_pb)) default_pb=NULL;
										if(IsLayoutOnPage(page, current_layout))	// position focus behind the parent QTabWidget
											{
												EndGroup();
												QObject* parent=layout->parent();
												if(parent->isWidgetType())
													{
														group_layout=(QBoxLayout*)layout;
														group_index=layout->indexOf(widget);
														current_layout=(QBoxLayout*)FindLayout((QWidget*)parent);
														current_index=current_layout->indexOf((QWidget*)parent)+1;
													}
												else
													{
														current_layout=(QBoxLayout*)layout;
														current_index=layout->indexOf(widget);
													}
											}
									}
								break;
							case DialogCommand::frame:
							case DialogCommand::groupbox:
								while(IsWidgetOnContainer(widget, current_tab_widget)) EndTabs();
								if(IsWidgetOnContainer(widget, current_list_widget)) EndList();
								if(IsWidgetOnContainer(widget, default_pb)) default_pb=NULL;
								if(IsLayoutOnContainer(widget, current_layout))
									{
										EndGroup();
										current_layout=(QBoxLayout*)layout;
										current_index=layout->indexOf(widget);
									}
								if(group_layout && widget->layout()==group_layout) EndGroup();
								break;
							case DialogCommand::listbox:
							case DialogCommand::combobox:
								if(current_view && chosen_view==current_view) EndList();
								break;
							case DialogCommand::pushbutton:
								if((QPushButton*)widget==default_pb) default_pb=NULL;
								break;
						}

					if(layout==group_layout && layout->indexOf(widget)<group_index) group_index--;
					if(layout==current_layout && layout->indexOf(widget)<current_index) current_index--;

					if(QWidget* proxywidget=widget->focusProxy())		// for joint widgets
						{
							layout->removeWidget(proxywidget);
							delete proxywidget;
						}

					layout->removeWidget(widget);
					delete widget;	// this also deletes child layouts and widgets
									// (parented to parentWidget by addWidget and addLayout) and so forth
									// for QTabWidget it deletes child pages which triggers RemovePage slot

					sanitize_layout(layout);
				}
			else	// this is page
				{
					while(IsWidgetOnPage(widget, current_tab_widget)) EndTabs();
					if(IsWidgetOnPage(widget, current_list_widget)) EndList();
					if(IsWidgetOnPage(widget, default_pb)) default_pb=NULL;

					if(IsLayoutOnPage(widget, current_layout))	// position focus behind the parent QTabWidget
						{
							EndGroup();

							QWidget* tabs=(QWidget*)widget->parent()->parent();
							layout=(QBoxLayout*)FindLayout(tabs);
							QObject* parent=layout->parent();

							if(parent->isWidgetType())
								{
									group_layout=(QBoxLayout*)layout;
									group_index=layout->indexOf(tabs)+1;
									current_layout=(QBoxLayout*)FindLayout((QWidget*)parent);
									current_index=current_layout->indexOf((QWidget*)parent)+1;
								}
							else
								{
									current_layout=(QBoxLayout*)layout;
									current_index=layout->indexOf(tabs)+1;
								}
						}
					delete widget;
				}
		}
}

void DialogBox::Position(char* name, bool behind, bool onto)
{
	QWidget* widget;
	QBoxLayout* layout;
	bool tabs_set=false;

	if( (widget=FindWidget(name)) )
		{
			if( !(layout=(QBoxLayout*)FindLayout(widget)) )	// this is page. widget!=this isn't tested as root page has no name
				{
					current_tab_widget=(QTabWidget*)widget->parent()->parent();
					tab_index=current_tab_widget->indexOf(widget);
					if(behind) tab_index++, behind=false;
					if(onto)
						{
							EndGroup();
							layout=(QBoxLayout*)widget->layout();
							layout=(QBoxLayout*)layout->itemAt(layout->count()-1)->layout();
							current_layout=(QBoxLayout*)layout->itemAt(layout->count()-1)->layout();
							current_index=current_layout->count();
							return;
						}
					widget=current_tab_widget;
					layout=(QBoxLayout*)FindLayout(widget);
					tabs_set=true;
				}

			if(layout)
				{
					QWidget* page;
					QObject* parent;
					int index;
					int type=WidgetType(widget);

					if(widget->focusProxy() && !(type & DialogCommand::tabs))	// joint widget
						{
							parent=layout->parent()->parent();
							index=indexOf(layout);
							layout=(QBoxLayout*)layout->parent();
						}
					else
						{
							parent=layout->parent();
							index=layout->indexOf(widget);
						}

					if(onto && type & (DialogCommand::listbox | DialogCommand::combobox))
						{
							current_view=chosen_view;
							current_list_widget=chosen_list_widget;
							view_index=current_view->model()->rowCount();
						}
					if(type & DialogCommand::item)
						{
							current_view=chosen_view;
							current_list_widget=chosen_list_widget;
							view_index= chosen_row>=0 ? chosen_row : current_view->model()->rowCount();
							if(behind) view_index++, behind=false;
						}

					if(parent->isWidgetType())	// the widget is installed onto QGroupBox/QFrame
						{
							page=((QWidget*)parent)->parentWidget();

							group_layout=layout;
							group_index=index;
							if(behind) group_index++;

							if( (layout=(QBoxLayout*)FindLayout( (QWidget*)parent )) )
								{
									current_layout=layout;
									current_index=layout->indexOf((QWidget*)parent)+1;
								}
						}
					else
						{
							page=widget->parentWidget();
							EndGroup();

							current_layout=layout;
							current_index=index;
							if(behind) current_index++;

							if(onto && (layout=(QBoxLayout*)widget->layout()))	// position onto QGroupBox/QFrame object
								{
									group_layout=layout;
									group_index=layout->count();
								}
						}

					if(!tabs_set)
						{
							if(onto && type & DialogCommand::tabs)
								{
									current_tab_widget=(QTabWidget*)widget;
									tab_index=((QTabWidget*)widget)->count();
								}
							else
								{
									if(page==this) current_tab_widget=NULL;
									else
										{
											current_tab_widget=(QTabWidget*)page->parent()->parent();
											tab_index=current_tab_widget->indexOf(page);
										}
								}
						}
				}
		}
}

void DialogBox::SetEnabled(QWidget* widget, bool enable)
{
	switch((unsigned)WidgetType(widget))
		{
			case DialogCommand::page:
				{
					QTabWidget* tabs=(QTabWidget*)widget->parent()->parent();
					tabs->setTabEnabled(tabs->indexOf(widget), enable);
				}
				break;
			default:
				widget->setEnabled(enable);
				if(QWidget* proxywidget=widget->focusProxy()) proxywidget->setEnabled(enable);
		}
}

/*******************************************************************************
 *	FindWidget searches for the widget with the given name. It does some analysis
 * 	of the name to recognize list item references (#number and :text) and sets
 * 	internal pointer to the QAbstractItemView and to that item.
 * ****************************************************************************/
QWidget* DialogBox::FindWidget(char* name)
{
	QWidget* widget=NULL;

	chosen_view=NULL;
	chosen_row_flag=false;

	if(name)
		{
			int li_row=-1;
			char* li_name=NULL;

			for(int i=0; name[i]!=0; i++)
				{
					if(name[i]=='#')
						{
							name[i]=0;
							if(name[i+1])
								{
									sscanf(name+i+1, "%d", &li_row);
								}
							break;
						}
					if(name[i]==':')
						{
							name[i]=0;
							li_name=name+i+1;
							break;
						}
				}

			if(name[0])
				for(int i=0, j=pages.count(); i<j; i++)
					{
						widget=pages.at(i);
						if(!strcmp(widget->objectName().toLocal8Bit().constData(), name)) break; // the widget is page
						if( (widget=find_widget_recursively(widget->layout(), name)) ) break;
					}

			switch((unsigned)WidgetType(widget))
				{
					case DialogCommand::listbox:
						chosen_view=(Listbox*)(chosen_list_widget=widget->focusProxy());
						break;
					case DialogCommand::combobox:
						chosen_list_widget=widget->focusProxy();
						chosen_view=((QComboBox*)chosen_list_widget)->view();
						break;
				}
			if(chosen_view)
				{
					if(li_row>=0) chosen_row=li_row, chosen_row_flag=true;
					if(li_name)
						{
							QAbstractItemModel* model=chosen_view->model();
							int i, j;
							for(i=0, j=model->rowCount(); i<j; i++)
								if(!strcmp(model->data(model->index(i, 0), Qt::DisplayRole).toString().toLocal8Bit().constData(), li_name))
									{
										chosen_row=i, chosen_row_flag=true;
										break;
									}
							if(i==j) chosen_row=i, chosen_row_flag=true;
						}
				}
		}

	return(widget);
}

/*******************************************************************************
 *	FindLayout searches for the layout the given widget is laid on.
 * ****************************************************************************/
QLayout* DialogBox::FindLayout(QWidget* widget)
{
	QLayout* layout=NULL;
	QWidget* page;

	for(int i=0, j=pages.count(); i<j; i++)
		{
			if( (page=pages.at(i)) == widget ) break; // the widget is page
			if( (layout=find_layout_recursively(page->layout(), widget)) ) break;
		}

	return(layout);
}

/*******************************************************************************
 *	Duplicate of void QDialogPrivate::hideDefault() which is made private.
 * 	Is called by listbox widget with activation option set when it gets focus.
 * ****************************************************************************/
void DialogBox::HideDefault()
{
    QList<QPushButton*> list = findChildren<QPushButton*>();
    for (int i=0; i<list.size(); i++) list.at(i)->setDefault(false);
}

/*******************************************************************************
 * 	Is called by listbox widget with activation option set when it loses focus.
 * 	Unfortunately QDialog keeps all properties/methods for default/autodefault
 * 	pushbuttons management private. Don't see a graceful way to restore default
 * 	indicator for an autodefault pushbutton...
 * ****************************************************************************/
void DialogBox::ShowDefault()
{
    if(default_pb) default_pb->setDefault(true);
}

/*******************************************************************************
 *	WidgetType returns the type of the widget as value of
 *  DialogCommand::Controls enum
 * ****************************************************************************/
DialogCommand::Controls DialogBox::WidgetType(QWidget* widget)
{
	if(widget)
		{
			const char* name=widget->metaObject()->className();

			if(!strcmp(name, "DialogBox")) return(DialogCommand::dialog);
			if(!strcmp(name, "QPushButton")) return(DialogCommand::pushbutton);
			if(!strcmp(name, "QRadioButton")) return(DialogCommand::radiobutton);
			if(!strcmp(name, "QCheckBox")) return(DialogCommand::checkbox);
			if(!strcmp(name, "QLabel"))
				{
					if(QWidget* proxywidget=widget->focusProxy())
						{
							const char* proxyname=proxywidget->metaObject()->className();

							if(!strcmp(proxyname, "QLineEdit")) return(DialogCommand::textbox);
							if(!strcmp(proxyname, "QComboBox"))
								{
									if(chosen_row_flag) return(DialogCommand::item);
									return(DialogCommand::combobox);
								}
							if(!strcmp(proxyname, "Listbox"))
								{
									if(chosen_row_flag) return(DialogCommand::item);
									return(DialogCommand::listbox);
								}
						}
					return(DialogCommand::label);
				}
			if(!strcmp(name, "QGroupBox")) return(DialogCommand::groupbox);
			if(!strcmp(name, "QFrame"))
				{
					int shape=((QFrame *)widget)->frameStyle() & QFrame::Shape_Mask;

					if(shape==QFrame::HLine || shape==QFrame::VLine) return(DialogCommand::separator);
					return(DialogCommand::frame);
				}
			if(!strcmp(name, "QLineEdit")) return(DialogCommand::textbox); // if the object queried directly
			if(!strcmp(name, "Listbox")) // if the object queried directly
				{
					if(chosen_row_flag) return(DialogCommand::item);
					return(DialogCommand::listbox);
				}
			if(!strcmp(name, "QComboBox")) // if the object queried directly
				{
					if(chosen_row_flag) return(DialogCommand::item);
					return(DialogCommand::combobox);
				}

			if(!strcmp(name, "QTabWidget")) return(DialogCommand::tabs);
			if(!strcmp(name, "QWidget")) return(DialogCommand::page);

			if(!strcmp(name, "QProgressBar")) return(DialogCommand::progressbar);
			if(!strcmp(name, "QSlider")) return(DialogCommand::slider);
			if(!strcmp(name, "QTextEdit")) return(DialogCommand::textview);
		}

	return(DialogCommand::none);
}

/*******************************************************************************
 *
 * Listbox class is introduced to prevent Enter key hit
 * propagation to default pushbutton when listbox activation option is on or
 * combobox widget is editable. In other words to prevent two controls to
 * respond to a single key.
 *
 * ****************************************************************************/

void Listbox::focusInEvent(QFocusEvent* event)
{
	if(activate_flag)
		{
			DialogBox* dialog=(DialogBox*)window();
			if(dialog) dialog->HideDefault();
		}
	QListWidget::focusInEvent(event);
}

void Listbox::focusOutEvent(QFocusEvent *event)
{
	if(activate_flag)
		{
			DialogBox* dialog=(DialogBox*)window();
			if(dialog) dialog->ShowDefault();
		}
	QListWidget::focusOutEvent(event);
}

void Listbox::SetActivateFlag(bool flag)
{
	DialogBox* dialog;

	if(activate_flag != flag && hasFocus() && (dialog=(DialogBox*)window()))
		{
			if(flag) dialog->HideDefault();
			else dialog->ShowDefault();
		}
	activate_flag=flag;
}

/*******************************************************************************
 *
 *	NON-CLASS MEMBER FUNCTIONS
 *
 * ****************************************************************************/

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
