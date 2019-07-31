QT += quick
#QT += opengl
CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Refer to the documentation for the
# deprecated API to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        flyricconfigmanager.cpp \
        flyricwindowthread.cpp \
        glad/src/glad.c \
        main.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=

HEADERS += \
    flyricconfigmanager.h \
    flyricwindowthread.h \
    glenv.h

# This is glfw library
win32: LIBS += -L$$PWD/glfw/lib-mingw-w64/ -lglfw3 -lgdi32

INCLUDEPATH += $$PWD/glfw/include
DEPENDPATH += $$PWD/glfw/include

win32: LIBS += -lopengl32
# This is glad library
INCLUDEPATH += glad/include

# freetype2
DEFINES += FT2_BUILD_LIBRARY
INCLUDEPATH += freetype2/include
#-- base components
SOURCES += \
  freetype2/src/base/ftsystem.c freetype2/src/base/ftinit.c freetype2/src/base/ftdebug.c freetype2/src/base/ftbase.c freetype2/src/base/ftbbox.c freetype2/src/base/ftglyph.c freetype2/src/base/ftbdf.c freetype2/src/base/ftbitmap.c freetype2/src/base/ftcid.c freetype2/src/base/ftfstype.c freetype2/src/base/ftgasp.c freetype2/src/base/ftgxval.c freetype2/src/base/ftmm.c freetype2/src/base/ftotval.c freetype2/src/base/ftpatent.c freetype2/src/base/ftpfr.c freetype2/src/base/ftstroke.c freetype2/src/base/ftsynth.c freetype2/src/base/fttype1.c freetype2/src/base/ftwinfnt.c freetype2/src/base/ftmac.c
#-- font drivers
SOURCES += \
  freetype2/src/bdf/bdf.c freetype2/src/cff/cff.c freetype2/src/cid/type1cid.c freetype2/src/pcf/pcf.c freetype2/src/pfr/pfr.c freetype2/src/sfnt/sfnt.c freetype2/src/truetype/truetype.c freetype2/src/type1/type1.c freetype2/src/type42/type42.c freetype2/src/winfonts/winfnt.c
#-- rasterizers
SOURCES += \
  freetype2/src/raster/raster.c freetype2/src/smooth/smooth.c
#-- auxiliary modules
SOURCES += \
  freetype2/src/autofit/autofit.c freetype2/src/cache/ftcache.c freetype2/src/gzip/ftgzip.c freetype2/src/lzw/ftlzw.c freetype2/src/bzip2/ftbzip2.c freetype2/src/gxvalid/gxvalid.c freetype2/src/otvalid/otvalid.c freetype2/src/psaux/psaux.c freetype2/src/pshinter/pshinter.c freetype2/src/psnames/psnames.c
# flyric_rendergl
INCLUDEPATH += flyric_rendergl/src
SOURCES += flyric_rendergl/src/flyric_rendergl.c
# flyric_rendergl/flyric_parser
INCLUDEPATH += flyric_rendergl/flyric_parser/include
SOURCES += flyric_rendergl/flyric_parser/src/fparser.c flyric_rendergl/flyric_parser/src/fparser_platform.c flyric_rendergl/flyric_parser/src/frp_bison.tab.c flyric_rendergl/flyric_parser/src/lex.frp_bison.c
