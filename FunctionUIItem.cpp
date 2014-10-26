#include "stdafx.h"
using namespace CodeAtlas;


CodeAtlas::ImageArray CodeAtlas::FunctionUIItem::m_protectedImgArray[3];
CodeAtlas::ImageArray CodeAtlas::FunctionUIItem::m_privateImgArray[3];
CodeAtlas::ImageArray CodeAtlas::FunctionUIItem::m_publicImgArray[3];


void FunctionUIItem::initUIItemModel()
{
	m_funcModelList.clear();

	CircleItemModel::Ptr pCircleModel = CircleItemModel::Ptr(new CircleItemModel);
	pCircleModel->setRectStretchFactor(0.6,0.6);
	pCircleModel->setPen(QPen(QColor(0,0,0,255)),       UI_NORMAL);
	pCircleModel->setPen(QPen(QColor(128,128,128,255)), UI_HOVERING);
	pCircleModel->setPen(QPen(QColor(255,0,0,255)),     UI_SELECTED);
	pCircleModel->setBrush(QBrush(QColor(183,198,201)), UI_NORMAL);
	pCircleModel->setBrush(QBrush(QColor(193,208,211)), UI_HOVERING);
	pCircleModel->setBrush(QBrush(QColor(153,198,201)), UI_SELECTED);

	TextItemModel::Ptr   pTextModel = TextItemModel::Ptr(new TextItemModel);
	pTextModel->setRectAlignMode(Qt::AlignHCenter | Qt::AlignBottom);
	pTextModel->setTxtAlignMode(Qt::AlignHCenter | Qt::AlignTop);
	pTextModel->setRectStretchFactor(8,0.3);
	pTextModel->setEntityStretchFactor(0.6,0.6);
	pTextModel->setFontShowSizeRange(20,350);
	QFont font("Arial");
	font.setPointSizeF(12);
	pTextModel->setPen(QPen(QColor(0,0,0,255)), UI_NORMAL);
	pTextModel->setPen(QPen(QColor(0,0,0,255)), UI_HOVERING);
	pTextModel->setPen(QPen(QColor(255,0,0,255)), UI_SELECTED);
	pTextModel->setFont(font, UI_NORMAL);
	font.setPointSizeF(48);
	pTextModel->setFont(font, UI_HOVERING);
	pTextModel->setFont(font, UI_SELECTED);

	// 	m_funcModelList.append(pCircleModel);
	// 	m_funcModelList.append(pTextModel);
	m_funcModelList.append(FunctionItemModel::Ptr(new FunctionItemModel()));
}

FunctionUIItem::FunctionUIItem( const SymbolNode::WeakPtr& node, UIItem* parent /*= 0*/ ) :NodeUIItem(node, parent)
{
	if (m_funcModelList.size() == 0)
		initUIItemModel();
}

void FunctionUIItem::getModeThreshold( float& minNormal, float& maxNormal, float& minFrozen, float& maxFrozen )
{
	minNormal = 0.2f;
	maxNormal = 100.f;
	minFrozen = 100.f;
	maxFrozen = 200.f;
}

void FunctionUIItem::drawVectorBuilding(QPainter* painter)
{
	const float unitOffsetW = 15;
	const float unitOffsetH = 30;
	float levelOffsetW = (m_level+1) * unitOffsetW;
	float levelOffsetH = (m_level+1) * unitOffsetH;
	float wOffset = m_entityRect.width() * 0.2;
	float hOffset = m_entityRect.height() * 0.2;
	QRectF baseRect= m_entityRect.adjusted(wOffset, hOffset, -wOffset, -hOffset);
	QRectF ceilRect = baseRect.translated(-levelOffsetW, -levelOffsetH);

	painter->setPen(Qt::NoPen);
	painter->setBrush(QBrush(QColor(189,183,170)));
	painter->drawRoundedRect(m_entityRect, 25,25, Qt::RelativeSize);

	QColor ceilColor(217,228,236);
	if (this->isSelected())
		ceilColor = UIUtility::adjustLightness(ceilColor, -0.1);
	else if (m_isMouseHover)
		ceilColor = UIUtility::adjustLightness(ceilColor, +0.1);

	QColor bottomColor = UIUtility::adjustLightness(ceilColor, -0.3);
	QColor rightColor = UIUtility::adjustLightness(ceilColor, -0.1);

	QPen   edgePen(UIUtility::adjustLightness(ceilColor, -0.5));
	edgePen.setWidthF(3);
	painter->setBrush(QBrush(ceilColor));
	painter->setPen(edgePen);
	painter->drawRect(ceilRect);

	QPointF rightShape[] = {ceilRect.topRight(), ceilRect.bottomRight(), baseRect.bottomRight(), baseRect.topRight()};
	QPointF bottomShape[]= {ceilRect.bottomLeft(), baseRect.bottomLeft(), baseRect.bottomRight(), ceilRect.bottomRight()};

	painter->setBrush(QBrush(bottomColor));
	painter->drawPolygon(bottomShape, 4);

	painter->setBrush(QBrush(rightColor));
	painter->drawPolygon(rightShape, 4);

	edgePen.setWidthF(7);
	painter->setPen(edgePen);
	painter->setBrush(Qt::NoBrush);
	int level = (int)m_level;
	float padding = baseRect.width() * 0.2;
	QPointF floorPnts[] = {baseRect.bottomLeft(), baseRect.bottomRight(), baseRect.topRight()};
	floorPnts[0].rx() += padding;
	floorPnts[2].ry() += padding;
	for (int i = 1; i <= level; ++i)
	{
		floorPnts[0] -= QPointF(unitOffsetW, unitOffsetH);
		floorPnts[1] -= QPointF(unitOffsetW, unitOffsetH);
		floorPnts[2] -= QPointF(unitOffsetW, unitOffsetH);

		painter->drawPolyline(floorPnts, 3);
	}
}

void CodeAtlas::FunctionUIItem::paint( QPainter *painter, const QStyleOptionGraphicsItem *option/* =0 */, QWidget *widget /* = 0 */ )
{
	if (m_lodStatus == LOD_INVISIBLE )
		return;
	//NodeUIItem::paint(painter, option, widget);

	//if (m_lodStatus == LOD_FOLDED)
	//	return;

	//drawVectorBuilding(painter);
	//return;
	initIconList();

	SymbolInfo info = m_node.toStrongRef()->getSymInfo();
	ImageArray* imgArray;
	if (info.elementType() & SymbolInfo::PublicMember)
	{
		imgArray = m_publicImgArray;
	}
	else if (info.elementType() & SymbolInfo::ProtectedMember)
	{
		imgArray = m_protectedImgArray;
	}
	else
	{
		imgArray = m_privateImgArray;
	}

	int ithArray = 0;
	if (m_interactionFlag & IS_FOCUSED)
		ithArray = 2;
	else if (m_interactionFlag & IS_MOUSE_HOVER)
		ithArray = 1;
	int nRows = imgArray[ithArray].getNumRows();

	const float baseRadius = 30;
	float maxRadius = baseRadius * nRows;
	float boundRadius= maxRadius * 2;
	QRectF rect(-boundRadius,-boundRadius, boundRadius* 2.f, boundRadius*2.f);

	int sizeIdx = m_radius / baseRadius;
	int levelIdx= m_level-1;
	const QImage* img = &imgArray[ithArray].getImage(sizeIdx, levelIdx);

	//QRectF drawRect = UIUtility::adjustRect(m_entityRect, 3.f, 3.f);
	painter->drawImage(rect, *img);
}

void CodeAtlas::FunctionUIItem::initIconList()
{
	if (m_publicImgArray[0].isEmpty())
		m_publicImgArray[0].loadImage(QImage(":/CodeAtlas/Resources/publicFunc.png"), QSize(150,150));
	if (m_publicImgArray[1].isEmpty())
		m_publicImgArray[1].loadImage(QImage(":/CodeAtlas/Resources/publicFuncHover.png"), QSize(150,150));
	if (m_publicImgArray[2].isEmpty())
		m_publicImgArray[2].loadImage(QImage(":/CodeAtlas/Resources/publicFuncSelected.png"), QSize(150,150));

	if (m_protectedImgArray[0].isEmpty())
		m_protectedImgArray[0].loadImage(QImage(":/CodeAtlas/Resources/protectedFunc.png"), QSize(150,150));
	if (m_protectedImgArray[1].isEmpty())
		m_protectedImgArray[1].loadImage(QImage(":/CodeAtlas/Resources/protectedFuncHover.png"), QSize(150,150));
	if (m_protectedImgArray[2].isEmpty())
		m_protectedImgArray[2].loadImage(QImage(":/CodeAtlas/Resources/protectedFuncSelected.png"), QSize(150,150));

	if (m_privateImgArray[0].isEmpty())
		m_privateImgArray[0].loadImage(QImage(":/CodeAtlas/Resources/privateFunc.png"), QSize(150,150));
	if (m_privateImgArray[1].isEmpty())
		m_privateImgArray[1].loadImage(QImage(":/CodeAtlas/Resources/privateFuncHover.png"), QSize(150,150));
	if (m_privateImgArray[2].isEmpty())
		m_privateImgArray[2].loadImage(QImage(":/CodeAtlas/Resources/privateFuncSelected.png"), QSize(150,150));
}