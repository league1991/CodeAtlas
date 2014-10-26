#include "StdAfx.h"
#include "CodeScene.h"

using namespace CodeAtlas;

float CodeAtlas::CodeScene::m_expMap[300];
 


CodeScene::CodeScene(void):m_receiveMsg(true), m_mutex(QMutex::Recursive)
{
	initKernelMap();
	setItemIndexMethod(QGraphicsScene::NoIndex);
}

CodeScene::~CodeScene(void)
{
	removeItem(RootItem::instance());
	clear();
}


void CodeScene::clear()
{
}

void CodeScene::initKernelMap()
{
	for (int i = 0; i < sizeof(m_expMap) / sizeof(float); ++i)
	{
		float v = float(i)/100.f;
		m_expMap[i] = qExp(-v*v);
	}
}

void CodeAtlas::CodeScene::drawBackground( QPainter * painter, const QRectF & rect )
{
	//m_terrainMaker.drawBackground(painter, rect, m_projData);
}

bool CodeAtlas::CodeScene::event( QEvent *event )
{
	bool res = true;
	m_mutex.lock(); 
	if (m_receiveMsg)
	{
		//qDebug("scene event");
		res = QGraphicsScene::event(event);
	}
	m_mutex.unlock();
	return res;
}

void CodeAtlas::CodeScene::lockScene()
{ 
	m_mutex.lock();
	//m_receiveMsg = false;
	removeItem(RootItem::instance());
	m_mutex.unlock();
}

void CodeAtlas::CodeScene::unlockScene()
{
	m_mutex.lock();
	//m_receiveMsg = true;
	addItem(RootItem::instance());
	m_mutex.unlock();
}

void CodeAtlas::CodeScene::init()
{
	addItem(RootItem::instance());
}

void CodeAtlas::CodeScene::setCurCursorNode( const SymbolNode::Ptr& node )
{
	m_curCursorNode = node;
	foreach (QGraphicsView*view, this->views())
	{
		((CodeView*)view)->centerViewWhenNecessary();
	}
	update();
}
