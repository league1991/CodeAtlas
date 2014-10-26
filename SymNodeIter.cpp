#include "StdAfx.h"
#include "SymNodeIter.h"
using namespace CodeAtlas;


bool DepthIterator::setRoot( const SymbolNode::Ptr& pRoot )
{
	if (pRoot.isNull() )
		return false;
	m_stack.clear();

	// make the stack top the first node
	// in pre-order  traversal, first node is the root
	// in post-order traversal, first node is the left-most child
	m_stack.push_back(StackEntry(pRoot, firstChildIter(pRoot)));
	if (m_traversalOrder == POSTORDER)
	{
		while(!isLeaf(m_stack.back().nodePtr()))
		{
			SymbolNode::Ptr pChild = m_stack.back().childIter().value();
			m_stack.push_back(StackEntry(pChild, firstChildIter(pChild)));
		}
	}
	return true;
}

SymbolNode::Ptr DepthIterator::operator++()
{
	if (m_stack.isEmpty())
		return SymbolNode::Ptr();

	// Note: the top of the stack always points to the current node
	if (m_traversalOrder == PREORDER)
	{
		// preorder traversal
		if (!isLeaf(m_stack.back().nodePtr()))
		{
			// has child, so go to the first child
			SymbolNode::Ptr pChild = m_stack.back().childIter().value();
			m_stack.push_back(StackEntry(pChild, firstChildIter(pChild)));
		}
		else
		{
			// reach leaf node
			// 1. find "youngest" parent that has some children remain not traversed
			// 2. go to one of those children
			do 
			{
				m_stack.pop_back();
				// now res is the last node, so the stack become empty
				if (m_stack.isEmpty())				
					return SymbolNode::Ptr();

				StackEntry& top = m_stack.back();
				advanceChildIter(top.nodePtr(), top.childIter()); 
			} while (m_stack.back().childIter() == m_stack.back().nodePtr()->childEnd());

			SymbolNode::Ptr pChild = m_stack.back().childIter().value();
			m_stack.push_back(StackEntry(pChild, firstChildIter(pChild)));
		}
		return m_stack.back().nodePtr();
	}
	else
	{
		// post order traversal
		// if already in leaf(which has been traversed), go to parent
		m_stack.pop_back();

		if (m_stack.isEmpty())				
			return SymbolNode::Ptr();			// now the last node has been traversed, so the stack become empty

		StackEntry& top = m_stack.back();
		advanceChildIter(top.nodePtr(), top.childIter()); 

		if (m_stack.back().childIter() == m_stack.back().nodePtr()->childEnd())
			return m_stack.back().nodePtr();	// reach a node that all it's children has been traversed, return that node

		// if reach a parent with some children not traversed, go to next leaf
		do{
			SymbolNode::Ptr pChild = m_stack.back().childIter().value();
			m_stack.push_back(StackEntry(pChild, firstChildIter(pChild)));
		}while(!isLeaf(m_stack.back().nodePtr()));

		return m_stack.back().nodePtr();
	}
}


DepthIterator::DepthIterator( const SymbolNode::Ptr& pRoot, TraversalOrder order /*= PREORDER*/ ) :m_traversalOrder(order)
{
	setRoot(pRoot);
}

SymbolNode::Ptr DepthIterator::operator*()
{
	if (m_stack.isEmpty())
		return SymbolNode::Ptr();
	return m_stack.back().nodePtr();
}

void DepthIterator::setTraversalOrder( TraversalOrder order )
{
	m_traversalOrder = order;
	if (!m_stack.isEmpty())
	{
		SymbolNode::Ptr root = m_stack[0].nodePtr();
		setRoot(root);
	}
}

DepthIterator::StackEntry::StackEntry(const SymbolNode::Ptr& pNode, const SymbolNode::ChildIterator& pChildIter)
{
	if (pNode.isNull())
		return;
	m_pNode  = pNode;
	m_pChild = pChildIter;
}


SymbolNode::Ptr CodeAtlas::SmartDepthIterator::operator++()
{
	while(1)
	{
		SymbolNode::Ptr pNode = DepthIterator::operator++();
		if (pNode.isNull())
			return pNode;
		SymbolInfo info = pNode->getSymInfo();
		if (info.elementType() & m_stepFilter)
			return pNode;
	}
}

SymbolNode::Ptr CodeAtlas::SmartDepthIterator::operator*()
{
	SymbolNode::Ptr res = DepthIterator::operator*();
	while(1)
	{
		if (res.isNull())
			return res;
		if (res->getSymInfo().elementType() & m_stepFilter)
			return res;
		res = operator++();
	}
}

CodeAtlas::SmartDepthIterator::SmartDepthIterator( const SymbolNode::Ptr& pRoot, 
												  TraversalOrder order /*= PREORDER*/, 
												  unsigned stepFilter /*= SymbolInformation::AllMask*/, 
												  unsigned subtreeFilter /*= SymbolInformation::AllMask*/ ) :
DepthIterator(pRoot, order), m_stepFilter(stepFilter), m_subtreeFilter(subtreeFilter)
{

}

bool CodeAtlas::SmartDepthIterator::isLeaf( const SymbolNode::Ptr& ptr )const
{
	if (m_subtreeFilter == SymbolInfo::All)
		return ptr->childCount() == 0;
	for (SymbolNode::ChildIterator pChild = ptr->childBegin();
		pChild != ptr->childEnd(); ++pChild)
	{
		if (pChild.key().elementType() & m_subtreeFilter)
			return false;
	}
	return true;
}

void CodeAtlas::SmartDepthIterator::advanceChildIter( const SymbolNode::Ptr& node, SymbolNode::ChildIterator& iter )
{
	if (m_subtreeFilter == SymbolInfo::All)
	{
		iter++;
		return;
	}
	do {
		++iter;
	} while (iter != node->childEnd() && !(iter.key().elementType() & m_subtreeFilter));
}

SymbolNode::ChildIterator CodeAtlas::SmartDepthIterator::firstChildIter( const SymbolNode::Ptr& ptr )
{
	if (m_subtreeFilter == SymbolInfo::All)
		return ptr->childBegin();

	SymbolNode::ChildIterator pChild;
	for (pChild = ptr->childBegin(); pChild != ptr->childEnd(); ++pChild)
	{
		if (pChild.key().elementType() & m_subtreeFilter)
			break;
	}
	return pChild;
}

bool CodeAtlas::DepthIterator::isCurNodeLeaf() const
{
	if (m_stack.size() == 0)
		return false;
	return isLeaf(m_stack.back().nodePtr());
}
