#pragma once

namespace CodeAtlas
{

class UIManager:public QObject
{
	Q_OBJECT
public:
	UIManager(void);
	~UIManager(void);

	static UIManager* instance();
	static CodeScene& getScene();
	MainUI&    getMainUI(){return m_mainUI;}

	void	   lockAll();
	void	   unlockAll();

public slots:
	void       updateScene();				// update scene when db updated
private:
	static UIManager* m_instance;

	MainUI            m_mainUI;
	static CodeScene* m_scene;
};

}
