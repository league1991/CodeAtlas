#include "stdafx.h"

using namespace CodeAtlas;

void EdgeUIElementAttr::setUIItem( const QSharedPointer<EdgeUIItem>& uiItem )
{
	m_uiItem = uiItem;
}

void CodeAtlas::EdgeUIElementAttr::buildOrUpdateUIItem( const SymbolEdge::Ptr& thisEdge, NodeUIItem * parent /*= NULL*/ )
{
	m_edge = thisEdge.toWeakRef();
	m_uiItem = EdgeUIItem::creator(thisEdge, parent);
	m_uiItem->buildUI();
}

CodeAtlas::EdgeUIElementAttr::~EdgeUIElementAttr()
{

}

CodeAtlas::SymbolEdgeAttr::~SymbolEdgeAttr()
{

}
