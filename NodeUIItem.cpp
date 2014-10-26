#include "StdAfx.h"
#include "NodeUIItem.h"
using namespace CodeAtlas;


RootItem* RootItem::s_instance = new RootItem;

QImage NodeUIItem::m_icon;

float NodeUIItem::s_affectRatio = 1.f / 2.f;
float NodeUIItem::s_minTerrainRadius = 300;
float NodeUIItem::s_minShowSize = 2;
float NodeUIItem::s_maxShowSize = 100;
float NodeUIItem::s_nodeBaseZValue = 30;

QList<UIItemModel::Ptr> NodeUIItem::m_uiItemModelList;

QList<UIItemModel::Ptr> ProjectUIItem::m_projModelList;

QList<UIItemModel::Ptr> ClassUIItem::m_classModelList;

QList<UIItemModel::Ptr> VariableUIItem::m_variableModelList;

QList<UIItemModel::Ptr> FunctionUIItem::m_funcModelList;

RootItem::RootItem() :QGraphicsItem(0)
{
	setFlags(	QGraphicsItem::ItemIsSelectable | 
			 QGraphicsItem::ItemIsFocusable);
}

QRectF RootItem::boundingRect() const
{
	return QRectF(0,0,1000,1000);
}



UIItem::UIItem( UIItem* parent ) :QGraphicsItem(0), m_lodStatus(LOD_EXPANDED)
{
	if (parent == 0)
		setParentItem(RootItem::instance());
	else
		setParentItem(parent);
}

UIItem::~UIItem()
{
	setParentItem(0);
	QList<QGraphicsItem*> childItemList = childItems();
	for (int ithChild = 0; ithChild < childItemList.size(); ++ithChild)
	{
		childItemList[ithChild]->setParentItem(0);
	}
}

void NodeUIItem::buildUI()
{
	if (m_node.isNull())
		return;

	SymbolNode::Ptr node = m_node.toStrongRef();
	if (node)
	{
		SymbolInfo info = node->getSymInfo();
		m_name = info.name();
		m_type = info.type();
		SymbolPath path = node->getSymPath();
		setToolTip(path.toString(false, false));
		setZValue(s_nodeBaseZValue);
	}
}

void NodeUIItem::paint( QPainter *painter, const QStyleOptionGraphicsItem *option/*=0*/, QWidget *widget /*= 0*/ )
{
	updateMode(painter->worldTransform());

	if (m_lodStatus & (LOD_INVALID|LOD_INVISIBLE))
		return;

	QList<UIItemModel::Ptr>& modelList = getUIItemModelList();

	//qDebug() << m_name << m_interactMode;
	for (int i = 0; i < modelList.size(); ++i)
	{
		UIItemModel::Ptr& pModel = modelList[i];
		pModel->paint(*this, painter, option);
	}

}

NodeUIItem::Ptr NodeUIItem::creator( const SymbolNode::Ptr& node, NodeUIItem* parent /*= 0*/ )
{
	SymbolInfo info = node->getSymInfo();
	NodeUIItem::Ptr ui;
	SymbolInfo::ElementType type = info.elementType();
	if (type == SymbolInfo::Project)
		ui = NodeUIItem::Ptr(new ProjectUIItem(node.toWeakRef(), parent));
	else if (type == SymbolInfo::Class)
		ui = NodeUIItem::Ptr(new ClassUIItem(node.toWeakRef(), parent));
	else if (type & SymbolInfo::Variable)
		ui = NodeUIItem::Ptr(new VariableUIItem(node.toWeakRef(), parent));
	else if (type & SymbolInfo::FunctionSignalSlot)
		ui = NodeUIItem::Ptr(new FunctionUIItem(node.toWeakRef(), parent));
	else
		ui = NodeUIItem::Ptr(new NodeUIItem(node.toWeakRef(), parent));
	return ui;
}

void NodeUIItem::setEntityRadius( float r0 )
{
	prepareGeometryChange();
	m_radius = r0;
	const float r = m_radius;
	const float f = qMax(m_radius / s_affectRatio, s_minTerrainRadius);
	m_terrainAffectRect = QRectF(-f, -f, f*2, f*2);
	m_entityRect = QRectF(-r, -r, r*2, r*2);
}





NodeUIItem::NodeUIItem(const SymbolNode::WeakPtr& node, 
									  UIItem* parent /*= 0*/ ) :
UIItem(parent), 
m_node(node), 
m_radius(1.f), 
m_level(1.f),
m_interactMode(UI_NORMAL),
m_isMouseHover(false),
m_uiAttr(NULL),
m_interactionFlag(0)
{
	setFlags(	QGraphicsItem::ItemIsSelectable | 
				QGraphicsItem::ItemIsFocusable);

	setAcceptHoverEvents(true);
	setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton | Qt::RightButton);
	setAcceptDrops(true);

	if (m_node)
	{
		m_uiAttr = m_node.toStrongRef()->getAttr<UIElementAttr>().data();
	}
	if (m_uiItemModelList.size() == 0)
		initUIItemModel();
}

void NodeUIItem::hoverEnterEvent( QGraphicsSceneHoverEvent * event )
{
	qDebug()<<"hover enter " << m_name << m_interactMode; 
	m_isMouseHover = true;
	m_interactionFlag |= IS_MOUSE_HOVER;
	QGraphicsItem::hoverEnterEvent(event);
	initUIItemModel();
	//update();
}

void NodeUIItem::hoverLeaveEvent( QGraphicsSceneHoverEvent *event )
{
	qDebug()<<"hover leave " << m_name << m_interactMode; 
	m_isMouseHover = false;
	m_interactionFlag &= ~IS_MOUSE_HOVER;
	QGraphicsItem::hoverLeaveEvent(event);
	//update(m_terrainAffectRect);
}

void NodeUIItem::updateMode( const QTransform& worldTransform )
{
	m_viewLod = getViewLOD(worldTransform);
	float r = m_viewLod * m_radius;
	float minInteract, maxInteract, minFrozen, maxFrozen;
	getModeThreshold(minInteract, maxInteract, minFrozen, maxFrozen);
	switch(m_interactMode)
	{
	case UI_INVISIBLE:
		if (r >= minInteract && r < maxInteract)
			m_interactMode = UI_NORMAL;
		else if(r >= minFrozen && r < maxFrozen)
			m_interactMode = UI_FROZEN;
		break;
	case UI_FROZEN:
		if (!(r >= minInteract && r < maxInteract))
			m_interactMode = UI_INVISIBLE;
		else if (r >= minInteract && r < maxInteract)
			m_interactMode = UI_NORMAL;
	case UI_NORMAL:
	case UI_HOVERING:
	case UI_SELECTED:
		if (isSelected())
			m_interactMode = UI_SELECTED;		
		else if(r >= minFrozen && r < maxFrozen)
			m_interactMode = UI_FROZEN;
		else if (!(r >= minInteract && r < maxInteract))
			m_interactMode = UI_INVISIBLE;
		else if (m_isMouseHover)
			m_interactMode = UI_HOVERING;
		else
			m_interactMode = UI_NORMAL;
		break;
	}
	switch(m_interactMode)
	{
	case UI_INVISIBLE:
		//setAcceptHoverEvents(false);
		setAcceptedMouseButtons(Qt::NoButton);
		setFlags(0);
		m_isMouseHover = false;
		break;
	case UI_FROZEN:
		//setAcceptHoverEvents(false);
		setAcceptedMouseButtons(Qt::NoButton);
		setFlags(0);
		m_isMouseHover = false;
		break;
	case UI_NORMAL:
	case UI_HOVERING:
	case UI_SELECTED:
		//setAcceptHoverEvents(true);
		setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton | Qt::RightButton);
		setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
	}
}



QPainterPath NodeUIItem::shape() const
{
	QPainterPath path;
	QList<UIItemModel::Ptr>& modelList = getUIItemModelList();
	for (int i = 0; i < modelList.size(); ++i)
	{
		UIItemModel::Ptr& pItem = modelList[i];
		path |= pItem->shape(*this);
	}
	return path;
}

void NodeUIItem::initUIItemModel()
{
	m_uiItemModelList.clear();

	CircleItemModel::Ptr pCircleModel = CircleItemModel::Ptr(new CircleItemModel);	
	pCircleModel->setPen(QPen(QColor(0,0,0,255)),       UI_NORMAL);
	pCircleModel->setPen(QPen(QColor(128,128,128,255)), UI_HOVERING);
	pCircleModel->setPen(QPen(QColor(255,0,0,255)),     UI_SELECTED);
	pCircleModel->setBrush(QBrush(QColor(183,198,201)), UI_NORMAL);
	pCircleModel->setBrush(QBrush(QColor(193,208,211)), UI_HOVERING);
	pCircleModel->setBrush(QBrush(QColor(153,198,201)), UI_SELECTED);

	TextItemModel::Ptr   pTextModel = TextItemModel::Ptr(new TextItemModel);
	QFont font("Arial");
	pTextModel->setPen(QPen(QColor(0,0,0,128)), UI_NORMAL);
	pTextModel->setPen(QPen(QColor(0,0,0,255)), UI_HOVERING);
	pTextModel->setPen(QPen(QColor(255,0,0,255)), UI_SELECTED);
	pTextModel->setFont(font, UI_NORMAL);
	pTextModel->setFont(font, UI_HOVERING);
	pTextModel->setFont(font, UI_SELECTED);

	m_uiItemModelList.append(pCircleModel);
	m_uiItemModelList.append(pTextModel);
}

void NodeUIItem::getModeThreshold( float& minNormal, float& maxNormal, float& minFrozen, float& maxFrozen )
{
	minNormal = 2.f;
	maxNormal = 300.f;
	minFrozen = 300.f;
	maxFrozen = 400.f;
}





void CodeAtlas::NodeUIItem::focusInEvent( QFocusEvent *event )
{
	qDebug("focusIn");
	m_interactionFlag |= IS_FOCUSED;
}

void CodeAtlas::NodeUIItem::focusOutEvent( QFocusEvent *event )
{
	qDebug("focusOut");
	m_interactionFlag &= ~IS_FOCUSED;
}

void CodeAtlas::NodeUIItem::mouseDoubleClickEvent( QGraphicsSceneMouseEvent *event )
{
	SymbolNode::Ptr node = m_node.toStrongRef();
	if (!node)
		return;

	LocationAttr::Ptr locAttr = node->getAttr<LocationAttr>();
	if (!locAttr)
		return;

	const QSet<SymbolLocation>& locs = locAttr->getSymbolLocations();
	DBManager::instance()->gotoLocations(locs);
}

void CodeAtlas::RootItem::paint( QPainter *painter, const QStyleOptionGraphicsItem *option/*=0*/, QWidget *widget /*= 0*/ )
{
// 	if (QGraphicsItem::isSelected())
// 	{
// 		painter->setBrush(QBrush(Qt::cyan));
// 	}
// 	else
// 		painter->setBrush(QBrush(Qt::white));
// 	painter->drawEllipse(QPointF(), 1000,1000);
}
