/*
 * Stellarium
 * Copyright (C) 2012 Anton Samoylov
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include <QDialog>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

#include "StelApp.hpp"
#include "StelShortcutMgr.hpp"
#include "StelTranslator.hpp"
#include "StelShortcutGroup.hpp"

#include "ShortcutsDialog.hpp"
#include "ui_shortcutsDialog.h"

ShortcutLineEdit::ShortcutLineEdit(QWidget *parent) :
	QLineEdit(parent)
{
	// call clear for setting up private fields
	clear();
}

QKeySequence ShortcutLineEdit::getKeySequence()
{
	return QKeySequence(m_keys[0], m_keys[1], m_keys[2], m_keys[3]);
}

void ShortcutLineEdit::clear()
{
	m_keyNum = m_keys[0] = m_keys[1] = m_keys[2] = m_keys[3] = 0;
	QLineEdit::clear();
	emit contentsChanged();
}

void ShortcutLineEdit::backspace()
{
	if (m_keyNum <= 0)
	{
		qDebug() << "Clear button works when it shouldn't: lineEdit is empty ";
		return;
	}
	--m_keyNum;
	m_keys[m_keyNum] = 0;
	// update text
	setContents(getKeySequence());
}

void ShortcutLineEdit::setContents(QKeySequence ks)
{
	// need for avoiding infinite loop of same signal-slot emitting/calling
	if (ks.toString(QKeySequence::NativeText) == text())
		return;
	// clear before setting up
	clear();
	// set up m_keys from given key sequence
	m_keyNum = ks.count();
	for (int i = 0; i < m_keyNum; ++i)
	{
		m_keys[i] = ks[i];
	}
	// Show Ctrl button as Cmd on Mac
	setText(ks.toString(QKeySequence::NativeText));
	emit contentsChanged();
}

void ShortcutLineEdit::keyPressEvent(QKeyEvent *e)
{
	int nextKey = e->key();
	if ( m_keyNum > 3 || // too long shortcut
	     nextKey == Qt::Key_Control || // dont count modifier keys
	     nextKey == Qt::Key_Shift ||
	     nextKey == Qt::Key_Meta ||
	     nextKey == Qt::Key_Alt )
		return;
	
	// applying current modifiers to key
	nextKey |= getModifiers(e->modifiers(), e->text());
	m_keys[m_keyNum] = nextKey;
	++m_keyNum;
	
	// set displaying information
	QKeySequence ks(m_keys[0], m_keys[1], m_keys[2], m_keys[3]);
	setText(ks.toString(QKeySequence::NativeText));
	
	emit contentsChanged();
	// not call QLineEdit's event because we already changed contents
	e->accept();
}

void ShortcutLineEdit::focusInEvent(QFocusEvent *e)
{
	emit focusChanged(false);
	QLineEdit::focusInEvent(e);
}

void ShortcutLineEdit::focusOutEvent(QFocusEvent *e)
{
	emit focusChanged(true);
	QLineEdit::focusOutEvent(e);
}


int ShortcutLineEdit::getModifiers(Qt::KeyboardModifiers state, const QString &text)
{
	int result = 0;
	// The shift modifier only counts when it is not used to type a symbol
	// that is only reachable using the shift key anyway
	if ((state & Qt::ShiftModifier) && (text.size() == 0
	                                    || !text.at(0).isPrint()
	                                    || text.at(0).isLetterOrNumber()
	                                    || text.at(0).isSpace()))
		result |= Qt::SHIFT;
	if (state & Qt::ControlModifier)
		result |= Qt::CTRL;
	// META key is the same as WIN key on non-MACs
	if (state & Qt::MetaModifier)
		result |= Qt::META;
	if (state & Qt::AltModifier)
		result |= Qt::ALT;
	return result;
}

ShortcutsDialog::ShortcutsDialog() :
	ui(new Ui_shortcutsDialogForm),
	filterModel(new QSortFilterProxyModel(this)),
	mainModel(new QStandardItemModel(this))
{
	shortcutMgr = StelApp::getInstance().getStelShortcutManager();
}

ShortcutsDialog::~ShortcutsDialog()
{
	collisionItems.clear();
	delete ui;
	ui = NULL;
}

void ShortcutsDialog::drawCollisions()
{
	QBrush brush(Qt::red);
	foreach(QStandardItem* item, collisionItems)
	{
		// change colors of all columns for better visibility
		item->setForeground(brush);
		QModelIndex index = item->index();
		mainModel->itemFromIndex(index.sibling(index.row(), 1))->setForeground(brush);
		mainModel->itemFromIndex(index.sibling(index.row(), 2))->setForeground(brush);
	}
}

void ShortcutsDialog::resetCollisions()
{
	QBrush brush =
	        ui->shortcutsTreeView->palette().brush(QPalette::Foreground);
	foreach(QStandardItem* item, collisionItems)
	{
		item->setForeground(brush);
		QModelIndex index = item->index();
		mainModel->itemFromIndex(index.sibling(index.row(), 1))->setForeground(brush);
		mainModel->itemFromIndex(index.sibling(index.row(), 2))->setForeground(brush);
	}
	collisionItems.clear();
}

void ShortcutsDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateTreeData();
	}
}

void ShortcutsDialog::initEditors()
{
	QModelIndex index = ui->shortcutsTreeView->currentIndex();
	index = index.sibling(index.row(), 0);
	QStandardItem* currentItem = mainModel->itemFromIndex(index);
	if (itemIsEditable(currentItem))
	{
		// current item is shortcut, not group (group items aren't selectable)
		ui->primaryShortcutEdit->setEnabled(true);
		ui->altShortcutEdit->setEnabled(true);
		// fill editors with item's shortcuts
		QVariant data = mainModel->data(index.sibling(index.row(), 1));
		ui->primaryShortcutEdit->setContents(data.value<QKeySequence>());
		data = mainModel->data(index.sibling(index.row(), 2));
		ui->altShortcutEdit->setContents(data.value<QKeySequence>());
	}
	else
	{
		// item is group, not shortcut
		ui->primaryShortcutEdit->setEnabled(false);
		ui->altShortcutEdit->setEnabled(false);
		ui->applyButton->setEnabled(false);
		ui->primaryShortcutEdit->clear();
		ui->altShortcutEdit->clear();
	}
	polish();
}

bool ShortcutsDialog::prefixMatchKeySequence(const QKeySequence& ks1,
                                             const QKeySequence& ks2)
{
	if (ks1.isEmpty() || ks2.isEmpty())
	{
		return false;
	}
	for (uint i = 0; i < qMin(ks1.count(), ks2.count()); ++i)
	{
		if (ks1[i] != ks2[i])
		{
			return false;
		}
	}
	return true;
}

QList<QStandardItem*> ShortcutsDialog::findCollidingItems(QKeySequence ks)
{
	QList<QStandardItem*> result;
	for (int row = 0; row < mainModel->rowCount(); row++)
	{
		QStandardItem* group = mainModel->item(row, 0);
		if (!group->hasChildren())
			continue;
		for (int subrow = 0; subrow < group->rowCount(); subrow++)
		{
			QKeySequence primary(group->child(subrow, 1)
			                     ->data(Qt::DisplayRole).toString());
			QKeySequence secondary(group->child(subrow, 2)
			                       ->data(Qt::DisplayRole).toString());
			if (prefixMatchKeySequence(ks, primary) ||
			    prefixMatchKeySequence(ks, secondary))
				result.append(group->child(subrow, 0));
		}
	}
	return result;
}

void ShortcutsDialog::handleCollisions(ShortcutLineEdit *currentEdit)
{
	resetCollisions();
	
	// handle collisions
	QString text = currentEdit->text();
	collisionItems = findCollidingItems(QKeySequence(text));
	QModelIndex currentIndex = ui->shortcutsTreeView->currentIndex();
	currentIndex = currentIndex.sibling(currentIndex.row(), 0);
	QStandardItem* currentItem = mainModel->itemFromIndex(currentIndex);
	collisionItems.removeOne(currentItem);
	if (!collisionItems.isEmpty())
	{
		drawCollisions();
		ui->applyButton->setEnabled(false);
		// scrolling to first collision item
		ui->shortcutsTreeView->scrollTo(collisionItems.first()->index());
		currentEdit->setProperty("collision", true);
	}
	else
	{
		// scrolling back to current item
		ui->shortcutsTreeView->scrollTo(currentIndex);
		currentEdit->setProperty("collision", false);
	}
}

void ShortcutsDialog::handleChanges()
{
	// work only with changed editor
	ShortcutLineEdit* editor = qobject_cast<ShortcutLineEdit*>(sender());
	bool isPrimary = (editor == ui->primaryShortcutEdit);
	// updating clear buttons
	if (isPrimary)
	{
		ui->primaryBackspaceButton->setEnabled(!editor->isEmpty());
	}
	else
	{
		ui->altBackspaceButton->setEnabled(!editor->isEmpty());
	}
	// updating apply button
	QModelIndex index = ui->shortcutsTreeView->currentIndex();
	if (!index.isValid() ||
	    (isPrimary &&
	     editor->text() == mainModel->data(index.sibling(index.row(), 1))) ||
	    (!isPrimary &&
	     editor->text() == mainModel->data(index.sibling(index.row(), 2))))
	{
		// nothing to apply
		ui->applyButton->setEnabled(false);
	}
	else
	{
		ui->applyButton->setEnabled(true);
	}
	handleCollisions(editor);
	polish();
}

void ShortcutsDialog::applyChanges() const
{
	// get ids stored in tree
	QModelIndex index = ui->shortcutsTreeView->currentIndex();
	if (!index.isValid())
		return;
	index = index.sibling(index.row(), 0);
	QStandardItem* currentItem = mainModel->itemFromIndex(index);
	QString actionId = currentItem->data(Qt::UserRole).toString();
	QString groupId = currentItem->parent()->data(Qt::UserRole).toString();
	// changing keys in shortcuts
	shortcutMgr->changeActionPrimaryKey(actionId, groupId, ui->primaryShortcutEdit->getKeySequence());
	shortcutMgr->changeActionAltKey(actionId, groupId, ui->altShortcutEdit->getKeySequence());
	// no need to change displaying information, as it changed in mgr, and will be updated in connected slot

	// save shortcuts to file
	shortcutMgr->saveShortcuts();

	// nothing to apply until edits' content changes
	ui->applyButton->setEnabled(false);
}

void ShortcutsDialog::switchToEditors(const QModelIndex& index)
{
	QStandardItem* item = mainModel->itemFromIndex(index);
	if (itemIsEditable(item))
	{
		ui->primaryShortcutEdit->setFocus();
	}
}

void ShortcutsDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	initModel();
	ui->shortcutsTreeView->setModel(mainModel);
	QHeaderView* header = ui->shortcutsTreeView->header();
	header->setMovable(false);
	
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->shortcutsTreeView->selectionModel(),
	        SIGNAL(currentChanged(QModelIndex,QModelIndex)),
	        this,
	        SLOT(initEditors()));
	connect(ui->shortcutsTreeView,
	        SIGNAL(activated(QModelIndex)),
	        this,
	        SLOT(switchToEditors(QModelIndex)));
	// apply button logic
	connect(ui->applyButton, SIGNAL(clicked()), this, SLOT(applyChanges()));
	// restore defaults button logic
	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(restoreDefaultShortcuts()));
	// we need to disable all shortcut actions, so we can enter shortcuts without activating any actions
	connect(ui->primaryShortcutEdit, SIGNAL(focusChanged(bool)), shortcutMgr, SLOT(setAllActionsEnabled(bool)));
	connect(ui->altShortcutEdit, SIGNAL(focusChanged(bool)), shortcutMgr, SLOT(setAllActionsEnabled(bool)));
	// handling changes in editors
	connect(ui->primaryShortcutEdit, SIGNAL(contentsChanged()), this, SLOT(handleChanges()));
	connect(ui->altShortcutEdit, SIGNAL(contentsChanged()), this, SLOT(handleChanges()));
	// handle outer shortcuts changes
	connect(shortcutMgr, SIGNAL(shortcutChanged(StelShortcut*)), this, SLOT(updateShortcutsItem(StelShortcut*)));

	updateTreeData();
}

void ShortcutsDialog::updateText()
{
}

void ShortcutsDialog::polish()
{
	ui->primaryShortcutEdit->style()->unpolish(ui->primaryShortcutEdit);
	ui->primaryShortcutEdit->style()->polish(ui->primaryShortcutEdit);
	ui->altShortcutEdit->style()->unpolish(ui->altShortcutEdit);
	ui->altShortcutEdit->style()->polish(ui->altShortcutEdit);
}

QStandardItem* ShortcutsDialog::updateGroup(StelShortcutGroup* group)
{
	QStandardItem* groupItem = findItemByData(QVariant(group->getId()),
	                                          Qt::UserRole);
	if (!groupItem)
	{
		// create new
		groupItem = new QStandardItem();
	}
	// group items aren't selectable, so reset default flag
	groupItem->setFlags(Qt::ItemIsEnabled);
	// setup displayed text
	QString text(q_(group->getText().isEmpty() ? group->getId() : group->getText()));
	groupItem->setText(text);
	// store id
	groupItem->setData(group->getId(), Qt::UserRole);
	groupItem->setColumnCount(3);
	// setup bold font for group lines
	QFont rootFont = groupItem->font();
	rootFont.setBold(true);
	rootFont.setPixelSize(14);
	groupItem->setFont(rootFont);
	mainModel->appendRow(groupItem);
	
	// expand only enabled group
	bool enabled = group->isEnabled();
	QModelIndex index = groupItem->index();
	if (enabled)
		ui->shortcutsTreeView->expand(index);
	else
		ui->shortcutsTreeView->collapse(index);
	ui->shortcutsTreeView->setFirstColumnSpanned(index.row(),
	                                             QModelIndex(),
	                                             true);
	ui->shortcutsTreeView->setRowHidden(index.row(), QModelIndex(), !enabled);
	
	return groupItem;
}

QStandardItem* ShortcutsDialog::findItemByData(QVariant value, int role, int column)
{
	for (int row = 0; row < mainModel->rowCount(); row++)
	{
		QStandardItem* item = mainModel->item(row, 0);
		if (!item)
			continue; //WTF?
		if (column == 0)
		{
			if (item->data(role) == value)
				return item;
		}
		
		for (int subrow = 0; subrow < item->rowCount(); subrow++)
		{
			QStandardItem* subitem = item->child(subrow, column);
			if (subitem->data(role) == value)
				return subitem;
		}
	}
	return 0;
}

void ShortcutsDialog::updateShortcutsItem(StelShortcut *shortcut,
                                          QStandardItem *shortcutItem)
{
	QVariant shortcutId(shortcut->getId());
	if (shortcutItem == NULL)
	{
		// search for item
		shortcutItem = findItemByData(shortcutId, Qt::UserRole, 0);
	}
	// we didn't find item, create and add new
	QStandardItem* groupItem = NULL;
	if (shortcutItem == NULL)
	{
		// firstly search for group
		QVariant groupId(shortcut->getGroup()->getId());
		groupItem = findItemByData(groupId, Qt::UserRole, 0);
		if (groupItem == NULL)
		{
			// create and add new group to treeWidget
			groupItem = updateGroup(shortcut->getGroup());
		}
		// create shortcut item
		shortcutItem = new QStandardItem();
		shortcutItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		groupItem->appendRow(shortcutItem);
		// store shortcut id, so we can find it when shortcut changed
		shortcutItem->setData(shortcutId, Qt::UserRole);
		QStandardItem* primaryItem = new QStandardItem();
		QStandardItem* secondaryItem = new QStandardItem();
		primaryItem->setFlags(Qt::ItemIsEnabled);
		secondaryItem->setFlags(Qt::ItemIsEnabled);
		groupItem->setChild(shortcutItem->row(), 1, primaryItem);
		groupItem->setChild(shortcutItem->row(), 2, secondaryItem);
	}
	// setup properties of item
	shortcutItem->setText(q_(shortcut->getText()));
	QModelIndex index = shortcutItem->index();
	mainModel->setData(index.sibling(index.row(), 1),
	                   shortcut->getPrimaryKey(), Qt::DisplayRole);
	mainModel->setData(index.sibling(index.row(), 2),
	                   shortcut->getAltKey(), Qt::DisplayRole);
}

void ShortcutsDialog::restoreDefaultShortcuts()
{
	initModel();
	shortcutMgr->restoreDefaultShortcuts();
	updateTreeData();
	initEditors();
}

void ShortcutsDialog::updateTreeData()
{
	// Create shortcuts tree
	QList<StelShortcutGroup*> groups = shortcutMgr->getGroupList();
	foreach (StelShortcutGroup* group, groups)
	{
		updateGroup(group);
		// display group's shortcuts
		QList<StelShortcut*> shortcuts = group->getActionList();
		foreach (StelShortcut* shortcut, shortcuts)
		{
			updateShortcutsItem(shortcut);
		}
	}
	updateText();
}

bool ShortcutsDialog::itemIsEditable(QStandardItem *item)
{
	if (item == NULL) return false;
	// non-editable items(not group items) have no Qt::ItemIsSelectable flag
	return (Qt::ItemIsSelectable & item->flags());
}

void ShortcutsDialog::initModel()
{
	mainModel->blockSignals(true);
	mainModel->clear();
	// TODO: Check if the translation idea will work.
	QStringList headerLabels;
	headerLabels << N_("Action")
	             << N_("Primary shortcut")
	             << N_("Alternative shortcut");
	mainModel->setHorizontalHeaderLabels(headerLabels);
	mainModel->blockSignals(false);
}
