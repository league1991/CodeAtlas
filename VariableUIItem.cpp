#include "stdafx.h"
using namespace CodeAtlas;

CodeAtlas::ImageArray CodeAtlas::VariableUIItem::m_publicImgArray[3];

CodeAtlas::ImageArray CodeAtlas::VariableUIItem::m_privateImgArray[3];

CodeAtlas::ImageArray CodeAtlas::VariableUIItem::m_protectedImgArray[3];

void VariableUIItem::initUIItemModel()
{
	m_variableModelList.clear();

	CircleItemModel::Ptr pCircleModel = CircleItemModel::Ptr(new CircleItemModel);
	pCircleModel->setPen(QPen(QColor(0,0,0,255)),       UI_NORMAL);
	pCircleModel->setPen(QPen(QColor(128,128,128,255)), UI_HOVERING);
	pCircleModel->setPen(QPen(QColor(255,0,0,255)),     UI_SELECTED);
	pCircleModel->setBrush(QBrush(QColor(123,198,101)), UI_NORMAL);
	pCircleModel->setBrush(QBrush(QColor(133,208,111)), UI_HOVERING);
	pCircleModel->setBrush(QBrush(QColor(93,198,101)), UI_SELECTED);
	pCircleModel->setRectStretchFactor(1.5f,1.5f);

	TextItemModel::Ptr   pTextModel = TextItemModel::Ptr(new TextItemModel);
	pTextModel->setRectAlignMode(Qt::AlignHCenter | Qt::AlignBottom);
	pTextModel->setTxtAlignMode(Qt::AlignHCenter | Qt::AlignTop);
	pTextModel->setRectStretchFactor(8,0.5);
	pTextModel->setEntityStretchFactor(1.5f,1.5f);
	pTextModel->setFontShowSizeRange(20,150);

	pTextModel->setPen(QPen(QColor(0,0,0,255)), UI_NORMAL);
	pTextModel->setPen(QPen(QColor(0,0,0,255)), UI_HOVERING);
	pTextModel->setPen(QPen(QColor(255,0,0,255)), UI_SELECTED);
	QFont font("Arial");
	font.setPointSizeF(12);
	pTextModel->setFont(font, UI_NORMAL);
	font.setPointSizeF(72);
	pTextModel->setFont(font, UI_HOVERING);
	pTextModel->setFont(font, UI_SELECTED);

	VariableItemModel::Ptr pVarModel(new VariableItemModel);
	// 	m_variableModelList.append(pCircleModel);
	// 	m_variableModelList.append(pTextModel);
	m_variableModelList.append(pVarModel);
}

VariableUIItem::VariableUIItem( const SymbolNode::WeakPtr& node, UIItem* parent /*= 0*/ ) :NodeUIItem(node, parent)
{
	if (m_variableModelList.size() == 0)
		initUIItemModel();
}

void VariableUIItem::getModeThreshold( float& minNormal, float& maxNormal, float& minFrozen, float& maxFrozen )
{
	minNormal = 0.2f;
	maxNormal = 100.f;
	minFrozen = 100.f;
	maxFrozen = 200.f;
}

void CodeAtlas::VariableUIItem::initIconList()
{
	if (m_publicImgArray[0].isEmpty())
		m_publicImgArray[0].loadImage(QImage(":/CodeAtlas/Resources/publicVar.png"), QSize(150,150));
	if (m_protectedImgArray[0].isEmpty())
		m_protectedImgArray[0].loadImage(QImage(":/CodeAtlas/Resources/protectedVar.png"), QSize(150,150));
	if (m_privateImgArray[0].isEmpty())
		m_privateImgArray[0].loadImage(QImage(":/CodeAtlas/Resources/privateVar.png"), QSize(150,150));

	if (m_publicImgArray[1].isEmpty())
		m_publicImgArray[1].loadImage(QImage(":/CodeAtlas/Resources/publicVarHover.png"), QSize(150,150));
	if (m_protectedImgArray[1].isEmpty())
		m_protectedImgArray[1].loadImage(QImage(":/CodeAtlas/Resources/protectedVarHover.png"), QSize(150,150));
	if (m_privateImgArray[1].isEmpty())
		m_privateImgArray[1].loadImage(QImage(":/CodeAtlas/Resources/privateVarHover.png"), QSize(150,150));

	if (m_publicImgArray[2].isEmpty())
		m_publicImgArray[2].loadImage(QImage(":/CodeAtlas/Resources/publicVarSelected.png"), QSize(150,150));
	if (m_protectedImgArray[2].isEmpty())
		m_protectedImgArray[2].loadImage(QImage(":/CodeAtlas/Resources/protectedVarSelected.png"), QSize(150,150));
	if (m_privateImgArray[2].isEmpty())
		m_privateImgArray[2].loadImage(QImage(":/CodeAtlas/Resources/privateVarSelected.png"), QSize(150,150));
}

void CodeAtlas::VariableUIItem::paint( QPainter *painter, const QStyleOptionGraphicsItem *option/* =0 */, QWidget *widget /* = 0 */ )
{
	if (m_lodStatus == LOD_INVISIBLE)
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

	const float baseRadius = 51;
	int nRows = imgArray[ithArray].getNumRows();
	float maxRadius = baseRadius * nRows;
	float boundRadius= maxRadius * 3;
	QRectF rect(-boundRadius,-boundRadius, boundRadius* 2.f, boundRadius*2.f);

	int sizeIdx = m_radius / baseRadius;
	int levelIdx= m_level;
	const QImage* img = &imgArray[ithArray].getImage(sizeIdx, levelIdx);

	//QRectF drawRect = UIUtility::adjustRect(m_entityRect, 3.f, 3.f);
	painter->drawImage(rect, *img);
}

