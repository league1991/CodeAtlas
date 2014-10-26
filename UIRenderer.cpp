#include "stdafx.h"
#include "UIRenderer.h"

using namespace CodeAtlas;
void CodeAtlas::IconRenderer::paint( QPainter* painter, const QImage& img, const QRectF& rect, EffectType mode /*= ICON_NORMAL*/ )
{
	if (!checkRectSize(painter, rect))
		return;

	QPainter::CompositionMode oriMode = painter->compositionMode();
	painter->setPen(Qt::NoPen);
	painter->drawImage(rect, img);
	switch(mode)
	{
	case EFFECT_LIGHTEN:
		painter->setCompositionMode(QPainter::CompositionMode_ColorDodge);
		painter->setBrush(m_lightenColor);
		painter->drawRect(rect);
		break;
	case EFFECT_HIGHLIGHTED:
		painter->setCompositionMode(QPainter::CompositionMode_ColorBurn);
		painter->setBrush(m_lightenColor);
		painter->drawRect(rect);
		break;
	//default:
	}
	painter->setCompositionMode(oriMode);
}


bool CodeAtlas::UIRenderer::checkRectSize( QPainter* painter, const QRectF& rect )
{
	float lod = QStyleOptionGraphicsItem::levelOfDetailFromTransform(painter->worldTransform());
	QSizeF viewSize= rect.size()*lod;
	float curSize = viewSize.width() > viewSize.height() ? viewSize.width() : viewSize.height();
	if (curSize < m_minSize || curSize > m_maxSize)
		return false;
	return true;
}

void CodeAtlas::UIRenderer::setShowSizeRange( float minSize, float maxSize )
{
	m_minSize = minSize;
	m_maxSize = maxSize;
}

// void CodeAtlas::TextRenderer::paintBelow( QPainter* painter, const QString& text, const QRectF& rect, EffectType mode)
// {
// 	if (!checkRectSize(painter, rect))
// 		return;
// 
// 	painter->setBrush(m_textColor[mode]);
// 	QRectF txtRect = getTextRect(rect);
// 
// 	m_font.setPointSizeF(5);
// 	painter->setFont(m_font);
// 	painter->scale(0.01,0.01);
// 	painter->drawText(txtRect, Qt::AlignHCenter | Qt::AlignTop, text);
// }

void CodeAtlas::TextRenderer::paintRect( QPainter* painter, const QString& text, const QRectF& rect, EffectType mode /*= UIRenderer::EFFECT_NORMAL*/ ,
										unsigned int alignFlag )
{
	painter->setPen(m_textColor[mode]);

	m_font.setPointSizeF(rect.height()*0.3+1.f);
	painter->setFont(m_font);

	//painter->rotate(-30);
	painter->drawText(rect, alignFlag, text);
}
