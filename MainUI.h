#pragma once
#include "GeneratedFiles/ui_mainUI.h"
namespace CodeAtlas
{

class MainUI:public QMainWindow
{
	Q_OBJECT
public:
	MainUI(void);
	~MainUI(void);

	void   lockUI();
	void   unlockUI();
public slots:
	void   resetDB();
	void   onActivate(bool isActivate);
private:
	Ui::MainUI			m_ui;
	void buildConnections();

};

}
