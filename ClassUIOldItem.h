#pragma  once
namespace CodeAtlas
{

	class ClassUIOldItem :	public QGraphicsItem
	{
	public:
		ClassUIOldItem(void);
		ClassUIOldItem(const Locator::SymbolPosition& pos);
		ClassUIOldItem(const QVector2D& pos, const SymbolPath& path, float radius, int level = 0);
		~ClassUIOldItem(void);

		QRectF boundingRect() const;
		void   paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0, QWidget *widget = 0);

		const SymbolInfo*   getProjectInfo()const;
		const SymbolInfo*   getItemInfo()const;

		const QColor&              getTerritoryColor()const;
		const QColor&              getProjTerritoryColor()const;

		float                      getDisplayRadius(){return m_radius;}
		SymbolPath&                getSymbolPath(){return m_path;}

		// return affect range of this item
		// the appearance of scene will be affected within the affect range
		QRectF                     getAffectRange()const;
		float                      getAffectRangeRadius()const{return m_radius*m_maxRangeFactor;}
		int                        getLevel()const{return m_level;}
	private:
		void                       initItem();
		QString m_name;
		QString m_type;
		SymbolPath m_path;

		float   m_nFunctions;
		float   m_nVariables;

		int     m_level;
		float   m_radius;
		QRectF  m_rect;

		// level of detail feature
		// 1.null stage: nothing to display
		// 2.point stage: only a point is displayed
		// 3.icon stage: icon and text is displayed
		// 4.full stage: content of class is displayed
		static float m_stageLOD[3];				// threshold of four stages, 0-3 -> null stage - full stage
		static float m_pointSize;				// radius of point in point stage
		static float m_iconSize;				// size of icon in icon stage

		static float m_maxRangeFactor;			// = actual affect radius / m_radius
	};
}