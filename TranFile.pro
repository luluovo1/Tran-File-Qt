QT       += core gui network concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += static
CONFIG -= qt_debug      # 禁用调试符号[5](@ref)

RC_ICONS = icon.ico
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

QMAKE_CXXFLAGS_RELEASE += -O2  # 平衡优化（推荐默认）

QMAKE_LFLAGS += -Wl,--gc-sections   # 移除未使用的代码段
QMAKE_LFLAGS += -Wl,--as-needed     # 仅链接实际需要的库
QMAKE_LFLAGS += -s                  # 去除符号表和调试信息[5](@ref)

SOURCES += \
    discoverservice.cpp \
    main.cpp \
    mainwindow.cpp \
    receivefile.cpp \
    sendfile.cpp \
    toast.cpp

HEADERS += \
    discoverservice.h \
    mainwindow.h \
    receivefile.h \
    sendfile.h \
    toast.h

FORMS += \
    mainwindow.ui

LIBS += -lz


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    setting.qrc

DISTFILES += \
    icon.ico
