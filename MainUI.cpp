#include "StdAfx.h"
#include "MainUI.h"
using namespace CodeAtlas;


MainUI::MainUI(void)
{
	m_ui.setupUi(this);

	buildConnections();
	setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}

MainUI::~MainUI(void)
{
}


void CodeAtlas::MainUI::buildConnections()
{
	connect(m_ui.actionUpdate, SIGNAL(triggered()),this, SLOT(resetDB()));
	connect(m_ui.actionActivate, SIGNAL(triggered(bool)),this, SLOT(onActivate(bool)));
}

void CodeAtlas::MainUI::resetDB()
{
#ifdef USING_QT_4
	CppTools::Internal::CppModelManager* mgr = CppTools::Internal::CppModelManager::instance();
#elif defined(USING_QT_5)
	CppTools::CppModelManagerInterface *mgr = CppTools::CppModelManagerInterface::instance();
	if (!mgr)
		return;
#endif
	CPlusPlus::Snapshot snapshot = mgr->snapshot();
}

void CodeAtlas::MainUI::onActivate( bool isActivate )
{
	DBManager::instance()->setEnable(isActivate);
}

void CodeAtlas::MainUI::unlockUI()
{
	CodeView* view = (CodeView*)m_ui.centralwidget;
	view->unlockView();
}

void CodeAtlas::MainUI::lockUI()
{
	CodeView* view = (CodeView*)m_ui.centralwidget;
	view->lockView();
}
