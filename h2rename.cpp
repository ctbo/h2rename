/*
	This file is part of H2rename.

	Copyright (C) 2009 by Harald Bögeholz / c't Magazin für Computertechnik
	www.ctmagazin.de

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "h2rename.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <qtconcurrentmap.h>
#include <QTime>
#include <QThreadPool>
#include <QtAlgorithms>
#include <QProgressDialog>

/*
#include <iostream>
using namespace std;
*/

bool isLongerThan(const QString &s, const QString &t)
{
	return s.length() > t.length();
}


RenameRulesModel::RenameRulesModel(QObject *parent)
	: QAbstractTableModel(parent)
{
}


bool RenameRulesModel::containsSearchString(const QString &s) const
{
	for (QList<RenameRule>::const_iterator p = rules.constBegin(); p != rules.constEnd(); ++p)
		if (p->column[RenameRule::Search] == s)
			return true;
	return false;
}


void RenameRulesModel::appendRules(const QList<RenameRule> &rules)
{
	for (QList<RenameRule>::const_iterator p = rules.constBegin(); p != rules.constEnd(); ++p)
	{
		if (!containsSearchString(p->column[RenameRule::Search]))
		{
			this->rules.append(*p);
			//nColumns = 3;
		}
	}
	reset();
	emit rulesChanged();
}

void RenameRulesModel::setHighlights(const QVector<bool> &highlights)
{
	for (int i=0; i<rules.count(); ++i)
		if (i<highlights.count() && highlights.at(i))
			rules[i].highlight = true;
		else
			rules[i].highlight = false;
	emit dataChanged(createIndex(0, 0), createIndex(rules.count(), 1));
}

void RenameRulesModel::clearHighlights()
{
	for (int i=0; i<rules.count(); ++i)
		rules[i].highlight = false;
	emit dataChanged(createIndex(0, 0), createIndex(rules.count(), 1));
}

void RenameRulesModel::clear()
{
	rules.clear();
	reset();
	emit rulesChanged();
}

int RenameRulesModel::rowCount(const QModelIndex & /* parent */) const
{
	return rules.count() + 1;
}

int RenameRulesModel::columnCount(const QModelIndex & /* parent */ ) const
{
	return RenameRule::NCOLUMNS;
}

QVariant RenameRulesModel::data(const QModelIndex &index, int role) const
{
	if (index.isValid())
	{
		switch(role)
		{
		case Qt::TextAlignmentRole:
			return int(Qt::AlignLeft | Qt::AlignVCenter);

		case Qt::DisplayRole:
		case Qt::EditRole:
			if (index.row() >= rules.count())
				return QString();
			else
				return rules.at(index.row()).column[index.column()];

		case Qt::ForegroundRole:
			if (index.row() < rules.count() && 
				(rules.at(index.row()).column[RenameRule::Search].isEmpty() ||
				rules.at(index.row()).column[RenameRule::Replace].contains(QChar('\\')) ||
				rules.at(index.row()).column[RenameRule::Replace].contains(QChar('/')) ||
				rules.at(index.row()).column[RenameRule::Replace].contains(QChar(':'))
				))
				return QColor(H2rename::ErrorColor);
			break;

		case Qt::ToolTipRole:
			if (index.row() < rules.count() && rules.at(index.row()).column[RenameRule::Search].isEmpty())
				return tr("ACHTUNG: Suchstring leer!");
			if (index.row() < rules.count() && rules.at(index.row()).column[RenameRule::Replace].contains(QChar('\\')))
				return tr("ACHTUNG: Ersetzungsstring enthält einen Backslash (\\)");
			if (index.row() < rules.count() && rules.at(index.row()).column[RenameRule::Replace].contains(QChar('/')))
				return tr("ACHTUNG: Ersetzungsstring enthält einen Schrägstrich (/)");
			if (index.row() < rules.count() && rules.at(index.row()).column[RenameRule::Replace].contains(QChar(':')))
				return tr("ACHTUNG: Ersetzungsstring enthält einen Doppelpunkt!");
			break;

		case Qt::BackgroundRole:
			if (index.row() < rules.count() && rules.at(index.row()).highlight)
				return QColor(H2rename::HighlightColor);
			break;
		}
	}
	return QVariant();
}

bool RenameRulesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (index.isValid() && role == Qt::EditRole)
	{
		if (index.row() >= rules.count())
		{
			beginInsertRows(QModelIndex(), index.row()+1, index.row()+1);
			rules.append(RenameRule());
			endInsertRows();
		}
		rules[index.row()].column[index.column()] = value.toString();

		emit dataChanged(index, index);
		emit rulesChanged();

		return true;
	}
	return false;
}


QVariant RenameRulesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
		return (QStringList() << tr("Ersetze") << tr("durch") << tr("Regel automatisch erzeugt aus")).at(section);
	return QVariant();
}


Qt::ItemFlags RenameRulesModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = QAbstractTableModel::flags(index);
	if (index.column() == RenameRule::Search || index.column() == RenameRule::Replace)
		flags |= Qt::ItemIsEditable;
	return flags;
}

bool RenameRulesModel::insertRow(int row, const QModelIndex &parent)
{
	if (row >= 0 && row <= rules.size())
	{
		beginInsertRows(parent, row, row);
		rules.insert(row, RenameRule());
		endInsertRows();
		return true;
	}
	return false;
}

QModelIndex RenameRulesModel::appendCreatedRule(const QString &text)
{
	RenameRule newRule(text, text);
	beginInsertRows(QModelIndex(), rules.count(), rules.count());
	rules.append(newRule);
	endInsertRows();
	emit rulesChanged();
	return index(rules.count()-1, 0);
}

QModelIndex RenameRulesModel::prependCreatedRule(const QString &text)
{
	RenameRule newRule(text, text);
	beginInsertRows(QModelIndex(), 0, 0);
	rules.insert(0, newRule);
	endInsertRows();
	emit rulesChanged();
	return index(0, 0);
}

void RenameRulesModel::removeRowList(const QVector<bool> &rowlist)
{
	for (int i=rowlist.count()-1; i>=0; --i)
		if (rowlist.at(i) && i<rules.count())
			rules.removeAt(i);
	reset();
	emit rulesChanged();
}


const QString& RenameRulesModel::searchAt(int row) const
{
	return rules.at(row).column[RenameRule::Search];
}

const QString& RenameRulesModel::replaceAt(int row) const
{
	return rules.at(row).column[RenameRule::Replace];
}


NamesModel::NamesModel(QObject *parent)
: QAbstractTableModel(parent), namesCount(0), nChanged(0), nCollisions(0)
{
}


void NamesModel::setDirectories(const QVector<Directory> &directories)
{
	this->directories = directories;
	rowOffset.clear();
	int row = 0;
	for (QVector<Directory>::const_iterator p = directories.constBegin(); p != directories.constEnd(); ++p)
	{
		rowOffset.append(row);
		row += p->names.count();
	}
	namesCount = row;

	computeNewNames();
	reset();
}

void NamesModel::clear()
{
	directories.clear();
	rowOffset.clear();
	namesCount = 0;
	nChanged = 0;
	nCollisions = 0;
	reset();
}

void NamesModel::setHighlights(const QVector<bool> &highlights)
{
	this->highlights = highlights;
	if (namesCount > 0)
	{
		emit dataChanged(index(0, 0), index(namesCount-1, 1));
		emit highlightsChanged();
	}
}

void NamesModel::clearHighlights()
{
	highlights.clear();
	if (namesCount > 0)
		emit dataChanged(index(0, 0), index(namesCount-1, 1));
}

int NamesModel::rowCount(const QModelIndex & /* parent */) const
{
	return namesCount;
}


int NamesModel::columnCount(const QModelIndex & /* parent */) const
{
	return 2;
}


QVariant NamesModel::data(const QModelIndex &index, int role) const
{
	if (index.isValid())
	{
		switch (role)
		{
		case Qt::TextAlignmentRole:
			return int(Qt::AlignLeft | Qt::AlignVCenter);
		case Qt::DisplayRole:
			switch (index.column())
			{
			case 0:
				return constNameAt(index.row()).name;
			case 1:
				return constNameAt(index.row()).newname;
			default:
				return tr("FIXME");
			}
		case Qt::ToolTipRole:
			return (constNameAt(index.row()).typeString())
				+ " in " + constNameAt(index.row()).path;
		case Qt::BackgroundRole:
			if (isCollision(index))
				return QColor(H2rename::CollisionColor);
			if (isHighlight(index))
				return QColor(H2rename::HighlightColor);
			break;
		case Qt::UserRole:
			return isHighlight(index);
		}
	}
	return QVariant();
}

QVariant NamesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
		return (QStringList() << tr("Originalname") << tr("Neuer Name")).at(section);
	return QVariant();
}

void applyRename(Directory &dir)
{
	QSet<QString> uniqueNames;
	dir.nChanged = dir.nCollisions = 0;
	for (QVector<Name>::iterator p = dir.names.begin(); p != dir.names.end(); ++p)
	{
		//p->newname = RenameRulesModel::globalInstance().applyRules(p->name);
		p->newname = p->name;
		p->usedRule.resize(RenameRulesModel::globalInstance().rowCount(QModelIndex()) - 1);
		for (int i=0; i<RenameRulesModel::globalInstance().rowCount(QModelIndex()) - 1; ++i)
		{
			const QString &search = RenameRulesModel::globalInstance().searchAt(i);
			const QString &replace = RenameRulesModel::globalInstance().replaceAt(i);
			if (!search.isEmpty() && p->newname.contains(search))
			{
				p->newname = p->newname.replace(search, replace);
				p->usedRule[i] = true;
			}
			else
				p->usedRule[i] = false;
		}

		p->isChanged = (p->name != p->newname);
		if (p->isChanged)
			++dir.nChanged;
		if (uniqueNames.contains(p->newname))
		{
			p->isUnique = false;
			++dir.nCollisions;
		}
		else
		{
			p->isUnique = true;
			uniqueNames.insert(p->newname);
		}
	}
}

bool NamesModel::isChanged(const QModelIndex &index) const
{
	if (index.isValid())
		return constNameAt(index.row()).isChanged;
	return false;
}

bool NamesModel::isCollision(const QModelIndex &index) const
{
	if (index.isValid())
		return !constNameAt(index.row()).isUnique;
	return false;
}

bool NamesModel::isHighlight(const QModelIndex &index) const
{
	if (index.isValid())
	{
		const Name &name = constNameAt(index.row());
		for (int i=0; i<highlights.count() && i < name.usedRule.count(); ++i)
			if (highlights.at(i) && name.usedRule.at(i))
				return true;
	}
	return false;
}

int NamesModel::changedNamesCount() const
{
	return nChanged;
}

int NamesModel::collisionCount() const
{
	return nCollisions;
}

void NamesModel::computeNewNames()
{
	if (namesCount > 0)
	{
		QApplication::setOverrideCursor(Qt::WaitCursor);

		QtConcurrent::blockingMap(directories, applyRename);

		int firstCollision = -1;
		nChanged = nCollisions = 0;
		for (QVector<Directory>::const_iterator p = directories.constBegin(); p != directories.constEnd(); ++p)
		{
			nChanged += p->nChanged;
			nCollisions += p->nCollisions;
			if (firstCollision == -1 && p->nCollisions > 0)
			{	// find first collision
				for (int i=0; i<p->names.count(); ++i)
					if (!p->names.at(i).isUnique)
					{
						firstCollision = i + rowOffset.at(p - directories.constBegin());
						break;
					}
			}

		}

		emit dataChanged(index(0, 1, QModelIndex()), index(namesCount-1, 1, QModelIndex()));
		NamesFilterProxyModel::globalInstance().invalidate();
		if (nCollisions > 0)
			emit collisionDetected(index(firstCollision,1));
		QApplication::restoreOverrideCursor();
	}
}

const Name& NamesModel::constNameAt(int row) const
{
	QVector<int>::const_iterator p = qUpperBound(rowOffset.constBegin(), rowOffset.constEnd(), row);
	if (p == rowOffset.constEnd() || *p > row)
		--p; // müsste eigentlich jedes Mal passieren
	int i = p - rowOffset.constBegin();
	int j = row - *p;
	return directories.at(i).names.at(j);
}

Name& NamesModel::nameAt(int row)
{
	QVector<int>::const_iterator p = qUpperBound(rowOffset.constBegin(), rowOffset.constEnd(), row);
	if (p == rowOffset.constEnd() || *p > row)
		--p; // müsste eigentlich jedes Mal passieren
	int i = p - rowOffset.constBegin();
	int j = row - *p;
	return directories[i].names[j];
}

void NamesFilterProxyModel::highlightsChanged()
{
	if (curFilter == ShowHighlights)
		filterChanged();
}

bool NamesFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
	QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
	switch (curFilter)
	{
	case ShowChanged:
		return static_cast<NamesModel*>(sourceModel())->isChanged(index);
	case ShowHighlights:
		return static_cast<NamesModel*>(sourceModel())->isHighlight(index);
	case ShowCollisions:
		return static_cast<NamesModel*>(sourceModel())->isCollision(index);
	default:
		;
	}
	return true;
}


H2rename::H2rename(QWidget *parent, Qt::WFlags flags)
	: QWidget(parent, flags)
{
	ui.setupUi(this);
	
	ui.tableView_replacements->setModel(&RenameRulesModel::globalInstance());
	ui.tableView_replacements->verticalHeader()->hide();
	ui.tableView_replacements->horizontalHeader()->setStretchLastSection(true);

	NamesFilterProxyModel::globalInstance().setSourceModel(&NamesModel::globalInstance());

	ui.tableView_names->setModel(&NamesFilterProxyModel::globalInstance());
	ui.tableView_names->verticalHeader()->hide();
	ui.tableView_names->horizontalHeader()->setStretchLastSection(true);

	insertRuleAction = new QAction(tr("Regel &einfügen"), this);
	insertRuleAction->setShortcut(tr("Ctrl+E"));
	insertRuleAction->setIconVisibleInMenu(false);
	connect(insertRuleAction, SIGNAL(triggered()),
		this, SLOT(on_pushButton_insertRow_clicked()));
	ui.tableView_replacements->addAction(insertRuleAction);

	removeRulesAction = new QAction(tr("Regel(n) &löschen"), this);
	removeRulesAction->setShortcut(tr("Ctrl+L"));
	removeRulesAction->setIconVisibleInMenu(false);
	connect(removeRulesAction, SIGNAL(triggered()),
		this, SLOT(on_pushButton_removeRow_clicked()));
	ui.tableView_replacements->addAction(removeRulesAction);

	createRuleAction = new QAction(tr("&Regel erzeugen"), this);
	createRuleAction->setShortcut(tr("Ctrl+R"));
	createRuleAction->setIconVisibleInMenu(false);
	connect(createRuleAction, SIGNAL(triggered()),
		this, SLOT(createRule()));
	ui.tableView_names->addAction(createRuleAction);

	connect(&RenameRulesModel::globalInstance(), SIGNAL(rulesChanged()),
		&NamesModel::globalInstance(), SLOT(computeNewNames()));
	connect(&RenameRulesModel::globalInstance(), SIGNAL(rowsRemoved (const QModelIndex &, int, int) ),
		&NamesModel::globalInstance(), SLOT(computeNewNames()));
	connect(&NamesModel::globalInstance(), SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
		this, SLOT(updateNumbers()));
	connect(ui.tableView_names->selectionModel(), SIGNAL(selectionChanged (const QItemSelection &, const QItemSelection &)),
		this, SLOT(namesSelectionChanged()));
	connect(ui.tableView_replacements->selectionModel(), SIGNAL(selectionChanged (const QItemSelection &, const QItemSelection &)),
		this, SLOT(replacementsSelectionChanged()));
	connect(&NamesModel::globalInstance(), SIGNAL(collisionDetected(const QModelIndex &)),
		this, SLOT(collisionDetected(const QModelIndex &)));
	connect(&NamesModel::globalInstance(), SIGNAL(highlightsChanged()),
		&NamesFilterProxyModel::globalInstance(), SLOT(highlightsChanged()));
}

H2rename::~H2rename()
{

}

QChar H2rename::parseUTF8Char(QString::const_iterator &p, QString::const_iterator end)
{
	if (p == end)
		return QChar();

	QString::const_iterator q=p;
	ushort decoded;
	
	if (q->unicode() < 0x80 || q->unicode() >= 0x100)
		return QChar();
	else if ((q->unicode() & 0xffe0) == 0xc0)
	{	// two byte character
		decoded = (q->unicode() & 0x1f) << 6;
		if (++q == end ||(q->unicode() & 0xffc0) != 0x80)
			return QChar();
		decoded |= q->unicode() & 0x3f;
		p = ++q;
		return QChar(decoded);
	}
	else if ((q->unicode() & 0xfff0) == 0xe0)
	{
		// three byte character
		decoded = (q->unicode() & 0x0f) << 12;
		if (++q == end || (q->unicode() & 0xffc0) != 0x80)
			return QChar();
		decoded |= (q->unicode() & 0x3f) << 6;
		if (++q == end || (q->unicode() & 0xffc0) != 0x80)
			return QChar();
		decoded |= q->unicode() & 0x3f;
		p = ++q;
		return QChar(decoded);
	}
	else if ((p->unicode() & 0xf0) == 0xf0)
	{	// four byte charater not implemented FIXME
		return QChar();
	}
	return QChar(); // otherwise invalid
}


void H2rename::updateNumbers()
{
	int nNames = NamesModel::globalInstance().rowCount();
	int nChanges = NamesModel::globalInstance().changedNamesCount();
	int nCollisions = NamesModel::globalInstance().collisionCount();
	ui.label_numbers->setText(tr("%1 %2, %3 %4").arg(nNames).arg(nNames==1?tr("Name"):tr("Namen")).arg(nChanges).arg(nChanges==1?tr("Änderung"):tr("Änderungen")));
	ui.pushButton_Rename->setEnabled(nChanges - nCollisions > 0);
	if (NamesModel::globalInstance().collisionCount() > 0)
		ui.label_collisions->setText(tr("%1 %2!").arg(NamesModel::globalInstance().collisionCount())
		.arg(NamesModel::globalInstance().collisionCount() == 1 ? tr("Namenskollision") : tr("Namenskollisionen")));
	else
		ui.label_collisions->setText("");
}


void ReadDirThread::run()
{
	{
		QMutexLocker locker(&m_aborted);
		aborted = false;
	}
	error.clear();
	directories.clear();
	replacements.clear();

	readDirectory(rootPath);
	analyseNames();
}

void ReadDirThread::abort()
{
	QMutexLocker locker(&m_aborted);
	aborted = true;
}


bool ReadDirThread::readDirectory(const QString &path)
{
	{
		QMutexLocker locker(&m_aborted);
		if (aborted)
			return false;
	}
	QDir dir(path);
	if (!dir.exists())
	{
		error = tr("Das Verzeichnis '%1' existiert nicht").arg(path); 
		return false;
	}
	emit processing(path);

	Directory currentDir(path);
	foreach (QFileInfo subdir, dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks))
	{
		if (recursive && !subdir.isBundle())
		{
			if (!readDirectory(path + QDir::separator() + subdir.fileName()))
				return false;
		}
		if (subdir.isBundle())
			currentDir.names.append(Name(subdir.fileName(), Name::Application, path));
		else
			currentDir.names.append(Name(subdir.fileName(), Name::Directory, path));
	}
	foreach (QString file, dir.entryList(QDir::Files))
	{
		currentDir.names.append(Name(file, Name::File, path));
	}
	directories.append(currentDir);

	return true;
}


void ReadDirThread::analyseNames()
{
	QVector<Directory>::const_iterator d;
	for (d = directories.constBegin(); d != directories.constEnd(); ++d)
	{
		QVector<Name>::const_iterator p;
		for (p=d->names.constBegin(); p != d->names.constEnd(); ++p)
			analyseName(p->name);
	}
}


void ReadDirThread::analyseName(const QString &s)
{
	QString::const_iterator p;
	for (p = s.constBegin(); p != s.constEnd(); )
	{
		QString decoded;
		QString::const_iterator start = p;

		QChar c = H2rename::parseUTF8Char(p, s.constEnd());
		if (!c.isNull())
		{
			do
			{
				decoded += c;
				c = H2rename::parseUTF8Char(p, s.constEnd());
			} while (!c.isNull());
			for (;;)
			{
				QString::const_iterator q;
				QString dedecoded;
				QChar cc;
				for (q = decoded.constBegin(); !(cc = H2rename::parseUTF8Char(q, decoded.constEnd())).isNull();)
					dedecoded += cc;
				if (q == decoded.constEnd())
					decoded = dedecoded;
				else
					break;
			}
			QString replace;
			for (QString::const_iterator q=start; q != p; ++q)
				replace += *q;
			if (!replacements.contains(replace))
			{
				replacements[replace] = decoded;
			}
		}
		else
			++p;
	}
			
}


void H2rename::on_pushButton_SelectDir_clicked()
{
	QString path = QFileDialog::getExistingDirectory(this, tr("Verzeichnis wählen"), 
		ui.lineEdit_DirName->text(), QFileDialog::ShowDirsOnly);
	ui.lineEdit_DirName->setText(QDir::toNativeSeparators(path));
	if (!path.isEmpty())
		readDirs();
}

void H2rename::readDirs()
{
	if (!readDirThread.isRunning())
	{
		NamesModel::globalInstance().clear();

		ReadDirProgressDialog dialog;
		connect(&readDirThread, SIGNAL(finished()), &dialog, SLOT(accept()));
		connect(&readDirThread, SIGNAL(processing(QString)), dialog.ui.label_currentDir, SLOT(setText(QString)));

		readDirThread.setRootPath(ui.lineEdit_DirName->text(), ui.checkBox_Recursive->isChecked());
		readDirThread.start();
		if (dialog.exec() == QDialog::Accepted)
		{
			if (!readDirThread.errorMessage().isEmpty())
				QMessageBox::warning(this, tr("H2rename"), readDirThread.errorMessage(), QMessageBox::Ok);
			else
			{
				QStringList repstrings = readDirThread.replacements.keys();
				qStableSort(repstrings.begin(), repstrings.end(), isLongerThan);
				QList<RenameRule> rules;
				foreach (QString replace, repstrings)
				{
					rules.append(RenameRule(replace, readDirThread.replacements[replace]));
				}
				if (rules.count() > 0)
				{
					RenameRulesModel::globalInstance().appendRules(rules);
					ui.comboBox->setCurrentIndex(NamesFilterProxyModel::ShowChanged);
					//ui.tableView_replacements->resizeColumnToContents(0);
					//ui.tableView_replacements->resizeColumnToContents(1);
				}

				NamesModel::globalInstance().setDirectories(readDirThread.directories);
			}
		}
		else
		{
			readDirThread.abort();
			readDirThread.wait();
		}
	}
}





void H2rename::on_pushButton_insertRow_clicked()
{
	RenameRulesModel::globalInstance().insertRow(ui.tableView_replacements->currentIndex().row());
}

void H2rename::on_pushButton_removeRow_clicked()
{
	int nRules = RenameRulesModel::globalInstance().rowCount(QModelIndex()) - 1;
	if (nRules > 0)
	{
		QVector<bool> rows(nRules);
		QItemSelectionModel *sel = ui.tableView_replacements->selectionModel();
		for (int i=0; i<nRules; ++i)
			rows[i] = sel->rowIntersectsSelection(i, QModelIndex());
		RenameRulesModel::globalInstance().removeRowList(rows);
		replacementsSelectionChanged();
	}
}


ReadDirProgressDialog::ReadDirProgressDialog(QWidget *parent, Qt::WFlags flags)
: QDialog(parent, flags)
{
	ui.setupUi(this);
}

void H2rename::on_lineEdit_DirName_returnPressed()
{
	readDirs();
}

void H2rename::on_pushButton_Rename_clicked()
{
	int renameCount = NamesModel::globalInstance().changedNamesCount() -
		NamesModel::globalInstance().collisionCount();
	if (renameCount == 0)
		return; // should not happen; button should be inactive

	QString text = tr("Sie sind im Begriff, %1 %2 umzubenennen.").arg(renameCount)
		.arg(renameCount==1 ? tr("Datei/Verzeichnis") : tr("Dateien/Verzeichnisse"));
	if (NamesModel::globalInstance().collisionCount() > 0)
		text += tr(" %1 %2 aufgrund von Namenskollisionen nicht umbenannt.")
		.arg(NamesModel::globalInstance().collisionCount())
		.arg(NamesModel::globalInstance().collisionCount() == 1 ? tr("wird") : tr("werden"));
	text += tr(" Fortfahren?");

	if (QMessageBox::Yes == QMessageBox::warning(this, tr("H2rename - Umbenennen starten?"), text,
		QMessageBox::Yes | QMessageBox::No))
	{
		QProgressDialog progress(this);
		progress.setLabelText(tr("Umbenennen läuft ..."));
		progress.setRange(0, renameCount);
		progress.setModal(true);

		int nDone = 0;
		for (int i=0; i<NamesModel::globalInstance().rowCount(); ++i)
		{
			Name &name = NamesModel::globalInstance().nameAt(i);
			if (name.isChanged && name.isUnique)
			{
				QString oldFullName = name.path + QDir::separator() + name.name;
				QString newFullName = name.path + QDir::separator() + name.newname;
				if (!QFile::rename(oldFullName, newFullName))
				{
					QMessageBox::critical(this, tr("H2rename"), 
						tr("Konnte %1 \"%2\" nicht in \"%3\" umbenennen.")
						.arg(name.typeString())
						.arg(oldFullName)
						.arg(newFullName)
						, QMessageBox::Ok);
					break;
				}
				name.name = name.newname;
				if (name.type == Name::Directory)
				{	// clean up path names for contained files and dirs
					for (int j=0; j<i; ++j)
					{
						Name &jname = NamesModel::globalInstance().nameAt(j);
						if (jname.path.startsWith(oldFullName))
							jname.path = newFullName + jname.path.mid(oldFullName.length());
					}
				}
				++nDone;
				progress.setValue(nDone);
				qApp->processEvents();
				if (progress.wasCanceled())
				{
					break;
				}
			}
		}
		NamesModel::globalInstance().computeNewNames();
	}
}

void H2rename::namesSelectionChanged()
{
	QItemSelectionModel *sel = ui.tableView_names->selectionModel();
	if (sel->hasSelection())
	{
		NamesModel::globalInstance().clearHighlights();
		QModelIndexList li = sel->selection().indexes();
		if (!li.isEmpty())
		{
			const QVector<bool> &highlights = NamesModel::globalInstance().constNameAt(NamesFilterProxyModel::globalInstance().mapToSource(li.at(0)).row()).usedRule;
			RenameRulesModel::globalInstance().setHighlights(highlights);
			ui.tableView_replacements->clearSelection();
			for (int i=0; i<highlights.count(); ++i)
				if (highlights.at(i))
				{
					ui.tableView_replacements->scrollTo(RenameRulesModel::globalInstance().index(i, 0));
					break;
				}
		}
	}
}

void H2rename::replacementsSelectionChanged()
{
	QItemSelectionModel *sel = ui.tableView_replacements->selectionModel();
	if (sel->hasSelection())
	{
		ui.pushButton_removeRow->setEnabled(true);
		RenameRulesModel::globalInstance().clearHighlights();
		ui.tableView_names->clearSelection();
		QVector<bool> selectedRules(RenameRulesModel::globalInstance().rowCount(QModelIndex()));
		for (int i=0; i<RenameRulesModel::globalInstance().rowCount(QModelIndex()); ++i)
			selectedRules[i] = sel->rowIntersectsSelection(i, QModelIndex());
		NamesModel::globalInstance().setHighlights(selectedRules);
		if (NamesFilterProxyModel::globalInstance().filterType() != NamesFilterProxyModel::ShowHighlights)
		{
			for (int i=0; i<NamesFilterProxyModel::globalInstance().rowCount(QModelIndex()); ++i)
				if (NamesFilterProxyModel::globalInstance().data(NamesFilterProxyModel::globalInstance().index(i, 0),Qt::UserRole).toBool())
				{
					ui.tableView_names->scrollTo(NamesFilterProxyModel::globalInstance().index(i, 0));
					break;
				}
		}
	}
	else
		ui.pushButton_removeRow->setEnabled(false);
}

void H2rename::collisionDetected(const QModelIndex &firstCollision)
{
	ui.tableView_names->scrollTo(NamesFilterProxyModel::globalInstance().mapFromSource(firstCollision));
}


void H2rename::on_comboBox_currentIndexChanged(int index)
{
	NamesFilterProxyModel::globalInstance().setFilterType(static_cast<NamesFilterProxyModel::Type>(index));
}

void H2rename::createRule()
{
	QModelIndex index = ui.tableView_names->currentIndex();
	if (index.isValid())
	{
		QString text = NamesFilterProxyModel::globalInstance().data(index).toString();
		if (!text.isEmpty())
		{
			QModelIndex newIndex;
			if (index.column() == NamesModel::OldName)
				newIndex = RenameRulesModel::globalInstance().prependCreatedRule(text);
			else
				newIndex = RenameRulesModel::globalInstance().appendCreatedRule(text);
			ui.tableView_replacements->selectionModel()->select(newIndex,
				QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
			ui.tableView_replacements->scrollTo(newIndex);
			ui.tableView_replacements->setFocus(Qt::OtherFocusReason);
		}
	}
}

void H2rename::on_pushButton_Load_clicked()
{
	readDirs();
}
