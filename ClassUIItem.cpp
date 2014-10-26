#include "StdAfx.h"
#include "ClassUIItem.h"
using namespace CodeAtlas;

void ClassUIItem::initUIItemModel()
{
	m_classModelList.clear();

	CircleItemModel::Ptr pCircleModel = CircleItemModel::Ptr(new CircleItemModel);
	pCircleModel->setRectStretchFactor(0.2,0.2);
	//pCircleModel->setRectStretchFactor(1,1);
	pCircleModel->setPen(QPen(QColor(0,0,0,255)),       UI_NORMAL);
	pCircleModel->setPen(QPen(QColor(128,128,128,255)), UI_HOVERING);
	pCircleModel->setPen(QPen(QColor(150,20,20,255)),     UI_SELECTED);
	pCircleModel->setBrush(QBrush(QColor(198,183,201,180)), UI_NORMAL);
	pCircleModel->setBrush(QBrush(QColor(208,193,211,255)), UI_HOVERING);
	pCircleModel->setBrush(QBrush(QColor(198,153,201,180)), UI_SELECTED);
	pCircleModel->setBrush(QBrush(QColor(198,183,201, 0)), UI_FROZEN);
	pCircleModel->setPen(QPen(QColor(198,183,201, 0)), UI_FROZEN);
	pCircleModel->setViewSizeRange(20,200);
	//pCircleModel->setViewSizeRange(0,1000);

	TextItemModel::Ptr   pTextModel = TextItemModel::Ptr(new TextItemModel);
	pTextModel->setRectStretchFactor(5,0.2);
	pTextModel->setEntityStretchFactor(0.2,0.2);
	pTextModel->setRectAlignMode(Qt::AlignHCenter|Qt::AlignBottom);
	pTextModel->setTxtAlignMode(Qt::AlignHCenter|Qt::AlignTop);
	pTextModel->setPen(QPen(QColor(0,0,0,200)), UI_NORMAL);
	pTextModel->setPen(QPen(QColor(0,0,0,255)), UI_HOVERING);
	pTextModel->setPen(QPen(QColor(220,200,20,255)), UI_SELECTED);
	pTextModel->setPen(QPen(QColor(0,0,0,30)), UI_FROZEN);
	pTextModel->setFontShowSizeRange(10,200);
	pTextModel->setViewSizeRange(0.2,1000);
	QFont font("Arial"); 
	font.setPointSizeF(12);
	pTextModel->setFont(font, UI_NORMAL);
	font.setPointSizeF(24);
	pTextModel->setFont(font, UI_HOVERING);
	pTextModel->setFont(font, UI_SELECTED);

	m_classModelList.append(pCircleModel);
	m_classModelList.append(pTextModel);
}

ClassUIItem::ClassUIItem( const SymbolNode::WeakPtr& node, UIItem* parent /*= 0*/ ) :NodeUIItem(node, parent)
{
	if (m_classModelList.size() == 0)
		initUIItemModel();
}

void ClassUIItem::getModeThreshold( float& minNormal, float& maxNormal, float& minFrozen, float& maxFrozen )
{
	minNormal = 0.03f;
	maxNormal = 0.2f;
	minFrozen = 0.2f;
	maxFrozen = 200.f;
}

void ClassUIItem::paint( QPainter *painter, const QStyleOptionGraphicsItem *option/*=0*/, QWidget *widget /*= 0*/ )
{
	if (m_lodStatus & (LOD_INVALID|LOD_INVISIBLE))
		return;
	//NodeUIItem::paint(painter, option, widget);
	float lod = QStyleOptionGraphicsItem::levelOfDetailFromTransform(painter->worldTransform());
	if (m_uiAttr)
	{
		if (QSharedPointer<QPolygonF> visualHull = m_uiAttr->getVisualHull())
		{
			if (m_lodStatus & (LOD_EXPANDED | LOD_HIGHLIGHTED))
			{
				float viewWidth = 1.0;
				float actualWidth = 25;
				float penWidth = actualWidth * lod < viewWidth ? viewWidth / lod : actualWidth;

				QPen pen(QColor(128,128,128,255), penWidth * 4, Qt::DashDotLine, Qt::FlatCap);
				if (m_interactionFlag & IS_FOCUSED)
				{
					pen.setColor(QColor(239,64,27,255));
				}
				else if (m_interactionFlag & IS_MOUSE_HOVER)
				{
					pen.setColor(QColor(252,185,73,255));
				}

				QPen gradPen;
				painter->setBrush(Qt::NoBrush);
				QPainterPath path;
				path.addPolygon(*visualHull);
				QColor borderColor(128,128,128,255);// = UISetting::getTerritoryColor(colorID).lighter(80);
				float width = penWidth * 1.5;
				gradPen.setWidthF(width);
				//gradPen.setColor(borderColor);
				gradPen.setColor(pen.color());
				painter->setPen(gradPen);
				painter->drawPolygon(*visualHull);

				QVector<qreal> dashes;
				qreal space = 0.5;
				dashes << (space * 2.f) << space ;
				pen.setDashPattern(dashes);
				painter->setPen(pen);
				painter->setBrush(Qt::NoBrush);
				painter->drawPolygon(*visualHull);

				int level = m_uiAttr->level();
				QColor classColor = UISetting::getBackgroundRenderer().getHeightColorWithLod(level, lod);
				//QBrush classBrush(classColor);
				classColor.setAlphaF(1.f);
				QBrush classBrush(classColor);
				painter->setBrush(classBrush);
				painter->setPen(Qt::NoPen);
				painter->drawPolygon(*visualHull);

				// draw entries
				const QVector<QPointF>& inEntries = m_uiAttr->getInEntries();
				const QVector<QPointF>& outEntries = m_uiAttr->getOutEntries();
				const QVector<QPointF>& inNormals = m_uiAttr->getInNormals();
				const QVector<QPointF>& outNormals = m_uiAttr->getOutNormals();

				QPen arrowPen(QColor(218,73,73,255));
				arrowPen.setWidthF(penWidth * 0.7);
				arrowPen.setJoinStyle(Qt::RoundJoin);
				arrowPen.setCapStyle(Qt::RoundCap);

				QPen gatePen;
				gatePen.setWidthF(penWidth * 4);
				gatePen.setCapStyle(Qt::FlatCap);
				gatePen.setColor(QColor(218,73,73));

				painter->setBrush(QBrush(QColor(218,73,73,255)));
				float entryWidth = penWidth * 2;
				bool showArrow = actualWidth * lod > 1.2;

				for (int i = 0; i < inEntries.size(); ++i)
				{
					QPointF nor  = inNormals[i] * entryWidth;
					const QPointF& head = inEntries[i];
					QPointF tail = head - nor;
					QPointF gateBeg = head - nor * 0.05;
					QPointF gateEnd = head + nor * 1.05;
					painter->setPen(gatePen);
					painter->drawLine(gateBeg, gateEnd);

					if (showArrow)
					{
						painter->setPen(arrowPen);
						UIUtility::drawArrow(painter, head, head-nor * 2, 0.5, 0.7);
					}
				}

				arrowPen.setColor(QColor(122,193,98,255));
				gatePen.setColor(QColor(122,193,98,255));
				painter->setBrush(QBrush(QColor(122,193,98,255)));
				for (int i = 0; i < outEntries.size(); ++i)
				{
					const QPointF& tail = outEntries[i];
					QPointF nor  = outNormals[i] * entryWidth;
					QPointF gateBeg = tail - nor * 0.05;
					QPointF gateEnd = tail + nor * 1.05;
					painter->setPen(gatePen);
					painter->drawLine(gateBeg, gateEnd);

					if (showArrow)
					{
						QPointF head = tail - nor * 2;
						painter->setPen(arrowPen);
						UIUtility::drawArrow(painter, head, tail, 0.5, 0.7);
					}
				}
			}
			else if (m_lodStatus == LOD_FOLDED)
			{
				QBrush brush(QColor(187,182,164));
				if (m_interactionFlag & IS_MOUSE_HOVER)
				{
					brush.setColor(QColor(165,150,112));
				}

				painter->setPen(Qt::NoPen);
				painter->setBrush(brush);
				painter->drawPolygon(*visualHull);
			}
		}
	}

	if(m_lodStatus == LOD_EXPANDED)
	{
		QFont titleFont("Arial", m_radius * 0.1);
		painter->setFont(titleFont);
		float alphaW = (lod - 0.1) / (0.6 - 0.1f);
		alphaW = max(min(alphaW, 1.f), 0.f);
		painter->setPen(QPen(QColor(20,20,20,30 * (1.f - alphaW))));
		UIUtility::drawText(painter, m_name, titleFont, QPointF(), Qt::AlignHCenter|Qt::AlignVCenter);
	}
}

QPainterPath CodeAtlas::ClassUIItem::shape() const
{
	QPainterPath path;
	if (m_uiAttr)
	{
		QSharedPointer<QPolygonF> hull = m_uiAttr->getVisualHull();
		if (hull)
			path.addPolygon(*hull);
	}
	return path;
}
