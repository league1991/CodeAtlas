#pragma once

namespace CodeAtlas
{
	namespace ClassViewConstants {

		const int IconSortOrder[] = {
			CPlusPlus::Icons::NamespaceIconType,
			CPlusPlus::Icons::EnumIconType,
			CPlusPlus::Icons::ClassIconType,
			CPlusPlus::Icons::FuncPublicIconType,
			CPlusPlus::Icons::FuncProtectedIconType,
			CPlusPlus::Icons::FuncPrivateIconType,
			CPlusPlus::Icons::SignalIconType,
			CPlusPlus::Icons::SlotPublicIconType,
			CPlusPlus::Icons::SlotProtectedIconType,
			CPlusPlus::Icons::SlotPrivateIconType,
			CPlusPlus::Icons::VarPublicIconType,
			CPlusPlus::Icons::VarProtectedIconType,
			CPlusPlus::Icons::VarPrivateIconType,
			CPlusPlus::Icons::EnumeratorIconType,
			CPlusPlus::Icons::KeywordIconType,
			CPlusPlus::Icons::MacroIconType,
			CPlusPlus::Icons::UnknownIconType
		};
		//! QStandardItem roles
		enum ItemRole
		{
			SymbolLocationsRole = Qt::UserRole + 1, //!< Symbol locations
			IconTypeRole,                           //!< Icon type (integer)
			SymbolNameRole,                         //!< Symbol name
			SymbolTypeRole                          //!< Symbol type
		};

	} // namespace ClassViewConstants


	class AtlasUtils
	{
		//! Private constructor
		AtlasUtils();
	public:

		static QList<QVariant> locationsToRole(const QSet<SymbolLocation> &locations);

		static QSet<SymbolLocation> roleToLocations(const QList<QVariant> &locations);

		static int iconTypeSortOrder(int iconType);

		static SymbolInfo symbolInformationFromItem(const QStandardItem *item);

		static QStandardItem *setSymbolInformationToItem(const SymbolInfo &information,
			QStandardItem *item);

		static void fetchItemToTarget(QStandardItem *item, const QStandardItem *target);

		static void moveItemToTarget(QStandardItem *item, const QStandardItem *target);

		template<typename Key, typename T>
		static QMap<Key, T> hash2Map(const QHash<Key, T>& hash)
		{
			QMap<Key, T> m;
			for (QHash<Key, T>::ConstIterator pV = hash.constBegin(); pV != hash.constEnd(); ++pV)
			{
				m[pV.key()] = pV.value();
			}
			return m;
		}
	};

	class SymbolPath;
	class SymbolTree;
	class DepthIterator;
	class SmartDepthIterator;
	class SymbolEdge;
	class SymbolNodeAttr;
	class SymEdgeIter;
	class SymbolNode
	{
#define ORDERED_PRINT
		friend class SymbolTree;
		friend class SymbolEdgeIter;
		typedef QHash<SymbolNodeAttr::AttrType, SymbolNodeAttr::Ptr>		AttrListMap;
		typedef QMultiHash<SymbolEdge::EdgeType,  SymbolEdge::Ptr>			EdgeListMap;
	public:
		typedef QSharedPointer<SymbolNode>									Ptr;
		typedef QWeakPointer<SymbolNode>									WeakPtr;
		typedef QSharedPointer<const SymbolNode>							ConstPtr;

		typedef QMap<SymbolInfo, SymbolNode::Ptr>							ChildMap;
		typedef ChildMap::Iterator											ChildIterator;
		typedef ChildMap::ConstIterator										ConstChildIterator;

		typedef DepthIterator												DepthIterator;
		typedef SmartDepthIterator											SmartDepthIterator;

		typedef AttrListMap::Iterator										AttrIterator;
		typedef AttrListMap::ConstIterator									ConstAttrIterator;

		typedef EdgeListMap::Iterator										EdgeIterator;
		typedef EdgeListMap::ConstIterator									ConstEdgeIterator;

		SymbolNode(unsigned timeStamp = 0);
		~SymbolNode();

		// get parent ptr
		const WeakPtr&		getParent()const{return m_parent;}
		// get SymbolInfo of current node
		const SymbolInfo    getSymInfo()const;
		const SymbolInfo::ElementType getElementType()const;
		// get SymbolPath of current node by upstream look-up, expensive operation
		const SymbolPath	getSymPath()const;
		bool				isGlobalSymbol()const;

		static void			deepCopy(const SymbolNode::ConstPtr &from,const SymbolNode::Ptr& to);
		// make original root's child and the new root point to each other
		// the original root remain unchanged
		static void			replaceRoot(const SymbolNode::ConstPtr &originalRoot, const SymbolNode::Ptr& newRoot);

		// child management
		static void			appendChild(const SymbolNode::Ptr& parent, const SymbolNode::Ptr& child, const SymbolInfo& childInfo);
		static SymbolNode::Ptr addOrFindChild( const SymbolNode::Ptr& parent,const SymbolInfo &inf, unsigned timeStamp);
		void                removeChild(const SymbolInfo &inf);
		SymbolNode::Ptr		child(const SymbolInfo &inf) const;
		int                 childCount() const;
		ConstChildIterator  childConstBegin()const{return m_childList.constBegin();}
		ConstChildIterator  childConstEnd()const{return m_childList.constEnd();}
		ChildIterator       childBegin(){return m_childList.begin();}
		ChildIterator		childEnd(){return m_childList.end();}
		void				findChild(unsigned childTypeMask, QList<Ptr>& childList)const;		// find in this node's child list
		bool				isChildExist( unsigned childTypeMask) const;
		void				getChildList(QList<Ptr>& childList)const;

		// set operation
		// to = from +/- to;
		static void add     (const SymbolNode::ConstPtr &from, const SymbolNode::Ptr& to);
		static void subtract(const SymbolNode::ConstPtr &from, const SymbolNode::Ptr& to);		// not used now

		// not used functions
		bool canFetchMore(QStandardItem *item) const;
		void fetchMore(QStandardItem *item) const;
		void convertTo(QStandardItem *item, bool recursive = true) const;

		void printTree(	int ident, 
						unsigned nodeMask = SymbolInfo::All,
						unsigned attrMask = SymbolNodeAttr::ATTR_ALL , 
						unsigned edgeMask = SymbolEdge::EDGE_NONE, 
						unsigned edgeType = SymbolNode::EDGE_ALL ) const;
		// find item by path, whose first element is the child of this item, not this item itself
		// if path is empty, return null pointer
		SymbolNode::Ptr			 findItem(const SymbolPath& path);
		SymbolNode::ConstPtr	 findItem(const SymbolPath& path)const;
		bool                     isItemExist(SymbolPath& path)const;

		// time stamp
		unsigned                 timeStamp()const{return m_timeStamp;}
		void                     setTimeStamp(unsigned timeStamp){m_timeStamp = timeStamp;}

		// attribute management
		// retrieve attribute, return null pointer if attribute not exist
		template<typename T>	QSharedPointer<T>		getAttr()const
		{
			QHash<SymbolNodeAttr::AttrType, SymbolNodeAttr::Ptr>::ConstIterator pAttr = m_attrList.find(T::classType());
			if(pAttr != m_attrList.constEnd())
				return qSharedPointerCast<T>(pAttr.value());
			return QSharedPointer<T>();
		}

		// retrieve attribute, a one new attribute if not exist
		template<typename T>	QSharedPointer<T>		getOrAddAttr()
		{ 
			QHash<SymbolNodeAttr::AttrType, SymbolNodeAttr::Ptr>::Iterator pAttr = m_attrList.find(T::classType());
			if(pAttr != m_attrList.end() && !pAttr.value().isNull())
				return qSharedPointerCast<T>(pAttr.value());

			SymbolNodeAttr::Ptr newAttr(new T);
			m_attrList[T::classType()] = newAttr;
			return qSharedPointerCast<T>(newAttr);
		}

		template<typename T>	bool					isAttrExist()const
		{	return m_attrList.contains(T::classType());	}


		bool                     addOrReplaceAttr(SymbolNodeAttr::Ptr attr);
		AttrIterator			 attrBegin(){return m_attrList.begin();}
		AttrIterator			 attrEnd(){return m_attrList.end();}
		ConstAttrIterator        attrConstBegin()const{return m_attrList.constBegin();}
		ConstAttrIterator		 attrConstEnd()const{return m_attrList.constEnd();}

		// edge
		enum EdgeDirType {
			EDGE_IN = 0x1,
			EDGE_OUT= (0x1)<<1,

			EDGE_NONE = 0,
			EDGE_ALL  = EDGE_IN | EDGE_OUT
		};
		int						 numInEdges(unsigned edgeTypeMask);
		int						 numOutEdges( unsigned edgeTypeMask );
		void				     inEdges(unsigned edgeTypeMask, QList<SymbolEdge::Ptr>& edgeList);
		void				     outEdges(unsigned edgeTypeMask, QList<SymbolEdge::Ptr>& edgeList);
		EdgeIterator			 inEdgeBegin(){return m_inEdgeList.begin();}
		EdgeIterator			 inEdgeEnd(){return m_inEdgeList.end();}
		ConstEdgeIterator		 inEdgeConstBegin()const{return m_inEdgeList.constBegin();}
		ConstEdgeIterator		 inEdgeConstEnd()const{return m_inEdgeList.constEnd();}
		EdgeIterator			 outEdgeBegin(){return m_outEdgeList.begin();}
		EdgeIterator			 outEdgeEnd(){return m_outEdgeList.end();}
		ConstEdgeIterator		 outEdgeConstBegin()const{return m_outEdgeList.constBegin();}
		ConstEdgeIterator		 outEdgeConstEnd()const{return m_outEdgeList.constEnd();}

	private:
		// internal data management
		SymbolNode &			 operator=(const SymbolNode &other);
		void                     copyNodeInfo(const SymbolNode& other);
		void                     addNodeInfo(const SymbolNode& other);
		void                     substractNodeInfo(const SymbolNode& other);
		void                     copyNodeAttributes(const SymbolNode& other);

		void					 deleteAllEdges();

		WeakPtr											m_parent;
		ChildMap										m_childList;		// child links, symbol information
		AttrListMap										m_attrList;			// attribute list
		EdgeListMap										m_inEdgeList;		// edge lists
		EdgeListMap										m_outEdgeList;
		unsigned										m_timeStamp;		// time stamp used for sync
		
};

	class SymbolTree
	{
#define TREE_USE_RECURSIVE_OUTPUT
	public:
		typedef SymbolNode::EdgeListMap  EdgeListMap;
		SymbolTree(){}
		SymbolTree(SymbolNode::Ptr root){m_root = root;}
		~SymbolTree(){}

		// find item by path, whose first element is the child of this item, not this item itself
		SymbolNode::ConstPtr findItem(SymbolPath& path)const
		{   return m_root->findItem(path);	}

		SymbolNode::Ptr		 getRoot()const{return m_root;}
		void				 setRoot( const SymbolNode::Ptr& root, unsigned* pLastHighestTimeStamp = NULL);
		const SymbolNode::ChildMap& getProjectList()const{return m_root->m_childList;}

		static private SymbolEdge::Ptr addEdge(const SymbolNode::Ptr& srcNode, const SymbolNode::Ptr& tarNode, SymbolEdge::EdgeType type)
		{
			SymbolEdge::Ptr edge(new SymbolEdge(type));
			srcNode->m_outEdgeList.insert(edge->type(), edge);
			tarNode->m_inEdgeList.insert(edge->type(), edge);

			edge->m_srcNode = srcNode.toWeakRef();
			edge->m_tarNode = tarNode.toWeakRef();
			return edge;
		}

		static SymbolEdge::Ptr getEdge(const SymbolNode::Ptr& srcNode, const SymbolNode::Ptr& tarNode, SymbolEdge::EdgeType type)
		{
			if (srcNode.isNull() || tarNode.isNull())
				return SymbolEdge::Ptr();

			// find edge in srcNode
			EdgeListMap::ConstIterator pEdge = srcNode->m_outEdgeList.find(type);
			while (pEdge != srcNode->m_outEdgeList.constEnd() && pEdge.value()->type() == type)
			{
				SymbolEdge::Ptr edge = pEdge.value();
				if (edge->tarNode() == tarNode.toWeakRef())
				{
					return edge;
				}
				++pEdge;
			}
			return SymbolEdge::Ptr();
		}

		static SymbolEdge::Ptr getOrAddEdge(const SymbolNode::Ptr& srcNode, const SymbolNode::Ptr& tarNode, SymbolEdge::EdgeType type)
		{
			if (srcNode.isNull() || tarNode.isNull())
				return SymbolEdge::Ptr();
			SymbolEdge::Ptr pEdge = getEdge(srcNode, tarNode, type);
			if (pEdge.isNull())
				pEdge = addEdge(srcNode, tarNode, type);
			return pEdge;
		}

		void print(	unsigned nodeMask = SymbolInfo::All, 
					unsigned attrMask = SymbolNodeAttr::ATTR_NONE,
					unsigned edgeMask = SymbolEdge::EDGE_NONE,
					unsigned edgeType = SymbolNode::EDGE_ALL)const;

		const QSet<SymbolNode::Ptr>& getDirtyProject()const{return m_dirtyProjList;}
		void addDirtyProject(const QSet<SymbolNode::Ptr>& dirtyList) {	m_dirtyProjList.unite(dirtyList);}
		void clearDirtyProject(){m_dirtyProjList.clear();}

		// find global classes/functions/enums/variable in a specified project, ignore namespaces
		static void getGlobalSymbols(const SymbolNode::Ptr& projNode, QList<SymbolNode::Ptr>& globalNodes);
		static void getNonProjChild(const SymbolNode::Ptr& projNode, QList<SymbolNode::Ptr>& childList);

		void		setTreeRect(const QRectF& rect){m_rect = rect;}
	private:
		void checkDirtyNodes( unsigned highestCleanStamp );
		QRectF					m_rect;
		SymbolNode::Ptr			m_root;
		QSet<SymbolNode::Ptr>	m_dirtyProjList;
	};
}