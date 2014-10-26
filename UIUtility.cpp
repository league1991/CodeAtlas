#include "stdafx.h"
#include "UIUtility.h"
#include "NodeUIItem.h"

using namespace CodeAtlas;
void CodeAtlas::UIUtility::drawArrow( QPainter* painter, const QPointF& headPnt, const QPointF& tailPnt, const float arrowHeadRatio, const float arrowWidthRatio )
{
	QPointF dir = tailPnt - headPnt;
	dir *= arrowHeadRatio;
	QPointF nor(dir.y(), -dir.x());
	nor *= (0.5f * arrowWidthRatio);
	painter->drawLine(headPnt + dir * 0.15, tailPnt);

	QPointF arrowWing = headPnt + dir;
	QPointF arrowHead[3] = {arrowWing + nor, headPnt, arrowWing - nor};
	painter->drawPolyline(arrowHead, 3);
}

void CodeAtlas::UIUtility::drawText( QPainter* painter, const QString& txt, const QFont& font, const QPointF& basePoint, unsigned alignFlag /*= Qt::AlignHCenter | Qt::AlignTop */ )
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
NodeUIItem::Ptr CodeAtlas::UIUtility::getUIItem( const SymbolNode::Ptr& node )
{
	if (!node.isNull())
	{
		if(UIElementAttr::Ptr uiAttr = node->getAttr<UIElementAttr>())
		{
			if (NodeUIItem::Ptr uiItem = uiAttr->getUIItem())
			{
				return uiItem;
			}
		}
	}
	return NodeUIItem::Ptr();
}


