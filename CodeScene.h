#pragma once
namespace CodeAtlas
{

// class SceneProjData
// {
// public:
// 	SceneProjData():m_timeStamp(0), m_projItem(NULL){}
// 	~SceneProjData();
// 
// 	void                 addClassUIItem(ClassUIOldItem* item);
// 	ProjectUIOldItem*       getOrAddProjectUIItem();
// 	ProjectUIOldItem*       getProjectUIItem()const{return m_projItem;}
// 	void                 clearItems();
// 	const QList<ClassUIOldItem*>& getClassUIItemList()const{return m_symbolItemList;}
// 
// 	int                  nSymbolLevels()const{return m_nSymbolLevel;}
// 	void                 setSymbolLevels(int nSymLevel){m_nSymbolLevel = nSymLevel;}
// 
// 	int                  projLevel()const{return m_projLevel;}
// 	void                 setProjLevel(int projLevel){m_projLevel = projLevel;}
// 
// 	unsigned             timeStamp(){return m_timeStamp;}
// 	void                 setTimeStamp(unsigned timeStamp){m_timeStamp = timeStamp;}
// 
// 	void                 updateLocalGeometry();
// 	QRectF               getProjectRange()const;
// 
// private:
// 	unsigned             m_timeStamp;
// 	int                  m_projLevel;           // level of project	
// 	int                  m_nSymbolLevel;		// highest level of symbols
// 
// 	QRectF               m_localRect;			// local coordinate range of symbols in this project
// 
// 	QList<ClassUIOldItem*>  m_symbolItemList;
// 	ProjectUIOldItem*       m_projItem;	
// };

class CodeScene: public QGraphicsScene
{
public:
	CodeScene(void);
	~CodeScene(void);

	void init();
	void clear();
	//void updateSceneData(const QHash<SymbolPath, Locator::ProjectData>& projData);

	// ignore message when the scene is locked
	void lockScene();
	void unlockScene();
	virtual void	drawBackground ( QPainter * painter, const QRectF & rect );

	// cursor node
	SymbolNode::Ptr getCurCursorNode()const{return m_curCursorNode;}
	void			setCurCursorNode(const SymbolNode::Ptr& node);
protected:
	bool event(QEvent *event);
private:
	// something about exponent kernel, which is used to compute influence range
	void                initKernelMap();
	inline float        fastKernel(float dist)
	{
		if (dist >= 3.f)
			return 0.f;
		return m_expMap[int(dist * 100)];
	}
	bool				m_receiveMsg;
	QMutex              m_mutex;

	SymbolNode::Ptr		m_curCursorNode;

	static float        m_itemAffectRadius;
	// threshold value
	static float        m_projLOD;

	static float        m_expMap[300];			// look-up table of e^(-x^2)
};

}