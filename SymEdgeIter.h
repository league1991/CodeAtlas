#pragma once
namespace CodeAtlas
{
	class SymEdgeIter
	{		
	public:
		typedef SymbolNode::EdgeDirType EdgeDirType;
		SymEdgeIter(const SymbolNode::Ptr& node,
					EdgeDirType type = SymbolNode::EDGE_OUT,
					unsigned edgeMask = SymbolEdge::EDGE_ALL);

		void setNode(const SymbolNode::Ptr& node);

		// dereference
		SymbolEdge::Ptr operator*();

		// advance the iterator
		SymbolEdge::Ptr operator++();
	private:
		void setInEdge();
		void setOutEdge();

		QMultiHash<SymbolEdge::EdgeType,  SymbolEdge::Ptr>::Iterator m_pEdge;
		unsigned        m_edgeMask;
		EdgeDirType     m_edgeType;
		SymbolNode::Ptr m_node;
	};
}