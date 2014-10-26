#pragma once

namespace CodeAtlas
{
	class DepthIterator
	{
	public:
		enum TraversalOrder{PREORDER, POSTORDER};
		DepthIterator(TraversalOrder order = PREORDER):m_traversalOrder(order){}
		DepthIterator(const SymbolNode::Ptr& pRoot, TraversalOrder order = PREORDER);

		void                setTraversalOrder(TraversalOrder order);

		bool				setRoot(const SymbolNode::Ptr& pRoot);
		// advance the pointer and return new node, return null pointer when the end is reached
		SymbolNode::Ptr		operator++();
		// get current node, return null pointer when the end is reached
		SymbolNode::Ptr     operator*();
		inline int          currentTreeLevel()const{return m_stack.size();}	// root node's level is 1

		bool				isCurNodeLeaf()const;
	protected:
		virtual bool isLeaf(const SymbolNode::Ptr& ptr)const
		{return ptr->childCount() == 0;}
		virtual void advanceChildIter(const SymbolNode::Ptr& node, SymbolNode::ChildIterator& iter)	
		{iter++;}
		virtual SymbolNode::ChildIterator firstChildIter(const SymbolNode::Ptr& ptr)
		{return ptr->childBegin();}
	private:
		class StackEntry
		{
		public:
			StackEntry(const SymbolNode::Ptr& pNode, 
				       const SymbolNode::ChildIterator& pFirstChild);
			inline SymbolNode::Ptr&				nodePtr(){return m_pNode;}
			inline const SymbolNode::Ptr&		nodePtr()const{return m_pNode;}
			inline SymbolNode::ChildIterator&	childIter(){return m_pChild;}
		private:
			SymbolNode::Ptr										m_pNode;
			SymbolNode::ChildIterator							m_pChild;
		};
		QList<StackEntry>	m_stack;
		TraversalOrder		m_traversalOrder;
	};

	class SmartDepthIterator: public DepthIterator
	{
	public:
		// stepFilter:
		//    only nodes with type in stepFilter can be dereferenced
		// subtreeFilter:
		//    when a node's type is NOT in subtreeFilter
		//    its subtree(INCLUDING itself) will NOT be considered
		//    but root will always be considered
		SmartDepthIterator(
			const SymbolNode::Ptr& pRoot, 
			TraversalOrder order = PREORDER,
			unsigned stepFilter = SymbolInfo::All,
			unsigned subtreeFilter = SymbolInfo::All);

		SymbolNode::Ptr		operator++();
		SymbolNode::Ptr	    operator*();

	protected:
		virtual bool isLeaf(const SymbolNode::Ptr& ptr)const;
		virtual void advanceChildIter(const SymbolNode::Ptr& node, SymbolNode::ChildIterator& iter);
		virtual SymbolNode::ChildIterator firstChildIter(const SymbolNode::Ptr& ptr);
	private:
		unsigned    m_stepFilter;
		unsigned    m_subtreeFilter;
	};
}