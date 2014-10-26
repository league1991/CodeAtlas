#include "stdafx.h"
using namespace CodeAtlas;
bool CodeAtlas::LodSetting::setLodStatus( const SymbolNode::Ptr& node, LodStatus lodStatus )
{
	UIItem::Ptr item = getUIItem(node);
	if (item.isNull())
		return false;
	item->setLodStatus(lodStatus);
	return true;
}

CodeAtlas::LodStatus AbsoluteLodSetting::computeLod( UIItem::Ptr& item, float lod )
{
	LodStatus ls = LOD_INVALID;
	if (item.isNull())
		return ls;
	if (lod < m_foldedLod)
		ls = LOD_INVISIBLE;
	else if (lod < m_expandedLod)
		ls = LOD_FOLDED;
	else
		ls = LOD_EXPANDED;
	item->setLodStatus(ls);
	return ls;
}
UIItem::Ptr LodSetting::getUIItem( const SymbolNode::Ptr& node )
{
	if (!node.isNull())
	{
		if(UIElementAttr::Ptr uiAttr = node->getAttr<UIElementAttr>())
		{
			if (UIItem::Ptr uiItem = uiAttr->getUIItem())
			{
				return uiItem;
			}
		}
	}
	return UIItem::Ptr();
}

void CodeAtlas::AbsoluteLodSetting::setLodThreshold( float foldedLod, float expandedLod )
{
	m_foldedLod = foldedLod;
	m_expandedLod = expandedLod;
}

void CodeAtlas::LodManager::computeEdgeLod(SymbolNode::Ptr& parent)
{
	for (SymbolNode::ChildIterator pChild = parent->childBegin();
		pChild != parent->childEnd(); ++pChild)
	{
		const SymbolInfo& childInfo = pChild.key();
		SymbolNode::Ptr& child = pChild.value();
		if (childInfo.elementType() & SymbolInfo::Block)
			continue;

		SymEdgeIter outEdgeIt(child, SymbolNode::EDGE_OUT);
		for (SymbolEdge::Ptr edge; edge = *outEdgeIt; ++outEdgeIt)
		{
			EdgeUIElementAttr::Ptr uiAttr = edge->getAttr<EdgeUIElementAttr>();
			if (uiAttr.isNull())
				continue;
			
			EdgeUIItem::Ptr uiItem = uiAttr->getUiItem();
			if (!uiItem)
				continue;

			SymbolNode::Ptr pSrc = edge->srcNode().toStrongRef();
			SymbolNode::Ptr pTar = edge->tarNode().toStrongRef();

			LodStatus lodStatus;
			if (pSrc && pTar)
			{
				UIElementAttr::Ptr srcUiAttr = pSrc->getAttr<UIElementAttr>();
				UIElementAttr::Ptr tarUiAttr = pTar->getAttr<UIElementAttr>();
				SymbolInfo srcInfo = pSrc->getSymInfo();
				SymbolInfo tarInfo = pTar->getSymInfo();
				bool isVarRefEdge = (srcInfo.elementType() & SymbolInfo::Variable) || (tarInfo.elementType() & SymbolInfo::Variable);

				if (!srcUiAttr || !tarUiAttr ||
					!srcUiAttr->getUIItem()  ||
					!srcUiAttr->getUIItem()  ||
					srcUiAttr->getUIItem()->getLodStatus() == LOD_INVISIBLE ||
					tarUiAttr->getUIItem()->getLodStatus() == LOD_INVISIBLE
					)
				{
					lodStatus = LOD_INVISIBLE;
				}
				else
				{
					QRectF brect = uiItem->boundingRect();
					float viewRadius = UIUtility::rectRadius(brect) * m_lod;
					float viewEdgeWidth = uiItem->getEdgeWidth() * m_lod;
					const float hideRadius = 3;
					const float showRadius = 150;
					float sizeAlpha = MathUtility::linearInterpolateClamp(QPointF(hideRadius,0), QPointF(showRadius,1), viewRadius);
					float edgeAlpha = MathUtility::linearInterpolateClamp(QPointF(0.2,0), QPointF(0.5,1), viewEdgeWidth);
					float alpha = min(0.5f * sizeAlpha + edgeAlpha, 1.f);
					float edgeWidth = uiItem->getEdgeWidth();
					bool  isHighlighted = srcUiAttr->getUIItem()->isSelected() || tarUiAttr->getUIItem()->isSelected();

					float level = isVarRefEdge ? 0 : srcUiAttr->level();
					float highLightZValue = NodeUIItem::getNodeBaseZValue() -1;
					float zValue = min(EdgeUIItem::getEdgeBaseZValue() + level, highLightZValue - 1.f);

					if (isHighlighted)
					{
						srcUiAttr->getUIItem()->setLodStatus(LOD_HIGHLIGHTED);
						tarUiAttr->getUIItem()->setLodStatus(LOD_HIGHLIGHTED);
						lodStatus = LOD_HIGHLIGHTED;
						uiItem->setDisplayAlpha(1.f);
						float highLightedWidth =  uiItem->getHighlightedEdgeViewWidth();
						float dr = highLightedWidth > edgeWidth * m_lod ? highLightedWidth / m_lod : edgeWidth;
						uiItem->setEdgeWidthToDraw(dr);
						zValue = highLightZValue;
					}
					else if (alpha > 0.2)
					{
						float viewWidth = uiItem->getInvariantEdgeViewWidth();
						if (viewWidth > edgeWidth * m_lod)
						{
							lodStatus = LOD_FOLDED;
							uiItem->setDisplayAlpha(alpha);
							uiItem->setEdgeWidthToDraw(viewWidth / m_lod);
						}
						else
						{
							lodStatus = LOD_EXPANDED;
							uiItem->setDisplayAlpha(1.f);
							uiItem->setEdgeWidthToDraw(edgeWidth);
						}
					}
					else
						lodStatus = LOD_INVISIBLE;

					if (uiItem->zValue() != zValue)
						uiItem->setZValue(zValue);
				}
			}
			else
				lodStatus = LOD_INVISIBLE;

			uiItem->setLodStatus(lodStatus);
		}

		computeEdgeLod(child);
	}
}
void CodeAtlas::LodManager::computeChildLod(SymbolNode::Ptr& parent, 
											LodStatus parentStatus, 
											float parentProjD, 
											float parentClassD,
											int depth)
{
	for (SymbolNode::ChildIterator pChild = parent->childBegin();
		pChild != parent->childEnd(); ++pChild)
	{
		const SymbolInfo& childInfo = pChild.key();
		SymbolNode::Ptr& child = pChild.value();
		if (childInfo.elementType() & SymbolInfo::Block)
			continue;

		// determine node lod status
		NodeUIItem::Ptr uiItem = UIUtility::getUIItem(child);
		LodStatus lodStatus = parentStatus;
		float projD = parentProjD, classD = parentClassD;
		if (uiItem.isNull())
		{
			lodStatus = parentStatus;
		}
		else if (!uiItem->mapRectToScene(uiItem->boundingRect()).intersects(m_viewRect))
		{
			lodStatus = parentStatus;
		}
		// hide all childs
		else if (parentStatus & (LOD_INVISIBLE | LOD_FOLDED))
		{
			lodStatus = LOD_INVISIBLE;
			uiItem->setLodStatus(LOD_INVISIBLE);
		}
		else
		{
			SymbolInfo::ElementType type = childInfo.elementType();
			float viewDiam = uiItem->getEntityRadius() * m_lod * 2;
						
			if (type & (SymbolInfo::Class | SymbolInfo::Enum | SymbolInfo::Namespace))
			{
				classD = viewDiam;
				if (m_lod < 0.01 && parentProjD < 150)
					lodStatus = LOD_INVISIBLE;
				else if (m_lod > 0.1 || viewDiam > 200)
					lodStatus = LOD_EXPANDED;
				else
					lodStatus = LOD_FOLDED;
			}
			else if (type & (SymbolInfo::FunctionSignalSlot | SymbolInfo::Variable | SymbolInfo::Enumerator | SymbolInfo::Macro))
			{ 
				if (m_lod > 0.4  || viewDiam > 10)
					lodStatus = LOD_EXPANDED;
				else if (m_lod < 0.1 && parentProjD < 150)
					lodStatus = LOD_INVISIBLE;
				else
					lodStatus = LOD_FOLDED;
			}
			else if (type & (SymbolInfo::Project))
			{
				projD = viewDiam;
// 				if (m_lod < 0.01)
// 					lodStatus = LOD_INVISIBLE;
				if (m_lod > 0.01 || viewDiam > 150)
					lodStatus = LOD_EXPANDED;
				else
					lodStatus = LOD_FOLDED;
			}
			else
			{
				// always expanded
				lodStatus = LOD_EXPANDED;
			}
			uiItem->setLodStatus(lodStatus);
			
		}

		if (uiItem)
		{
			bool newSelectableFlag = lodStatus != LOD_INVISIBLE;
			QGraphicsItem::GraphicsItemFlags flags = uiItem->flags();
			if (flags.testFlag(QGraphicsItem::ItemIsSelectable) != newSelectableFlag)
				uiItem->setFlag(QGraphicsItem::ItemIsSelectable, lodStatus != LOD_INVISIBLE);
		}
		computeChildLod(child, lodStatus, projD, classD, depth + 1);
	}
}

void CodeAtlas::LodManager::setAllEdgeExpanded()
{
	SmartDepthIterator nIt( m_root, DepthIterator::PREORDER, 
		SymbolInfo::All & ~(SymbolInfo::Namespace),
		SymbolInfo::All & ~(SymbolInfo::Block));

	for (SymbolNode::Ptr node; node = *nIt; ++nIt)
	{
		SymEdgeIter eIt(node, SymbolNode::EDGE_OUT);
		for (SymbolEdge::Ptr edge; edge = *eIt; ++eIt)
		{
			EdgeUIElementAttr::Ptr uiAttr = edge->getAttr<EdgeUIElementAttr>();
			if (uiAttr.isNull())
				continue;

			if (EdgeUIItem::Ptr uiItem = uiAttr->getUiItem())
			{
				uiItem->setLodStatus(LOD_EXPANDED);
			}
		}
	}
}

void CodeAtlas::LodManager::countProjRadius()
{
	m_minProjRadius = FLT_MAX;
	m_maxProjRadius = 0;
	m_maxClassRadius = 0;

	SmartDepthIterator nIt( m_root, DepthIterator::PREORDER,
		SymbolInfo::Project | SymbolInfo::Class,
		SymbolInfo::Root | SymbolInfo::Project | SymbolInfo::Namespace | SymbolInfo::Class);

	float totalRadius = 0;
	int projCount = 0;
	for (SymbolNode::Ptr node; node = *nIt; ++nIt)
	{

		if (UIElementAttr::Ptr uiAttr = node->getAttr<UIElementAttr>())
		{
			float r = uiAttr->radius();
			if (node->getSymInfo().isClass())
			{
				m_maxClassRadius = max(m_maxClassRadius, r);
			}
			else
			{
				m_minProjRadius = min(m_minProjRadius, r);
				m_maxProjRadius = max(m_maxProjRadius, r);
				totalRadius += r;
				projCount++;
			}
		}
	}

	m_avgProjRadius = totalRadius / projCount;
}

void CodeAtlas::LodManager::compute()
{
	countProjRadius();
	setAllEdgeExpanded();
	computeChildLod(m_root, LOD_EXPANDED);
	computeEdgeLod(m_root);
}
