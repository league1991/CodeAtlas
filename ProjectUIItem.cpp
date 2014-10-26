#include "stdafx.h"
using namespace CodeAtlas;

void ProjectUIItem::initUIItemModel()
{
	m_projModelList.clear();

	CircleItemModel::Ptr pCircleModel = CircleItemModel::Ptr(new CircleItemModel);
	pCircleModel->setRectStretchFactor(0.2f,0.2f);
	//pCircleModel->setRectStretchFactor(1,1);
	pCircleModel->setBrush(QBrush(QColor(201,50, 50, 200)), UI_NORMAL);
	pCircleModel->setBrush(QBrush(QColor(211,50, 50, 200)), UI_HOVERING);
	pCircleModel->setBrush(QBrush(QColor(201,50, 50, 255)), UI_SELECTED);
	pCircleModel->setBrush(QBrush(QColor(201,50, 50, 0)), UI_FROZEN);
	pCircleModel->setPen(QPen(QColor(201,50, 50, 0)), UI_FROZEN);
	//pCircleModel->setViewSizeRange(20, 40);
	pCircleModel->setViewSizeRange(0, 1000);

	TextItemModel::Ptr   pTextModel = TextItemModel::Ptr(new TextItemModel);
	pTextModel->setRectStretchFactor(5,0.2f);
	pTextModel->setEntityStretchFactor(0.2f,0.2f);
	pTextModel->setRectAlignMode(Qt::AlignHCenter|Qt::AlignBottom);
	pTextModel->setTxtAlignMode(Qt::AlignHCenter|Qt::AlignTop);
	pTextModel->setFontShowSizeRange(0.1f, 200.f); 
	pTextModel->setViewSizeRange(20, 200);
	QFont font("Arial");
	pTextModel->setPen(QPen(QColor(0,0,0,128)), UI_NORMAL);
	pTextModel->setPen(QPen(QColor(0,0,0,255)), UI_HOVERING);
	pTextModel->setPen(QPen(QColor(255,0,0,255)), UI_SELECTED);
	pTextModel->setPen(QPen(QColor(0,0,0,50)), UI_FROZEN);
	pTextModel->setFont(font, UI_NORMAL);
	pTextModel->setFont(font, UI_HOVERING);
	pTextModel->setFont(font, UI_SELECTED);

	m_projModelList.append(pCircleModel);
	m_projModelList.append(pTextModel);
}

ProjectUIItem::ProjectUIItem( const SymbolNode::WeakPtr& node, UIItem* parent /*= 0*/ ) :NodeUIItem(node, parent)
{
	if (m_projModelList.size() == 0)
		initUIItemModel();
}

QList<UIItemModel::Ptr>& ProjectUIItem::getUIItemModelList() const
{
	return m_projModelList;
}

void ProjectUIItem::getModeThreshold( float& minNormal, float& maxNormal, float& minFrozen, float& maxFrozen )
{
	minNormal = 0.f; 
	maxNormal = 0.05f;
	minFrozen = 0.05f;
	maxFrozen = 0.2f;
}


void CodeAtlas::ProjectUIItem::paint( QPainter *painter, const QStyleOptionGraphicsItem *option/*=0*/, QWidget *widget /*= 0*/ )
{
	if (m_lodStatus == LOD_INVISIBLE || !m_uiAttr)
		return;

	if (m_lodStatus == LOD_FOLDED)
	{
		//painter->setFont(QFont("Arial", 1000));
		//painter->drawText(0,0, m_name);
	}
	QSharedPointer<QPolygonF> visualHull;
	if (visualHull = m_uiAttr->getVisualHull())
	{
		//painter->setPen(Qt::NoPen);
		//painter->setBrush(QBrush(QColor(200,200,200,50)));
		//painter->setBrush(Qt::NoBrush);
		QPen pen(QColor(128,128,128,100), 0.02 * m_radius + 10, Qt::CustomDashLine, Qt::FlatCap);
		QVector<qreal> dashes;
		qreal space0 = 3;
		qreal space1 = 15;
		dashes << 4 << space0 << 4 << space1 ;
		pen.setDashPattern(dashes);
		painter->setPen(pen);
		painter->drawPolygon(*visualHull);
	}
}
