#include "stdafx.h"

using namespace CodeAtlas;
void CodeAtlas::SymEdgeIter::setOutEdge()
{
	m_pEdge = m_node->outEdgeBegin();
	for (; m_pEdge != m_node->outEdgeEnd(); ++m_pEdge)
	{
		if (m_pEdge.value()->type() & m_edgeMask)
			return;
	}
}

void CodeAtlas::SymEdgeIter::setInEdge()
{
	m_pEdge = m_node->inEdgeBegin();
	for (; m_pEdge != m_node->inEdgeEnd(); ++m_pEdge)
	{
		if (m_pEdge.value()->type() & m_edgeMask)
			return;
	}
}

CodeAtlas::SymEdgeIter::SymEdgeIter( const SymbolNode::Ptr& node, 
									EdgeDirType type /*= EDGE_OUT*/ , 
									unsigned edgeMask /*= SymbolEdge::EDGE_ALL*/):
m_edgeMask(edgeMask), m_edgeType(type)
{
	setNode(node);
}

void CodeAtlas::SymEdgeIter::setNode( const SymbolNode::Ptr& node )
{
	m_node = node;
	switch (m_edgeType)
	{
	case SymbolNode::EDGE_IN:
		setInEdge(); break;
	case SymbolNode::EDGE_OUT:
		setOutEdge(); break;
	default:
		return;
	}
}

SymbolEdge::Ptr CodeAtlas::SymEdgeIter::operator*()
{
	if ((m_edgeType == SymbolNode::EDGE_IN && m_pEdge != m_node->inEdgeEnd()) ||
		(m_edgeType == SymbolNode::EDGE_OUT && m_pEdge != m_node->outEdgeEnd()))
		return *m_pEdge;
	return SymbolEdge::Ptr();
}

SymbolEdge::Ptr CodeAtlas::SymEdgeIter::operator++()
{
	if (m_pEdge == m_node->inEdgeEnd() || m_pEdge == m_node->outEdgeEnd())
		return SymbolEdge::Ptr();
	if (m_edgeType == SymbolNode::EDGE_IN)
	{
		while ((++m_pEdge) != m_node->inEdgeEnd())
		{
			if (m_pEdge.value()->type() & m_edgeMask)
				return *m_pEdge;
		}
	}
	else if (m_edgeType == SymbolNode::EDGE_OUT)
	{
		while ((++m_pEdge) != m_node->outEdgeEnd())
		{
			if (m_pEdge.value()->type() & m_edgeMask)
				return *m_pEdge;
		}
	}
	return SymbolEdge::Ptr();
}
