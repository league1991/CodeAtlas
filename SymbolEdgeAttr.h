#pragma once
#define N_SYMBOL_EDGE_ATTR 1

namespace CodeAtlas
{
	class EdgeUIItem;
	class SymbolEdge;
	class NodeUIItem;

	class SymbolEdgeAttr
	{
	public:
		typedef QSharedPointer<SymbolEdgeAttr> Ptr;
		enum AttrType
		{
			ATTR_UIELEMENT  = 0
		};
		virtual ~SymbolEdgeAttr();
	};

	class EdgeUIElementAttr:public SymbolEdgeAttr
	{
	public:
		typedef QSharedPointer<EdgeUIElementAttr> Ptr;
		EdgeUIElementAttr():m_edgeWeight(1.f){}
		~EdgeUIElementAttr();;
		static AttrType					classType(){return ATTR_UIELEMENT;}
		void							setUIItem(const QSharedPointer<EdgeUIItem>& uiItem);
		const QSharedPointer<EdgeUIItem>&getUiItem()const{return m_uiItem;}
		const MatrixXf&					pointList()const{return m_pointList;}
		MatrixXf&						pointList(){return m_pointList;}
		float&							edgeWeight(){return m_edgeWeight;}
		void							buildOrUpdateUIItem( const QSharedPointer<SymbolEdge>& thisEdge, NodeUIItem * parent /*= NULL*/ );
	private:
		float							m_edgeWeight;
		QWeakPointer<SymbolEdge>		m_edge;
		QSharedPointer<EdgeUIItem>      m_uiItem;
		MatrixXf						m_pointList;
	};
}