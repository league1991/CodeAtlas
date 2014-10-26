#pragma once

namespace CodeAtlas
{
	class SymbolNode;
	class SymbolTree;
	class EdgeUIItem;
	class SymbolEdgeAttr;

	class SymbolEdge
	{
		friend class SymbolNode;
		friend class SymbolTree;
	public:
		typedef QSharedPointer<SymbolEdge>	Ptr;
		typedef QWeakPointer<SymbolEdge>	WeakPtr;
		enum EdgeType
		{
			EDGE_NONE			= 0,
			EDGE_VARIABLE_REF	= (0x1),			// symbol dependency, build in SymbolTreeBuilder
			EDGE_PROJ_DEPEND	= (0x1 << 1),		// project dependency, build in SymbolTreeBuilder
			EDGE_FUZZY_DEPEND	= (0x1 << 2),		// Fuzzy dependency, build in FuzzyDependBuilder
			EDGE_PROJ_BUILD_DEPEND = (0x1 << 3),

			EDGE_ALL			= 0xffffffff
		};
		SymbolEdge(EdgeType type);
		virtual ~SymbolEdge(void);

		EdgeType						type()const{return m_type;}
		const QWeakPointer<SymbolNode>& srcNode()const{return m_srcNode;}
		const QWeakPointer<SymbolNode>& tarNode()const{return m_tarNode;}
		virtual	void					clear(){};

		virtual QString					toString()const;;

		float							strength()const{return m_strength;}
		float&							strength(){return m_strength;}

		template<typename T> QSharedPointer<T> getAttr()const
		{
			return qSharedPointerCast<T>(m_attrList[T::classType()]);
		}
		template<typename T> QSharedPointer<T> getOrAddAttr()
		{
			SymbolEdgeAttr::Ptr& attrPtr = m_attrList[T::classType()];
			if (!attrPtr)
			{
				attrPtr = QSharedPointer<T>(new T);
			}
			return qSharedPointerCast<T>(attrPtr);
		}
	protected:
	private:
		const EdgeType			 m_type;
		QWeakPointer<SymbolNode> m_srcNode;
		QWeakPointer<SymbolNode> m_tarNode;
		float					 m_strength;

		SymbolEdgeAttr::Ptr      m_attrList[N_SYMBOL_EDGE_ATTR];

	};
}