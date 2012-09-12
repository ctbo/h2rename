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

#include <QtGui/QApplication>
#include <QTranslator>
#include "h2rename.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QTranslator appTranslator;
	appTranslator.load("h2rename_" + QLocale::system().name(), ":/");
	a.installTranslator(&appTranslator);

	QTranslator qtTranslator;
	qtTranslator.load("qt_" + QLocale::system().name(), ":/");
	a.installTranslator(&qtTranslator);

	H2rename w;
	w.show();
	return a.exec();
}
