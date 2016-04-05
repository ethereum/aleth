/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file SettingDialog.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2015
 */


#include "SettingsDialog.h"

#include <QIcon>
#include <QLabel>
#include <QListView>
#include <QAbstractListModel>
#include <QStyledItemDelegate>
#include <QScrollBar>
#include <QDialogButtonBox>
#include <QPushButton>
#include "Plugin.h"
#include "ui_SettingsDialog.h"

const int categoryIconSize = 24;

namespace dev
{
namespace aleth
{
namespace zero
{

struct Category
{
	int index = -1;
	QString displayName;
	QIcon icon;
	SettingsPage *widget;
};

class CategoryModel: public QAbstractListModel
{
public:
	CategoryModel(QObject* _parent = 0);

	void addPage(int _index, QString const& _title, SettingsPage* _widget);
	int rowCount(QModelIndex const& _parent = QModelIndex()) const;
	QVariant data(QModelIndex const& _index, int _role = Qt::DisplayRole) const;
	QList<Category> const& categories() const { return m_categories; }

private:
	QList<Category> m_categories;
	QIcon m_emptyIcon;
};

CategoryModel::CategoryModel(QObject* _parent):
	QAbstractListModel(_parent)
{
	QPixmap empty(categoryIconSize, categoryIconSize);
	empty.fill(Qt::transparent);
	m_emptyIcon = QIcon(empty);
}

int CategoryModel::rowCount(QModelIndex const& _parent) const
{
	return _parent.isValid() ? 0 : m_categories.size();
}

QVariant CategoryModel::data(QModelIndex const& _index, int role) const
{
	switch (role)
	{
	case Qt::DisplayRole:
		return m_categories.at(_index.row()).displayName;
	case Qt::DecorationRole:
	{
		QIcon icon = m_categories.at(_index.row()).icon;
		if (icon.isNull())
			icon = m_emptyIcon;
		return icon;
	}
	}

	return QVariant();
}

void CategoryModel::addPage(int _index, QString const& _title, SettingsPage* _widget)
{
	beginInsertRows(QModelIndex(), rowCount(), rowCount() + 1);
	Category cat;
	cat.index = _index;
	cat.displayName = _title;
	cat.widget = _widget;
	m_categories.append(std::move(cat));
	endInsertRows();
}

class CategoryListViewDelegate : public QStyledItemDelegate
{
public:
	CategoryListViewDelegate(QObject *parent) : QStyledItemDelegate(parent) {}
	QSize sizeHint(QStyleOptionViewItem const& _option, QModelIndex const& _index) const
	{
		QSize size = QStyledItemDelegate::sizeHint(_option, _index);
		size.setHeight(qMax(size.height(), 32));
		return size;
	}
};

class CategoryListView : public QListView
{
public:
	CategoryListView(QWidget* _parent = 0): QListView(_parent)
	{
		setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
		setItemDelegate(new CategoryListViewDelegate(this));
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	}

	virtual QSize sizeHint() const
	{
		int width = sizeHintForColumn(0) + frameWidth() * 2 + 5;
		width += verticalScrollBar()->width();
		return QSize(width, 100);
	}

	// QListView installs a event filter on its scrollbars
	virtual bool eventFilter(QObject *obj, QEvent *event)
	{
		if (obj == verticalScrollBar()
				&& (event->type() == QEvent::Show
					|| event->type() == QEvent::Hide))
			updateGeometry();
		return QListView::eventFilter(obj, event);
	}
};


SettingsDialog::SettingsDialog(QWidget* _parent) :
	QDialog(_parent),
	m_model(new CategoryModel(this)),
	m_ui(new Ui::SettingsDialog)
{
	m_ui->setupUi(this);
	m_ui->stackedWidget->removeWidget(m_ui->stackedWidget->widget(0));
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	m_ui->categoryList->setIconSize(QSize(categoryIconSize, categoryIconSize));
	m_ui->categoryList->setModel(m_model);
	m_ui->categoryList->setSelectionMode(QAbstractItemView::SingleSelection);
	m_ui->categoryList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	connect(m_ui->categoryList->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &SettingsDialog::currentChanged);
	m_ui->categoryList->setFocus();

	connect(m_ui->buttonBox->button(QDialogButtonBox::Apply), &QAbstractButton::clicked, this, &SettingsDialog::apply);
	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
}

SettingsDialog::~SettingsDialog()
{
}

int SettingsDialog::exec()
{
	displayed();
	for (int c = 0; c < m_model->rowCount(); ++c)
		m_model->categories().at(c).widget->displayed();
	return QDialog::exec();
}

void SettingsDialog::showCategory(int index)
{
	Category const& category = m_model->categories().at(index);
	// Update current category and page
	m_currentCategory = index;
	m_ui->stackedWidget->setCurrentIndex(index);
	m_ui->headerLabel->setText(category.displayName);
}

void SettingsDialog::currentChanged(const QModelIndex &current)
{
	if (current.isValid())
		showCategory(current.row());
	else
	{
		m_ui->stackedWidget->setCurrentIndex(0);
		m_ui->headerLabel->clear();
	}
}

void SettingsDialog::accept()
{
	apply();
	done(QDialog::Accepted);
}

void SettingsDialog::reject()
{
	done(QDialog::Rejected);
}

void SettingsDialog::apply()
{
	for (int c = 0; c < m_model->rowCount(); ++c)
		m_model->categories().at(c).widget->applied();
	applied();
}

void SettingsDialog::addPage(int _index, QString const& _title, std::function<SettingsPage*()> const& _factory)
{
	SettingsPage* widget = _factory();
	widget->setParent(this);
	m_ui->stackedWidget->addWidget(widget);
	m_model->addPage(_index, _title, widget);
}

}
}
}
