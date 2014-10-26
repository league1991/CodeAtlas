#include "stdafx.h"
#include "UIItemModel.h"

using namespace CodeAtlas;

CodeAtlas::CircleItemModel::CircleItemModel( ):UIItemModel()
{
}


QPainterPath CodeAtlas::CircleItemModel::shape( const NodeUIItem& item )
{
	QPainterPath s;
	UIInteractMode mode = item.curInteractMode();
	if (mode == UI_INVISIBLE || mode == UI_FROZEN)
		return s;
	QRectF rect = computeRect(item.entityRect(), item.getCurViewRadius());
	s.addEllipse(rect);
	return s;
}

void CodeAtlas::CircleItemModel::paint( const NodeUIItem& item,QPainter *painter, const QStyleOptionGraphicsItem *option/* =0 */ )
{
	UIInteractMode mode = item.curInteractMode();
	if (mode == UI_INVISIBLE)
		return;
// 	if (mode < 0 || mode > 4)
// 		mode = UI_NORMAL;
	QRectF rect = computeRect(item.entityRect(), item.getCurViewRadius());
	painter->setPen(m_pen[mode]);
	painter->setBrush(m_brush[mode]);
	painter->drawEllipse(rect);
}


QRectF CodeAtlas::UIItemModel::centerScaleRect( const QRectF& rect, float wFactor, float hFactor )
{
	float w = rect.width();
	float h = rect.height();
	float dw = (w * wFactor - w) * .5f;
	float dh = (h * hFactor - h) * .5f;
	return rect.adjusted(-dw, -dh, dw, dh);
}







QPainterPath CodeAtlas::TextItemModel::shape( const NodeUIItem& item )
{
	QPainterPath path;
	UIInteractMode mode = item.curInteractMode();
	if (mode == UI_INVISIBLE || mode == UI_FROZEN)
		return path;
	if (m_hasShape)
	{
		QRectF rect = computeRect(item.entityRect(), item.getCurViewRadius());
		path.addRect(rect);
	}
	return path;
}

void CodeAtlas::TextItemModel::paint( const NodeUIItem& item,QPainter *painter, const QStyleOptionGraphicsItem *option )
{
	UIInteractMode mode = item.curInteractMode();
	if (mode == UI_INVISIBLE)
		return;

	float		   lod  = QStyleOptionGraphicsItem::levelOfDetailFromTransform(painter->worldTransform());
	QRectF		   rect = computeRect(item.entityRect(), item.getCurViewRadius());
	QString		   name = item.name();

	QFont      paintFont= m_font[mode];
	float     pointSize = paintFont.pointSizeF();
	float		viewSize= item.getCurViewRadius();
	if (mode == UI_NORMAL && (viewSize < m_minShowSize || viewSize > m_maxShowSize))
		return;
	float	   fontSize = m_font[mode].pointSizeF()* rect.height() * 0.08f;
	paintFont.setPointSizeF(fontSize);

	painter->setPen(m_pen[mode]);
	painter->setFont(paintFont);
	painter->drawText(rect, m_txtAlignMode | Qt::TextDontClip, name);
}

CodeAtlas::TextItemModel::TextItemModel():
UIItemModel(Qt::AlignCenter, 3.f, 0.1f),
m_txtAlignMode(Qt::AlignCenter), 
m_hasShape(false),
m_minShowSize(0.f),
m_maxShowSize(200.f)
{

}



QRectF CodeAtlas::UIItemModel::computeRect( const QRectF& rect , float viewSize)
{
	float lodScale = 1.f;
	if (viewSize < m_minViewSize)
		lodScale = m_minViewSize / viewSize;
	else if (viewSize > m_maxViewSize)
		lodScale = m_maxViewSize / viewSize;
	QRectF res = centerScaleRect(rect, m_wFactor * lodScale, m_hFactor * lodScale);
	float tx = 0.f, ty = 0.f;
	if (m_rectAlignMode & Qt::AlignLeft)
		tx = (rect.width()*m_entityWFactor*lodScale + res.width()) * -0.5f;
	else if (m_rectAlignMode & Qt::AlignRight)
		tx = (rect.width()*m_entityWFactor*lodScale + res.width()) * +0.5f;
	if (m_rectAlignMode & Qt::AlignTop)  
		ty = (rect.height()*m_entityHFactor*lodScale + res.height()) * -0.5f;
	else if (m_rectAlignMode & Qt::AlignBottom)
		ty = (rect.height()*m_entityHFactor*lodScale + res.height()) * 0.5f;
	res.translate(tx, ty);
	return res;
}

CodeAtlas::UIItemModel::UIItemModel() :
m_rectAlignMode(Qt::AlignCenter),
m_wFactor(1.f), m_hFactor(1.f),
m_entityWFactor(1.f), m_entityHFactor(1.f),
m_minViewSize(0.f), m_maxViewSize(1e6f)
{
	QFont font("Arial");
	setDefaultScaleBehavior(false);
	setDefaultPen(QPen(QColor(128,128,128,255)));
	setDefaultBrush(QBrush(QColor(198,183,201)));
	setDefaultFont(font);
}

CodeAtlas::UIItemModel::UIItemModel( unsigned rectAlignMode, float wFactor, float hFactor ) : 
m_rectAlignMode(rectAlignMode),
m_wFactor(wFactor), m_hFactor(hFactor),
m_entityWFactor(1.f), m_entityHFactor(1.f),
m_minViewSize(0.f), m_maxViewSize(1e6f)
{
	QFont font("Arial");
	setDefaultScaleBehavior(false);
	setDefaultPen(QPen(QColor(128,128,128,255)));
	setDefaultBrush(QBrush(QColor(198,183,201)));
	setDefaultFont(font);
}

void CodeAtlas::UIItemModel::setDefaultPen( const QPen& pen )
{
	for (int i = 0; i < TOTAL_MODES; ++i)
		m_pen[i] = pen;
}

void CodeAtlas::UIItemModel::setDefaultBrush( const QBrush& brush )
{
	for (int i = 0; i < TOTAL_MODES; ++i)
		m_brush[i] = brush;
}

void CodeAtlas::UIItemModel::setDefaultFont( const QFont& font )
{
	for (int i = 0; i < TOTAL_MODES; ++i)
	{
		m_font[i] = font;
	}
}

void CodeAtlas::UIItemModel::setDefaultScaleBehavior( bool isScaleInvariant )
{
	for (int i = 0; i < TOTAL_MODES; ++i)
		m_isScaleInVariant[i] = isScaleInvariant;
}


QPainterPath CodeAtlas::VariableItemModel::shape( const NodeUIItem& item )
{
	QPainterPath path;
	path.addRect(item.entityRect());
	return path;
}

void CodeAtlas::VariableItemModel::paint( const NodeUIItem& item, QPainter *painter, const QStyleOptionGraphicsItem *option )
{
	const QRectF& entityRect = item.entityRect();

	painter->setBrush(UISetting::s_varBackgroundClr);

	SymbolInfo nodeInfo = getNodeSymInfo(item);
	QPen bgPen;
	if (nodeInfo.elementType() & SymbolInfo::PublicMember)
		bgPen = UISetting::s_publicMemPen;
	else if (nodeInfo.elementType() & SymbolInfo::ProtectedMember)
		bgPen = UISetting::s_protectedMemPen;
	else if (nodeInfo.elementType() & SymbolInfo::PrivateMember)
		bgPen = UISetting::s_privateMemPen;
	bgPen.setColor(UISetting::s_varCountourClr);
	painter->setPen(bgPen);
	QRectF treeRect = entityRect;
	float offset = treeRect.width() * 0.1;
	treeRect.adjust(offset,0,-offset,-offset*2.f);
	painter->drawEllipse(treeRect);
	//painter->drawRoundedRect(entityRect, 25, 25, Qt::RelativeSize);

	QRectF trunkRect = entityRect;
	float trunkWidth = entityRect.width() * 0.2;
	float trunkHeight= entityRect.height() * 0.5;
	trunkRect.setWidth(trunkWidth);
	trunkRect.setHeight(trunkHeight);
	trunkRect.translate(entityRect.width()*0.5 - trunkWidth*0.5, trunkHeight);
	painter->setPen(Qt::NoPen);
	painter->setBrush(QBrush(QColor(104,82,62)));
	painter->drawRect(trunkRect);

	// draw title
	if (item.getCurViewRadius() > 6 || item.curInteractMode() == UI_HOVERING)
	{
		QString name = nodeInfo.name();
		QPointF bottomHCenter(entityRect.left()+entityRect.width()*0.5, entityRect.bottom());
		QFont font("Arial", 28);
		painter->setPen(QPen(QColor(0,0,0,180)));
		painter->setFont(font);
		unsigned hAlignFlag = Qt::AlignTop;
		unsigned vAlignFlag = Qt::AlignHCenter; 
		//drawText(painter, name, font, bottomHCenter, hAlignFlag|vAlignFlag);
	}
}

void CodeAtlas::VariableItemModel::drawTreeShape( QPainter* painter, const QRectF& entityRect )
{

}

CodeAtlas::SymbolInfo CodeAtlas::UIItemModel::getNodeSymInfo( const NodeUIItem& item )
{
	SymbolNode::Ptr node = item.getNode().toStrongRef();
	if (node)
		return node->getSymInfo();
	return SymbolInfo();
}

void CodeAtlas::UIItemModel::drawText( QPainter* painter, const QString& txt, const QFont& font, const QPointF& basePoint, unsigned alignFlag /*= Qt::AlignHCenter | Qt::AlignTop*/ )
{
	QFontMetricsF metrics(font);
	QRectF rect = metrics.boundingRect(txt);
	rect.moveTo(basePoint);
	float w = rect.width();
	float h = rect.height();
	float tx = 0.f, ty = 0.f;
	if (alignFlag & Qt::AlignRight)
		tx = -w;
	else if (alignFlag & Qt::AlignHCenter)
		tx = -w*0.5;
	if (alignFlag & Qt::AlignBottom)
		ty = -h;
	else if (alignFlag & Qt::AlignVCenter)
		ty = -h*0.5;
	rect.translate(tx, ty);

	painter->drawText(rect, Qt::AlignCenter, txt);
}

QPainterPath CodeAtlas::FunctionItemModel::shape( const NodeUIItem& item )
{
	QPainterPath path;
	path.addRect(item.entityRect());
	return path;
}

void CodeAtlas::FunctionItemModel::paint( const NodeUIItem& item, QPainter *painter, const QStyleOptionGraphicsItem *option )
{
	const QRectF& entityRect = item.entityRect();
	painter->setBrush(QColor(200,200,200,255));

	SymbolInfo nodeInfo = getNodeSymInfo(item);
	QPen bgPen;
	if (nodeInfo.elementType() & SymbolInfo::PublicMember)
	{
		bgPen = UISetting::s_publicMemPen;
		bgPen.setWidthF(10);
	}
	else if (nodeInfo.elementType() & SymbolInfo::ProtectedMember)
		bgPen = UISetting::s_protectedMemPen;
	else if (nodeInfo.elementType() & SymbolInfo::PrivateMember)
		bgPen = UISetting::s_privateMemPen;
	bgPen.setWidthF(bgPen.widthF() * 2);
	bgPen.setColor(QColor(150,150,150,255));
	painter->setPen(bgPen);
	if (nodeInfo.elementType() & SymbolInfo::PrivateMember)
		painter->setPen(Qt::NoPen);

	painter->drawRoundedRect(entityRect, 50, 50, Qt::RelativeSize);

// 	QPen roofPen(QColor(249,116,45));
// 	painter->setPen(roofPen);
	// draw title
	if (item.getCurViewRadius() > 6 || item.curInteractMode() == UI_HOVERING)
	{
		QString name = nodeInfo.name();
		QPointF bottomHCenter(entityRect.left()+entityRect.width()*0.5, entityRect.center().y());
		QFont font("Arial", 28);
		painter->setPen(QPen(QColor(0,0,0,180)));
		painter->setFont(font);
		unsigned hAlignFlag = Qt::AlignVCenter;
		unsigned vAlignFlag = Qt::AlignHCenter; 
		drawText(painter, "+", font, bottomHCenter, hAlignFlag|vAlignFlag);
	}
}
