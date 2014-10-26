#pragma once

namespace CodeAtlas
{ 


class CodeAtlasDB:public QObject
{
	Q_OBJECT
public:
	CodeAtlasDB(void);
	~CodeAtlasDB(void);

	void reset(const CPlusPlus::Snapshot &snapshot);
	void moveToNewThread(QThread& newThread);

	//void clearCallGraph(){m_callGraph.clear();}

	//Locator& getVocabularyCollector(){return m_wordCollector;}
	SymbolTreeBuilder&getSymbolCollector(){return m_symCollector;}
signals:
	void dbUpdated();
public slots:
	void clearCacheAll();
	void clearCache();

	void updateDB();
	void updateCursor();

	void setFileList(const QStringList &fileList);
	void removeFiles(const QStringList &fileList);

	void resetData(const CPlusPlus::Snapshot &snapshot);
	void resetDataToCurrentState();

	void updateDocumentCache(const CPlusPlus::Document::Ptr &doc);
private:
	void initialize();
	void updateTimeStamp();

	QTimer             m_timer;
	SymbolTreeBuilder     m_symCollector;
	//CallGraphData      m_callGraph;
	//Locator m_wordCollector;
	unsigned           m_timeStamp;
};

}