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

#ifndef H2RENAME_H
#define H2RENAME_H

#include <QtGui/QDialog>
#include <QThread>
#include <QMutex>
#include <QSortFilterProxyModel>
#include "ui_h2rename.h"
#include "ui_ReadDirProgress.h"

class RenameRule
{
public:
	RenameRule(const QString &s1 = QString(), const QString &s2 = QString())
        { column[0] = s1; column[1] = s2; highlight = false;}
	enum Columns {Search = 0, Replace, NCOLUMNS};
	QString column[NCOLUMNS];
	bool highlight;
};


class RenameRulesModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	static RenameRulesModel& globalInstance() // Singleton
	{
		static RenameRulesModel renameRulesModel;
		return renameRulesModel;
	}
	bool containsSearchString(const QString &s) const;
	void appendRules(const QList<RenameRule> &rules);
	QModelIndex appendCreatedRule(const QString &text);
	QModelIndex prependCreatedRule(const QString &text);
	void setHighlights(const QVector<bool> &highlights);
	void clearHighlights();
	void clear();

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;
	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role);
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	bool insertRow(int row, const QModelIndex &parent = QModelIndex());

	void removeRowList(const QVector<bool> &rowlist);
	const QString& searchAt(int row) const;
	const QString& replaceAt(int row) const;

signals:
	void rulesChanged();

private:
	RenameRulesModel(QObject *parent = 0); // Singleton
	RenameRulesModel(RenameRulesModel const&);
	RenameRulesModel& operator=(RenameRulesModel const&);

	QList<RenameRule> rules;
};


class Name
{
	Q_DECLARE_TR_FUNCTIONS(Name)
public:
	enum Type {Undefined, File, Directory, Application};

	Name(const QString& name = QString(), Type type = Undefined,
		 const QString &path = QString(), const QString &newname = QString())
		: name(name), type(type), path(path), newname(newname), isChanged(false), isUnique(true)
    {}
	QString typeString(void) const
	{
		switch (type)
		{
			case Directory:
				return tr("Verzeichnis");
			case Application:
				return tr("Anwendung");
			default:
				return tr("Datei");
		}
	};
	QString name;
	Type type;
	QString path;
	QString newname;
	bool isChanged;
	bool isUnique;
	QVector<bool> usedRule;
};


class Directory
{
public:
	Directory(const QString& path = QString())
		: path(path), nChanged(0), nCollisions(0)
    {}

	QVector<Name> names;
	QString path;
	int nChanged;
	int nCollisions;
};


class NamesModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	static NamesModel& globalInstance() // Singleton
	{
		static NamesModel namesModel;
		return namesModel;
	}
	enum Columns {OldName = 0, NewName, NCOLUMNS};
	void setDirectories(const QVector<Directory> &directories);
	void clear();
	void setHighlights(const QVector<bool> &highlights);
	void clearHighlights();

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	bool isChanged(const QModelIndex &index) const;
	bool isCollision(const QModelIndex &index) const;
	bool isHighlight(const QModelIndex &index) const;
	int changedNamesCount() const;
	int collisionCount() const;
	const Name& constNameAt(int row) const;
	Name& nameAt(int row);

public slots:
	void computeNewNames();

signals:
	void collisionDetected(const QModelIndex &firstCollision);
	void highlightsChanged();

private:
	NamesModel(QObject *parent = 0);
	NamesModel(NamesModel const&); // Singleton
	NamesModel& operator=(NamesModel const&);

	QVector<Directory> directories;
	QVector<int> rowOffset; // one int per directory: row# for dir.names.at(0)
	int namesCount; // total number of names in all directories
	int nChanged;
	int nCollisions;
	QVector<bool> highlights;
};


class NamesFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

public:
	static NamesFilterProxyModel& globalInstance() // Singleton
	{
		static NamesFilterProxyModel namesFilterProxyModel;
		return namesFilterProxyModel;
	}
	enum Type {ShowAll = 0, ShowChanged, ShowHighlights, ShowCollisions};

    Type filterType() {return curFilter;}

public slots:
    void setFilterType(Type filterType) {curFilter = filterType; filterChanged();}
	void highlightsChanged();

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
	NamesFilterProxyModel() {curFilter = ShowAll; }
	NamesFilterProxyModel(NamesFilterProxyModel const&); // Singleton
	NamesFilterProxyModel& operator=(NamesFilterProxyModel const&);

	Type curFilter;
};


class ReadDirThread : public QThread
{
	Q_OBJECT

public:
//	ReadDirThread();
//	~ReadDirThread();

	void setRootPath(const QString &path, bool recursive)
		{ rootPath = path; this->recursive = recursive; }
	QString errorMessage() const {return error;}
	void abort();

	QVector<Directory> directories;
	QMap<QString, QString> replacements;

signals:
	void processing(QString path);

protected:
	void run();

private:
	QString rootPath;
	bool recursive;
	QString error;
	volatile bool aborted;
	QMutex m_aborted;

	bool readDirectory(const QString &path);
	void analyseNames();
	void analyseName(const QString &s);

};


class H2rename : public QWidget
{
	Q_OBJECT

public:
	H2rename(QWidget *parent = 0, Qt::WFlags flags = 0);
	~H2rename();
	enum Colors {CollisionColor = 0xff0000, HighlightColor = 0xffe489, ErrorColor = 0xff0000};
private:

	Ui::H2renameClass ui;
	QAction *insertRuleAction;
	QAction *removeRulesAction;
	QAction *createRuleAction;
	ReadDirThread readDirThread;
	QList<Name*> names;
	QMap<QString, QString> replacements;

private:
	void readDirectory(const QString &path, bool recursive, Name *parent);

public:
	static QChar parseUTF8Char(QString::const_iterator &p, QString::const_iterator end);

public slots:
	void updateNumbers();
	void createRule();

private slots:
	void on_pushButton_Load_clicked();
 void on_comboBox_currentIndexChanged(int);
	void replacementsSelectionChanged();
	void namesSelectionChanged();
	void collisionDetected(const QModelIndex &);
	void on_pushButton_Rename_clicked();
	void on_lineEdit_DirName_returnPressed();
	void on_pushButton_removeRow_clicked();
	void on_pushButton_insertRow_clicked();
	void readDirs();
	void on_pushButton_SelectDir_clicked();
};

class ReadDirProgressDialog : public QDialog
{
	Q_OBJECT

public:
	ReadDirProgressDialog(QWidget *parent = 0, Qt::WFlags flags = 0);

	Ui::ReadDirProgressDialog ui;
};


bool isLongerThan(const QString &s, const QString &t);

#endif // H2RENAME_H
