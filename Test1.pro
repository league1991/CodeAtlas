TEMPLATE = lib 
TARGET = Test1
 
include(../../../qtcreatorplugin.pri) 
DESTDIR = $$IDE_PLUGIN_PATH/MyPlugin/Test1
PROVIDER = Me
include(../../plugins/coreplugin/coreplugin_dependencies.pri) 
 
HEADERS += test1.h 
SOURCES += test1.cpp 
 
OTHER_FILES += Test1.pluginspec