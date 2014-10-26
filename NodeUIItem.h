#pragma once
namespace CodeAtlas{

	// static class 
	// represent root item of the scene
	class RootItem:public QGraphicsItem
	{
	public:
		RootItem();
		QRectF boundingRect() const;
		void   paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0, QWidget *widget = 0);
		static inline RootItem* instance(){return s_instance;}
	private:
		static RootItem* s_instance;
	};

	enum LodStatus;
	enum InteractionStatus
	{
		IS_MOUSE_HOVER = 0x1,
		IS_FOCUSED	   = (0x1)<<1
	};
	// abstract class
	// inherited by NodeUIItem and EdgeUIItem
	class UIItem:public QGraphicsItem
	{
	public:
		typedef QSharedPointer<UIItem> Ptr;
		// constructor, 
		// if parent is null, make root item as parent
		UIItem(UIItem* parent = 0);

		// do not delete any children, children's memory is managed by UIElementAttr
		// so firstly detach all children from the node
		virtual ~UIItem(); 
		
		void		setLodStatus(LodStatus status){m_lodStatus = status;}
		LodStatus	getLodStatus(){return m_lodStatus;}
	protected:
		LodStatus m_lodStatus;
	};

	// The corresponding UIItem of a SymbolNode
	class NodeUIItem: public UIItem
	{
	public:
		enum SymbolUIItemType
		{
			UI_SYMBOL	= 0x1,
			UI_PROJECT	= (0x1<<1),
			UI_CLASS	= (0x1<<2),
			UI_FUNCTION = (0x1<<3),
			UI_VARIABLE = (0x1<<4),
		};
		typedef	QSharedPointer<NodeUIItem> Ptr;
		NodeUIItem(const SymbolNode::WeakPtr& node,  UIItem* parent = 0);

		// custom functions
		virtual void	buildUI();
		void			setNode(const SymbolNode::WeakPtr& node){m_node = node;}
		void			setEntityRadius(float size);
		float			getEntityRadius(){return m_radius;}
		void			setLevel(float l){m_level = l;}
		float			getLevel(){return m_level;}
		const QString&  name()const{return m_name;}

		// virtual function that must be implemented
		QRectF			boundingRect() const{return m_terrainAffectRect;}
		const QRectF&	entityRect()const{return m_entityRect;}
		const QRectF&   terrainAffectRect(){return m_terrainAffectRect;}
		QPainterPath    shape()const;
		void			paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0, QWidget *widget = 0);

		UIInteractMode  curInteractMode()const{return m_interactMode;}


		// create UIItem based on node type
		static NodeUIItem::Ptr creator( const SymbolNode::Ptr& node, NodeUIItem* parent = NULL);

		float			getCurLod()const{return m_viewLod;}
		float			getCurViewRadius()const{return m_viewLod * m_radius;}

		const SymbolNode::WeakPtr& getNode()const{return m_node;}

		inline static float   getNodeBaseZValue(){return s_nodeBaseZValue;}
	protected:
		// item interaction status
		void	        updateMode(const QTransform& worldTransform);
		virtual void	getModeThreshold(float& minNormal, float& maxNormal, float& minFrozen, float& maxFrozen);

		void			hoverEnterEvent( QGraphicsSceneHoverEvent * event );
		void			hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
		void			focusInEvent(QFocusEvent *event);
		void			focusOutEvent(QFocusEvent *event);
		void			mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
		
		inline float	getViewRadius(const QTransform & worldTransform)
		{
			float lod = QStyleOptionGraphicsItem::levelOfDetailFromTransform(worldTransform);
			return lod * m_radius;
		}
		inline float	getViewLOD(const QTransform & worldTransform)
		{
			return QStyleOptionGraphicsItem::levelOfDetailFromTransform(worldTransform);
		}
 
		// ui item model
		virtual void					 initUIItemModel();
		// virtual function, other classes can override it to retrieve different UIItemModels
		virtual QList<UIItemModel::Ptr>& getUIItemModelList()const{return m_uiItemModelList;}
		
		// data
		SymbolNode::WeakPtr				m_node;
		UIElementAttr*					m_uiAttr;
		QString							m_name;
		QString							m_type;
		float							m_radius;
		float							m_level;

		// shape
		QRectF							m_terrainAffectRect;// range this item actually affect, including terrain range, used by qt graphics system to determine whether to draw this item
		QRectF							m_entityRect;		// range of this item's icon, guaranteed not overlapping with other's 

		// appearance
		static float					s_affectRatio;		// = size of affectRect / size of entityRect
		static float					s_minTerrainRadius;
		static QImage					m_icon;

		UIInteractMode					m_interactMode;
		float							m_viewLod;

		unsigned						m_interactionFlag;// ui status
		bool							m_isMouseHover;
	private:

		static QList<UIItemModel::Ptr>  m_uiItemModelList;
		static float					s_minShowSize;
		static float					s_maxShowSize;
		static float					s_nodeBaseZValue;
	};







}