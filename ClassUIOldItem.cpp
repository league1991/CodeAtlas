#include "stdafx.h"
#include "ClassUIOldItem.h"

using namespace CodeAtlas;

float CodeAtlas::ClassUIOldItem::m_stageLOD[3] = {1, 4, 35};
float CodeAtlas::ClassUIOldItem::m_pointSize   = 1;
float CodeAtlas::ClassUIOldItem::m_iconSize    = 1;
float CodeAtlas::ClassUIOldItem::m_maxRangeFactor = 10.f;

const QColor& CodeAtlas::ClassUIOldItem::getTerritoryColor() const
{
	return GlobalSetting::getTerritoryColor(m_path.hash());
}

const QColor& CodeAtlas::ClassUIOldItem::getProjTerritoryColor() const
{
	const SymbolInfo* info = getProjectInfo();
	unsigned hash = 0;
	if (info)
		hash = info->hash();
	return GlobalSetting::getTerritoryColor(hash);
}

QRectF CodeAtlas::ClassUIOldItem::getAffectRange()const
{
	float rangeRadius = m_radius * m_maxRangeFactor;
	return QRectF(	pos().x() - rangeRadius,
		pos().y() - rangeRadius,
		rangeRadius * 2.f,
		rangeRadius * 2.f);
}

// void CodeAtlas::ClassUIItem::paint( QPainter *painter, const QStyleOptionGraphicsItem *option/*=0*/, QWidget *widget /*= 0*/ )
// {
// 	UIRenderer::EffectType effect = currentEffect();
// 	m_iconRenderer.paint(painter, m_icon, m_entityRect, effect);
// 	m_textRenderer.paintBelow(painter, m_name, m_entityRect, effect);
// }


void ClassUIOldItem::initItem()
{
	QString toolTip = QString(m_path.toString() + "\nlevel: %1").arg(m_level);
	setToolTip(toolTip);
	const SymbolInfo* info = m_path.getLastSymbol();
	if (info)
	{
		m_name = info->name();
		m_type = info->type();
	}
	m_rect = QRectF(-1,-1,2,2);

	SymbolTree tree = DBManager::instance()->getDB().getSymbolCollector().getSymbolTree();
	SymbolNode::ConstPtr pNode = tree.findItem(m_path);
	if (pNode)
	{
		if (ChildInfoAttr::Ptr pChildInfo = pNode->getAttr<ChildInfoAttr>())
		{
			m_nFunctions = pChildInfo->childFunctions();
			m_nVariables = pChildInfo->childVariables();
		}
	}

	setFlags(ItemIsSelectable|ItemIsMovable);
	setAcceptsHoverEvents(true);
}

const SymbolInfo* ClassUIOldItem::getProjectInfo() const
{
	for (int ithSym = m_path.getSymbolCount()-1; ithSym >= 0; ithSym--)
	{
		const SymbolInfo* info = m_path.getSymbol(ithSym);
		if (info->isProject())
			return info;
	}
	return NULL;
}

const SymbolInfo* ClassUIOldItem::getItemInfo() const
{
	return m_path.getLastSymbol();
}



ClassUIOldItem::ClassUIOldItem(void)
{
	m_nFunctions = m_nVariables = m_radius = 0;
	m_level = 0;
	initItem();
}

ClassUIOldItem::ClassUIOldItem( const Locator::SymbolPosition& pos )
{
	setPos(pos.m_position.x(), pos.m_position.y());
	m_path = pos.m_path;
	m_radius = pos.m_radius;
	m_level  = pos.m_level;
	initItem();
}
ClassUIOldItem::ClassUIOldItem( const QVector2D& pos, const SymbolPath& path, float radius, int level )
{
	setPos(pos.x(), pos.y());
	m_path = path;
	m_radius = radius;
	m_level  = level;
	initItem();
}
ClassUIOldItem::~ClassUIOldItem(void)
{
}

void ClassUIOldItem::paint( QPainter *painter, const QStyleOptionGraphicsItem *option/*=0*/, QWidget *widget /*= 0*/ )
{
	const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());
	const QColor& color = getTerritoryColor();
	if (lod < m_stageLOD[0])
	{	// null stage
		return;
	}
	else if (lod < m_stageLOD[1])
	{	// point stage
		painter->setPen(Qt::NoPen);
		painter->setBrush(QBrush(color));
		painter->drawEllipse(QPointF(0,0), m_pointSize, m_pointSize); 
	}
	else if (lod < m_stageLOD[2])
	{   // icon stage
		painter->setPen(Qt::NoPen);
		painter->setBrush(QBrush(color));
		painter->drawEllipse(QPointF(0,0), m_iconSize, m_iconSize);
		painter->setFont(QFont("Arial",2));
		painter->scale(0.5,0.5);
		painter->setPen(QPen(QColor(50,50,50,255)));
		painter->drawText(QRect(-20, -2.5, 40, 5),Qt::AlignCenter, m_name);
	}
	else
	{
		painter->setPen(QPen(QColor(0,0,0,50)));
		painter->setFont(QFont("Arial",2));
		painter->scale(0.2,0.2);
		painter->drawText(QRect(-20, -2.5, 40, 5),Qt::AlignCenter, m_name);
	}
}

QRectF ClassUIOldItem::boundingRect() const
{
	return m_rect;
}

