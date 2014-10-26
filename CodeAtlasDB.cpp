#include "StdAfx.h"
#include "CodeAtlasDB.h"
using namespace CodeAtlas;


CodeAtlasDB::CodeAtlasDB(void):m_timer(this)
{
	m_timeStamp = 0;
	m_timer.setSingleShot(true);
	connect(&m_timer, SIGNAL(timeout()), SLOT(updateDB()), Qt::QueuedConnection);

	initialize();
}

CodeAtlasDB::~CodeAtlasDB(void)
{
}



void CodeAtlas::CodeAtlasDB::initialize()
{
	m_symCollector.addModifier(SymbolModifier::Ptr(new WordCollector));
	m_symCollector.addModifier(SymbolModifier::Ptr(new FuzzyDependBuilder));
	m_symCollector.addModifier(SymbolModifier::Ptr(new UIElementLocator));
}



void CodeAtlas::CodeAtlasDB::removeFiles( const QStringList& fileList )
{
	m_symCollector.removeFiles(fileList);
}


void CodeAtlas::CodeAtlasDB::clearCacheAll()
{
	updateTimeStamp();
	m_symCollector.clearCacheAll();
}

void CodeAtlas::CodeAtlasDB::clearCache()
{
	updateTimeStamp();
	m_symCollector.clearCache();
}

void CodeAtlas::CodeAtlasDB::updateDB()
{
	qDebug() << "update SymbolTreeBuilder";
	m_timer.stop();
	updateTimeStamp();
	m_symCollector.update();

	updateCursor();
	emit dbUpdated();
}

void CodeAtlas::CodeAtlasDB::setFileList( const QStringList &fileList )
{
	updateTimeStamp();
	m_symCollector.setFileList(fileList);
}

void CodeAtlas::CodeAtlasDB::resetData( const CPlusPlus::Snapshot &snapshot )
{
	updateTimeStamp();
	m_symCollector.resetData(snapshot);
//	m_wordCollector.clear();

	emit dbUpdated();
}

void CodeAtlas::CodeAtlasDB::resetDataToCurrentState()
{
	updateTimeStamp();
	m_symCollector.resetDataToCurrentState();

//	m_wordCollector.clear();
//	m_wordCollector.compute(m_symCollector);

	emit dbUpdated();
}

void CodeAtlas::CodeAtlasDB::updateDocumentCache( const CPlusPlus::Document::Ptr &doc )
{
	updateTimeStamp();
	m_symCollector.updateDocumentCache(doc);

// 	CallGraphAnalyzer cgAnalyzer(doc, m_symCollector.getSymbolTree());
// 	cgAnalyzer.extractCallList();
// 	CallGraphData::CallItem callList = cgAnalyzer.getCallList();
// 
// 	m_callGraph.printCallGraph();
// 	m_callGraph.updateDocCallGraph(doc, callList);
// 	m_callGraph.printCallGraph();

	qDebug() << "request update";
	if (!m_timer.isActive())
		m_timer.start(1000);
}

void CodeAtlas::CodeAtlasDB::moveToNewThread(QThread& newThread)
{
	this->moveToThread(&newThread);
	m_symCollector.moveToThread(&newThread);
}

void CodeAtlas::CodeAtlasDB::updateTimeStamp()
{
	m_timeStamp++;
	m_symCollector.setCurrentTimeStamp(m_timeStamp);
}

void CodeAtlas::CodeAtlasDB::updateCursor()
{
	CPlusPlus::Symbol* symbol = NULL;
	CPlusPlus::Document::Ptr doc;
	SymbolUtility::findSymbolUnderCursor(symbol, doc);

	SymbolNode::Ptr node;
	if (symbol)
		node = m_symCollector.findNodeInDoc(doc->fileName(), symbol);
	if (!node && doc)
	{
		QList<SymbolNode::Ptr> nodeList = m_symCollector.findProjNodeInDoc(doc->fileName());
		if (nodeList.size() >0)
			node = nodeList[0];
	}
	UIManager::instance()->getScene().setCurCursorNode(node);
}

