#include "StdAfx.h"
#include "UIManager.h"

using namespace CodeAtlas;

CodeScene* UIManager::m_scene		= NULL;
UIManager* UIManager::m_instance	= NULL;

UIManager::UIManager(void)
{
	m_instance = this;
}

UIManager::~UIManager(void)
{
	m_instance = NULL;
	delete m_scene;
}

UIManager* UIManager::instance()
{
	return m_instance;
}


void CodeAtlas::UIManager::updateScene()
{
	//Locator& locator = DBManager::instance()->getDB().getVocabularyCollector();
	//m_scene.updateSceneData(locator.getProjectData());
	getScene().update();
}

void CodeAtlas::UIManager::lockAll()
{
	getScene().lockScene();
	m_mainUI.lockUI();
}

void CodeAtlas::UIManager::unlockAll()
{
	m_mainUI.unlockUI();
	getScene().unlockScene();
}

CodeScene& CodeAtlas::UIManager::getScene()
{
	if (m_scene == NULL)
	{
		m_scene = new CodeScene;
		m_scene->init();
	}
	return *m_scene;
}
