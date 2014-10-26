#include "StdAfx.h"
#include "SymbolEdge.h"

using namespace CodeAtlas;

SymbolEdge::~SymbolEdge(void)
{
	for (int i = 0; i < N_SYMBOL_EDGE_ATTR; ++i)
	{
		if (m_attrList[i])
		{
			m_attrList[i] = SymbolEdgeAttr::Ptr();
		}
	}
}


QString CodeAtlas::SymbolEdge::toString() const
{
	SymbolNode::WeakPtr wpSrc = srcNode();
	SymbolNode::WeakPtr wpTar = tarNode();

	if (wpSrc.isNull() || wpTar.isNull())
		return QString("bad edge!");

	QString res;
	SymbolNode::Ptr     pSrc = wpSrc.toStrongRef();
	SymbolNode::Ptr     pTar = wpTar.toStrongRef();
	SymbolInfo          srcInfo = pSrc->getSymInfo();
	SymbolInfo			tarInfo = pTar->getSymInfo();
	res += srcInfo.name() + " " + srcInfo.type() + "  --->  ";
	res += tarInfo.name() + " " + tarInfo.type() + " " + QString::number(m_strength) + "\n";
	res += "type" + QString("%1").arg(m_type) + "\n" + 
		"strength" + QString("%1").arg(m_strength);

	return res;
}

CodeAtlas::SymbolEdge::SymbolEdge( EdgeType type ) :m_type(type), m_strength(1)
{
}

