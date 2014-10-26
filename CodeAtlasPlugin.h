#pragma once

#ifndef CODEATLASPLUGIN_H 
#define CODEATLASPLUGIN_H 

#include <extensionsystem/iplugin.h> 
namespace CodeAtlas
{
class CodeAtlasPlugin : public ExtensionSystem::IPlugin 
{ 
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CodeAtlas.json")
public: 
	CodeAtlasPlugin(); 
	~CodeAtlasPlugin(); 

	void extensionsInitialized(); 
	bool initialize(const QStringList & arguments, QString * errorString); 
	void shutdown(); 

private slots:
	void getClassInfo();

private:
	void buildConnections();
}; 
}
#endif 