# =================================================
# * This file is part of the TTK Music Player project
# * Copyright (C) 2015 - 2020 Greedysky Studio
#
# * This program is free software; you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as published by
# * the Free Software Foundation; either version 3 of the License, or
# * (at your option) any later version.
#
# * This program is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU General Public License for more details.
#
# * You should have received a copy of the GNU General Public License along
# * with this program; If not, see <http://www.gnu.org/licenses/>.
# =================================================

include($$PWD/../../../TTKVersion.pri)
unix:VERSION += 1.0.0.0

QT       += core gui
equals(QT_MAJOR_VERSION, 5){
QT       += widgets
}

win32:msvc{
    CONFIG +=c++11
}else{
    QMAKE_CXXFLAGS += -std=c++11
}

TARGET = toolsurl
TEMPLATE = app
DEFINES += TTK_LIBRARY

INCLUDEPATH += \
    $$PWD/../../../ \
    $$PWD/../../../TTKThirdParty \
    $$PWD/../../../TTKThirdParty/TTKDumper \
    $$PWD/../../../TTKModule/TTKCore/musicCoreKits \
    $$PWD/../../../TTKModule/TTKCore/musicUtilsKits

LIBS += -L$$OUT_PWD/../../../bin/$$TTKMusicPlayer -lTTKCore
unix:LIBS += -L$$OUT_PWD/../../../bin/$$TTKMusicPlayer -lqmmp -lTTKUi -lTTKExtras -lTTKWatcher -lzlib -lTTKZip

SOURCES += \
    mainurl.cpp\
    toolsurl.cpp

HEADERS  += \
    toolsurl.h

FORMS   += \
    toolsurl.ui
