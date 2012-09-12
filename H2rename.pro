# H2rename.pro
TEMPLATE = app

# Input
HEADERS += h2rename.h
FORMS += h2rename.ui \
    ReadDirProgress.ui
SOURCES += h2rename.cpp \
    main.cpp
OTHER_FILES += H2rename.nsi \
    license.txt
RC_FILE = H2rename.rc
TRANSLATIONS = h2rename.ts \
	h2rename_de.ts \
	h2rename_nl.ts
RESOURCES += h2rename.qrc
