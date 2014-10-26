#include "StdAfx.h"
#include "SymbolNode.h"


using namespace CodeAtlas;
using namespace ProjectExplorer;
using namespace CPlusPlus;


AtlasUtils::AtlasUtils()
{
}

/*!
Converts internal location container to QVariant compatible.
\a locations specifies a set of symbol locations.
Returns a list of variant locations that can be added to the data of an
item.
*/

QList<QVariant> AtlasUtils::locationsToRole(const QSet<SymbolLocation> &locations)
{
	QList<QVariant> locationsVar;
	//   foreach (const SymbolLocation &loc, locations)
	//        locationsVar.append(QVariant::fromValue(loc));

	return locationsVar;
}

/*!
Converts QVariant location container to internal.
\a locationsVar contains a list of variant locations from the data of an
item.
Returns a set of symbol locations.
*/

QSet<SymbolLocation> AtlasUtils::roleToLocations(const QList<QVariant> &locationsVar)
{
	QSet<SymbolLocation> locations;
	foreach (const QVariant &loc, locationsVar) {
		//    if (loc.canConvert<SymbolLocation>())
		//        locations.insert(loc.value<SymbolLocation>());
	}

	return locations;
}

/*!
Returns sort order value for the \a icon.
*/

int AtlasUtils::iconTypeSortOrder(int icon)
{
	static QHash<int, int> sortOrder;

	// initialization
	if (sortOrder.count() == 0) {
		for (unsigned i = 0 ;
			i < sizeof(ClassViewConstants::IconSortOrder) / sizeof(ClassViewConstants::IconSortOrder[0]) ; ++i)
			sortOrder.insert(ClassViewConstants::IconSortOrder[i], sortOrder.count());
	}

	// if it is missing - return the same value
	if (!sortOrder.contains(icon))
		return icon;

	return sortOrder[icon];
}

/*!
Sets symbol information specified by \a information to \a item.
\a information provides the name, type, and icon for the item.
Returns the filled item.
*/

QStandardItem *AtlasUtils::setSymbolInformationToItem(const SymbolInfo &information,
														  QStandardItem *item)
{
	Q_ASSERT(item);

	item->setData(information.name(), ClassViewConstants::SymbolNameRole);
	item->setData(information.type(), ClassViewConstants::SymbolTypeRole);
	item->setData(information.elementType(), ClassViewConstants::IconTypeRole);

	return item;
}

/*!
Returns symbol information for \a item.
*/

SymbolInfo AtlasUtils::symbolInformationFromItem(const QStandardItem *item)
{
	Q_ASSERT(item);

	if (!item)
		return SymbolInfo();

	const QString &name = item->data(ClassViewConstants::SymbolNameRole).toString();
	const QString &type = item->data(ClassViewConstants::SymbolTypeRole).toString();
	SymbolInfo::ElementType iconType = SymbolInfo::Unknown;

	QVariant var = item->data(ClassViewConstants::IconTypeRole);
	bool ok = false;
	int value;
	if (var.isValid())
		value = var.toInt(&ok);
	if (ok)
		iconType = SymbolInfo::ElementType(value);

	return SymbolInfo(name, type, iconType);
}

/*!
Updates \a item to \a target, so that it is sorted and can be fetched.
*/

void AtlasUtils::fetchItemToTarget(QStandardItem *item, const QStandardItem *target)
{
	if (!item || !target)
		return;

	int itemIndex = 0;
	int targetIndex = 0;
	int itemRows = item->rowCount();
	int targetRows = target->rowCount();

	while (itemIndex < itemRows && targetIndex < targetRows) {
		const QStandardItem *itemChild = item->child(itemIndex);
		const QStandardItem *targetChild = target->child(targetIndex);

		const SymbolInfo &itemInf = symbolInformationFromItem(itemChild);
		const SymbolInfo &targetInf = symbolInformationFromItem(targetChild);

		if (itemInf < targetInf) {
			++itemIndex;
		} else if (itemInf == targetInf) {
			++itemIndex;
			++targetIndex;
		} else {
			item->insertRow(itemIndex, targetChild->clone());
			++itemIndex;
			++itemRows;
			++targetIndex;
		}
	}

	// append
	while (targetIndex < targetRows) {
		item->appendRow(target->child(targetIndex)->clone());
		++targetIndex;
	}
}

/*!
Moves \a item to \a target (sorted).
*/
void AtlasUtils::moveItemToTarget(QStandardItem *item, const QStandardItem *target)
{
	if (!item || !target)
		return;

	int itemIndex = 0;
	int targetIndex = 0;
	int itemRows = item->rowCount();
	int targetRows = target->rowCount();

	while (itemIndex < itemRows && targetIndex < targetRows) {
		QStandardItem *itemChild = item->child(itemIndex);
		const QStandardItem *targetChild = target->child(targetIndex);

		const SymbolInfo &itemInf = AtlasUtils::symbolInformationFromItem(itemChild);
		const SymbolInfo &targetInf = AtlasUtils::symbolInformationFromItem(targetChild);

		if (itemInf < targetInf) {
			item->removeRow(itemIndex);
			--itemRows;
		} else if (itemInf == targetInf) {
			moveItemToTarget(itemChild, targetChild);
			++itemIndex;
			++targetIndex;
		} else {
			item->insertRow(itemIndex, targetChild->clone());
			moveItemToTarget(item->child(itemIndex), targetChild);
			++itemIndex;
			++itemRows;
			++targetIndex;
		}
	}

	// append
	while (targetIndex < targetRows) {
		item->appendRow(target->child(targetIndex)->clone());
		moveItemToTarget(item->child(itemIndex), target->child(targetIndex));
		++itemIndex;
		++itemRows;
		++targetIndex;
	}

	// remove end of item
	while (itemIndex < itemRows) {
		item->removeRow(itemIndex);
		--itemRows;
	}
}
/*!
\class SymbolLocation
\brief The SymbolLocation class stores information about symbol location
to know the exact location to open when the user clicks on a tree item.

This class might be used in QSet and QHash.
*/

SymbolLocation::SymbolLocation() :
m_line(0),
m_column(0),
m_hash(0)
{
}

SymbolLocation::SymbolLocation(QString file, int lineNumber, int columnNumber) :
m_fileName(file),
m_line(lineNumber),
m_column(columnNumber)
{
	if (m_column < 0)
		m_column = 0;

	// pre-computate hash value
	m_hash = qHash(qMakePair(m_fileName, qMakePair(m_line, m_column)));
}

SymbolInfo::SymbolInfo() :
m_elementType(Unknown),
m_hash(0)
{
}

SymbolInfo::SymbolInfo(const QString &valueName, const QString &valueType,
									 ElementType iconType) :
m_elementType(iconType),
m_name(valueName),
m_type(valueType)
{
	// calculate hash
	unsigned elementHash = elementTypeIdx(m_elementType);
	unsigned nameHash    = qHash(m_name);
	unsigned typeNameHash= qHash(m_type);

	// hash = [elementHash  | typeNameHash | nameHash ]
	//      = [   5         |      13      |    14    ]
	m_hash = ((elementHash & 0x1f) << 27) | ((typeNameHash & 0x1fff) << 14) | ((nameHash & 0x3fff));
//	m_hash = qHash(qMakePair(m_elementType, qMakePair(m_name, m_type)));
}

/*!
Returns an icon type sort order number. It is not pre-calculated, as it is
needed for converting to standard item only.
*/

int SymbolInfo::iconTypeSortOrder() const
{
	return AtlasUtils::iconTypeSortOrder(m_elementType);
}

bool SymbolInfo::operator<(const SymbolInfo &other) const
{
// 	if (m_hash != other.m_hash)
// 		return m_hash < other.m_hash;

	int thisType = elementTypeIdx(m_elementType);
	int otherType= elementTypeIdx(other.m_elementType);
	if (thisType != otherType)
		return thisType < otherType;

	int cmp = name().compare(other.name(), Qt::CaseInsensitive);
	if (cmp != 0)
		return cmp < 0;
	return type().compare(other.type(), Qt::CaseInsensitive) < 0;
}

int SymbolInfo::compare(const SymbolInfo &other) const
{
	// comparsion is not a critical for speed
	int thisType = elementTypeIdx(m_elementType);
	int otherType= elementTypeIdx(other.m_elementType);
	if (thisType != otherType)
		return thisType < otherType ? -1 : 1;

	int cmp = name().compare(other.name(), Qt::CaseInsensitive);
	if (cmp != 0)
		return cmp;
	return type().compare(other.type(), Qt::CaseInsensitive);
}




///////////////////////////////// SymbolTreeNode //////////////////////////////////

/*!
\class SymbolTreeNode
\brief The SymbolTreeNode class is an item for the internal Class View tree.

Not virtual - to speed up its work.
*/

SymbolNode::SymbolNode(unsigned timeStamp):m_timeStamp(timeStamp)
{ 
}

SymbolNode::~SymbolNode()
{
	deleteAllEdges();
}

SymbolNode &SymbolNode::operator=(const SymbolNode &other)
{
	m_childList.clear();
	copyNodeInfo(other);
	return *this;
}

void SymbolNode::addNodeInfo( const SymbolNode& other )
{
	unsigned t0 = m_timeStamp;
	unsigned t1 = other.m_timeStamp;
	m_timeStamp = t0 > t1 ? t0 : t1;

	for (AttrListMap::ConstIterator pAttr = other.m_attrList.constBegin(); pAttr != other.m_attrList.constEnd(); ++pAttr)
	{
		SymbolNodeAttr::AttrType t = pAttr.key();
		if (m_attrList.contains(t))
		{
			(m_attrList[t])->unite(*pAttr.value());
		}
		else
			m_attrList[t] = pAttr.value()->duplicate();
	}
}

void SymbolNode::substractNodeInfo( const SymbolNode& other )
{
	unsigned t0 = m_timeStamp;
	unsigned t1 = other.m_timeStamp;
	m_timeStamp = t0 > t1 ? t0 : t1;
	for (AttrListMap::ConstIterator pAttr = other.m_attrList.constBegin(); pAttr != other.m_attrList.constEnd(); ++pAttr)
	{
		SymbolNodeAttr::AttrType t = pAttr.key();
		if (m_attrList.contains(t))
			(m_attrList[t])->substract(*pAttr.value());
	}
}

void SymbolNode::copyNodeInfo( const SymbolNode& other )
{
	m_timeStamp       = other.m_timeStamp;
	m_attrList.clear();
	for (AttrListMap::ConstIterator pAttr = other.m_attrList.constBegin(); pAttr != other.m_attrList.constEnd(); ++pAttr)
	{
		m_attrList[pAttr.key()] = pAttr.value()->duplicate();
	}
}

/*!
Copies a parser tree item from the location specified by \a from to this
item.
*/

void SymbolNode::replaceRoot(const SymbolNode::ConstPtr &originalRoot, const SymbolNode::Ptr& newRoot)
{
	if (originalRoot.isNull() || newRoot.isNull())
		return;

	newRoot->m_childList.clear();
	newRoot->copyNodeInfo(*originalRoot);

	for(ConstChildIterator pChild = originalRoot->m_childList.constBegin();
		pChild != originalRoot->m_childList.constEnd(); ++pChild)
	{
		appendChild(newRoot, pChild.value(), pChild.key());
	}
}

/*!
\fn void copyTree(const ParserTreeItem::ConstPtr &from)
Copies a parser tree item with children from the location specified by
\a from to this item.
*/

void SymbolNode::deepCopy(const SymbolNode::ConstPtr &from, const SymbolNode::Ptr& to)
{
	if (from.isNull() || to.isNull())
		return;

	// copy content
	to->m_childList.clear();
	to->copyNodeInfo(*from);

	// reserve memory
	//    int amount = qMin(100 , target->d_ptr->symbolInformations.count() * 2);
	//    d_ptr->symbolInformations.reserve(amount);

	// every child
	ConstChildIterator cur =
		from->m_childList.constBegin();
	ConstChildIterator end =
		from->m_childList.constEnd();

	for (; cur != end; cur++) {
		SymbolNode::Ptr item(new SymbolNode(from->m_timeStamp));
		deepCopy(cur.value(), item);
		appendChild(to, item, cur.key());
	}
}


/*!
Appends the child item \a item to \a inf symbol information.
*/
/*
void SymbolTreeNode::appendChild(const SymbolTreeNode::Ptr &item, const SymbolInformation &inf)
{
	// removeChild must be used to remove an item
	if (item.isNull())
		return;

	symbolInformations[inf] = item;
}*/

/*!
Removes the \a inf symbol information.
*/

void SymbolNode::removeChild(const SymbolInfo &inf)
{
	ChildIterator pChild = m_childList.find(inf);
	if (pChild != m_childList.end())
	{
		pChild.value()->m_parent = WeakPtr();
		m_childList.remove(inf);
	}
}

/*!
Returns the child item specified by \a inf symbol information.
*/

SymbolNode::Ptr SymbolNode::child(const SymbolInfo &inf) const
{
	if (!m_childList.contains(inf))
		return SymbolNode::Ptr();
	return m_childList[inf];
}

SymbolNode::Ptr SymbolNode::addOrFindChild( const SymbolNode::Ptr& parent,const SymbolInfo &inf, unsigned timeStamp)
{
	if (!parent->m_childList.contains(inf))
		appendChild(parent, SymbolNode::Ptr(new SymbolNode(timeStamp)), inf);
	parent->m_timeStamp = qMax(parent->m_timeStamp, timeStamp);
	return parent->m_childList[inf];
}


/*!
Returns the amount of children of the tree item.
*/

int SymbolNode::childCount() const
{
	return m_childList.count();
}

/*!
\property QIcon::icon
\brief the icon assigned to the tree item
*/

// QIcon SymbolTreeNode::icon() const
// {
// 	return icon;
// }
// 
// /*!
// Sets the \a icon for the tree item.
// */
// void SymbolTreeNode::setIcon(const QIcon &icon)
// {
// 	icon = icon;
// }

/*!
Adds an internal state with \a target, which contains the correct current
state.
*/

void SymbolNode::add(const SymbolNode::ConstPtr &from, const SymbolNode::Ptr& to)
{
	if (from.isNull())
		return;

	// add locations
	to->addNodeInfo(*from);

	// add children
	// every from child
	ConstChildIterator cur =	from->m_childList.constBegin();
	ConstChildIterator end =	from->m_childList.constEnd();

	while (cur != end) 
	{
		const SymbolInfo &inf = cur.key();
		const SymbolNode::Ptr &fromChild = cur.value();
		if (to->m_childList.contains(inf)) 
		{
			// this item has the same child node
			const SymbolNode::Ptr &childItem = to->m_childList[inf];
			if (!childItem.isNull())
				SymbolNode::add(fromChild, childItem);
			else 
			{
				SymbolNode::Ptr newChild(new SymbolNode(fromChild->m_timeStamp));
				deepCopy(fromChild, newChild);
				appendChild(to, newChild, inf);
			}
		} 
		else 
		{ 
			SymbolNode::Ptr newItem(new SymbolNode(fromChild->m_timeStamp));
			deepCopy(fromChild, newItem);
			appendChild(to, newItem, inf);
		}
		// next item
		++cur;
	}
}

/*!
Subtracts an internal state with \a target, which contains the subtrahend.
*/

void SymbolNode::subtract(const SymbolNode::ConstPtr &from, const SymbolNode::Ptr& to)
{
	if (from.isNull())
		return;

	to->substractNodeInfo(*from);

	// every from child
	ConstChildIterator cur =	from->m_childList.constBegin();
	ConstChildIterator end =	from->m_childList.constEnd();

	while (cur != end) {
		const SymbolInfo &inf = cur.key();
		if (to->m_childList.contains(inf)) 
		{
			// this item has the same child node
			if (!to->m_childList[inf].isNull())
				SymbolNode::subtract(cur.value(), to->m_childList[inf]);
			if (to->m_childList[inf].isNull() || to->m_childList[inf]->childCount() == 0)
				to->m_childList.remove(inf);
		}
		// next item
		++cur;
	}
}

/*!
Appends this item to the QStandardIten item \a item.
\a recursive does it recursively for the tree items (might be needed for
lazy data population.
*/

void SymbolNode::convertTo(QStandardItem *item, bool recursive) const
{
	if (!item)
		return;

	QMap<SymbolInfo, SymbolNode::Ptr> map;

	qDebug("---");
	// convert to map - to sort it
	ConstChildIterator curHash =
		m_childList.constBegin();
	ConstChildIterator endHash =
		m_childList.constEnd();
	while (curHash != endHash) {
		qDebug() << curHash.key().name() << curHash.key().type();
		map.insert(curHash.key(), curHash.value());
		++curHash;
	}

	// add to item
	QMap<SymbolInfo, SymbolNode::Ptr>::const_iterator cur = map.constBegin();
	QMap<SymbolInfo, SymbolNode::Ptr>::const_iterator end = map.constEnd();
	while (cur != end) {
		const SymbolInfo &inf = cur.key();
		SymbolNode::Ptr ptr = cur.value();

		QStandardItem *add = new QStandardItem();
		AtlasUtils::setSymbolInformationToItem(inf, add);
		if (!ptr.isNull()) {
			// icon
			//add->setIcon(ptr->icon());

			// locations
			//add->setData(AtlasUtils::locationsToRole(ptr->symbolLocations),
			//	ClassViewConstants::SymbolLocationsRole);

			if (recursive)
				cur.value()->convertTo(add, true);
		}
		item->appendRow(add);
		++cur;
	}
}

/*!
Checks \a item in a QStandardItemModel for lazy data population.
*/

bool SymbolNode::canFetchMore(QStandardItem *item) const
{
	if (!item)
		return false;

	// incremental data population - so we have to check children
	// count subchildren of both - current QStandardItem and our internal

	// for the current UI item
	int storedChildren = 0;
	for (int i = 0; i < item->rowCount(); i++) {
		QStandardItem *child = item->child(i);
		if (!child)
			continue;
		storedChildren += child->rowCount();
	}
	// children for the internal state
	int internalChildren = 0;
	ConstChildIterator curHash =
		m_childList.constBegin();
	ConstChildIterator endHash =
		m_childList.constEnd();
	while (curHash != endHash) {
		const SymbolNode::Ptr &child = curHash.value();
		if (!child.isNull()) {
			internalChildren += child->childCount();
			// if there is already more items than stored, then can be stopped right now
			if (internalChildren > storedChildren)
				break;
		}
		++curHash;
	}

	if (storedChildren < internalChildren)
		return true;

	return false;
}

/*!
Performs lazy data population for \a item in a QStandardItemModel if needed.
*/

void SymbolNode::fetchMore(QStandardItem *item) const
{
	if (!item)
		return;

	for (int i = 0; i < item->rowCount(); i++) {
		QStandardItem *child = item->child(i);
		if (!child)
			continue;

		const SymbolInfo &childInf = AtlasUtils::symbolInformationFromItem(child);

		if (m_childList.contains(childInf)) {
			const SymbolNode::Ptr &childPtr = m_childList[childInf];
			if (childPtr.isNull())
				continue;

			// create a standard
			QScopedPointer<QStandardItem> state(new QStandardItem());
			childPtr->convertTo(state.data(), false);

			AtlasUtils::fetchItemToTarget(child, state.data());
		}
	}
}

/*!
Debug dump.
*/

void SymbolNode::printTree(int ident, 
						   unsigned nodeMask /*= SymbolInformation::All*/, 
						   unsigned attrMask /*= SymNodeAttr::ATTR_ALL*/ ,
						   unsigned edgeMask /*= SymbolEdge::EDGE_NONE*/,
						   unsigned edgeType /*= SymbolNode::EDGE_ALL*/) const
{
	QString identStr = "\t" + QString(1*(ident), QLatin1Char('\t'));
	QString fillStr  = "\t" + QString(5*(ident), QLatin1Char('-'));

	const SymbolNode* node = this;
	SymbolInfo info = node->getSymInfo();
	if (info.elementType() & nodeMask)
	{
		QString hashStr = QString::number(info.hash());
		qDebug() << node->timeStamp() 
				 << qPrintable(fillStr + info.elementTypeName() + " " + info.name() + ":" + info.type() + " " + hashStr);

		for (SymbolNode::ConstAttrIterator pAttr = node->attrConstBegin(); 
			pAttr != node->attrConstEnd(); ++pAttr)
		{
			SymbolNodeAttr::Ptr attr = pAttr.value();
			if (!attr.isNull() &&(attr->type() & attrMask))
			{
				QString titleStr = identStr + "### " + QString::number(pAttr.key()) + "\n";
				QString attrStr  = attr->toString();
				attrStr = identStr + attrStr.replace("\n", "\n"+identStr);

				qDebug() << qPrintable(titleStr) << qPrintable(attrStr);
			}
		}

		if (edgeType & SymbolNode::EDGE_IN)
		{
			for (SymbolNode::ConstEdgeIterator pEdge = node->inEdgeConstBegin(); 
				pEdge != node->inEdgeConstEnd(); ++pEdge)
			{
				SymbolEdge::Ptr edge = pEdge.value();
				if (!edge.isNull() &&(edge->type() & edgeMask))
				{
					QString titleStr = identStr + "### inEdge ### " + QString::number(pEdge.key()) + "\n";
					QString edgeStr  = edge->toString();
					edgeStr = identStr + edgeStr.replace("\n", "\n"+identStr);

					qDebug() << qPrintable(titleStr) << qPrintable(edgeStr);
				}
			}
		}


		if (edgeType & SymbolNode::EDGE_OUT)
		{
			for (SymbolNode::ConstEdgeIterator pEdge = node->outEdgeConstBegin(); 
				pEdge != node->outEdgeConstEnd(); ++pEdge)
			{
				SymbolEdge::Ptr edge = pEdge.value();
				if (!edge.isNull() &&(edge->type() & edgeMask))
				{
					QString titleStr = identStr + "### outEdge ### " + QString::number(pEdge.key()) + "\n";
					QString edgeStr  = edge->toString();
					edgeStr = identStr + edgeStr.replace("\n", "\n"+identStr);

					qDebug() << qPrintable(titleStr) << qPrintable(edgeStr);
				}
			}
		}

	}

#ifdef ORDERED_PRINT
	//QMap<SymbolInfo, SymbolNode::Ptr> childMap = AtlasUtils::hash2Map(m_childList);
	const QMap<SymbolInfo, SymbolNode::Ptr>& childMap = m_childList;
	for (QMap<SymbolInfo, SymbolNode::Ptr>::ConstIterator pV = childMap.constBegin(); pV != childMap.constEnd(); ++pV)
#else
	for (QHash<SymbolInfo, SymbolNode::Ptr>::ConstIterator pV = m_childList.constBegin(); pV != m_childList.constEnd(); ++pV)
#endif
	{
		(pV.value())->printTree(ident+1, nodeMask, attrMask, edgeMask, edgeType);
	}
}


SymbolNode::Ptr SymbolNode::findItem(const SymbolPath& path )
{
	SymbolNode* item = this;
	SymbolNode::Ptr ptr;
	for (int ithPathElement = 0; ithPathElement < path.getSymbolCount(); ++ithPathElement)
	{
		const SymbolInfo& symInfo = *path.getSymbol(ithPathElement);
		if (!item || !item->m_childList.contains(symInfo))
			return SymbolNode::Ptr();

		ptr = item->m_childList[symInfo];
		item = ptr.data();
	}
	return ptr;
}

SymbolNode::ConstPtr SymbolNode::findItem(const SymbolPath& path ) const
{
	const SymbolNode* item = this;
	SymbolNode::ConstPtr ptr;
	for (int ithPathElement = 0; ithPathElement < path.getSymbolCount(); ++ithPathElement)
	{
		const SymbolInfo& symInfo = *path.getSymbol(ithPathElement);
		if (!item || !item->m_childList.contains(symInfo))
			return SymbolNode::ConstPtr();

		ptr = item->m_childList[symInfo];
		item = ptr.data();
	}
	return ptr;
}



bool SymbolNode::isItemExist( SymbolPath& path )const
{
	return findItem(path).data() != NULL;
}




QString SymbolInfo::toString( bool showType /*= false*/, bool showElementType /*= false*/)const
{
	QString s = m_name;
	if (showType)
		s += (":" + m_type);
	if (showElementType)
		s += (":" + elementTypeName());
	return s;
}

SymbolInfo::ElementType SymbolInfo::elementTypeFromSymbol( const CPlusPlus::Symbol *symbol )
{
	if (const Template *templ = symbol->asTemplate()) {
		if (Symbol *decl = templ->declaration())
			return elementTypeFromSymbol(decl);
	}

	FullySpecifiedType symbolType = symbol->type();
	if (symbol->isFunction() || (symbol->isDeclaration() && symbolType &&
		symbolType->isFunctionType()))
	{
		const CPlusPlus::Function *func = symbol->asFunction();
		if (!func)
			func = symbol->type()->asFunctionType();

		if (func->isSlot() ) {
			if (func->isPublic())
				return SymbolInfo::SlotPublic;
			else if (func->isProtected())
				return SymbolInfo::SlotProtected;
			else if (func->isPrivate())
				return SymbolInfo::SlotPrivate;
		} else if (func->isSignal()) {
			return SymbolInfo::Signal;
		} else if (symbol->isPublic()) {
			return SymbolInfo::FuncPublic;
		} else if (symbol->isProtected()) {
			return SymbolInfo::FuncProtected;
		} else if (symbol->isPrivate()) {
			return SymbolInfo::FuncPrivate;
		}
	} else if (symbol->enclosingScope() && symbol->enclosingScope()->isEnum()) {
		return SymbolInfo::Enumerator;
	} else if (symbol->isDeclaration() || symbol->isArgument()) {
		if (symbol->isPublic())
			return SymbolInfo::VarPublic;
		else if (symbol->isProtected())
			return SymbolInfo::VarProtected;
		else if (symbol->isPrivate())
			return SymbolInfo::VarPrivate;
	} else if (symbol->isEnum()) {
		return SymbolInfo::Enum;
	} else if (symbol->isClass() || symbol->isForwardClassDeclaration()) {
		return SymbolInfo::Class;
	} else if (symbol->isObjCClass() || symbol->isObjCForwardClassDeclaration()) {
		return SymbolInfo::Class;
	} else if (symbol->isObjCProtocol() || symbol->isObjCForwardProtocolDeclaration()) {
		return SymbolInfo::Class;
	} else if (symbol->isObjCMethod()) {
		return SymbolInfo::FuncPublic;
	} else if (symbol->isNamespace()) {
		return SymbolInfo::Namespace;
	} else if (symbol->isTypenameArgument()) {
		return SymbolInfo::Class;
	} else if (symbol->isUsingNamespaceDirective() ||
		symbol->isUsingDeclaration()) {
			// TODO: Might be nice to have a different icons for these things
			return SymbolInfo::Namespace;
	} else if (symbol->isBlock()){
		return SymbolInfo::Block;
	}

	return SymbolInfo::Unknown;
}

bool SymbolInfo::operator==( const SymbolInfo &other ) const
{
	return elementType() == other.elementType() && name() == other.name()
		&& type() == other.type();
}


const   Overview& SymbolInfo::getNamePrinter()
{
	if (!m_namePrinter)
	{
		m_namePrinter = new Overview;
		m_namePrinter->showArgumentNames = true;
		m_namePrinter->showDefaultArguments = true;
		m_namePrinter->showFunctionSignatures = true;
		m_namePrinter->showReturnTypes = true;
		m_namePrinter->showTemplateParameters = true; 
	}
	return *m_namePrinter;
}

void SymbolInfo::getTypeElements( QStringList& typeList )const
{
	typeList = m_type.split(m_typeElementSplitter, QString::SkipEmptyParts);
}

bool SymbolInfo::isBasicTypeWord( const QString& type )
{
	if (m_basicTypeWords.size() == 0)
		initBasicTypeWordSet();
	return m_basicTypeWords.contains(type);
}

void SymbolInfo::initBasicTypeWordSet()
{
	if (m_basicTypeWords.size() != 0)
		return;
	QSet<QString>& typeSet = m_basicTypeWords;
	typeSet << "bool" << "char" << "wchar_t" << "short" << "int" << "long" << "float" << "double"
		<< "void" << "const" << "static";
}

void SymbolInfo::getNonBasicTypeElements( QStringList& typeList ) const
{
	QStringList tmpList = m_type.split(m_typeElementSplitter, QString::SkipEmptyParts);
	for (int ithType = 0; ithType < tmpList.size(); ++ithType)
	{
		if (!isBasicTypeWord(tmpList[ithType]))
			typeList.push_back(tmpList[ithType]);
	}
}

bool SymbolInfo::matchNameOrType( const QString& str, MatchCase matchCase )const
{
	bool res = false;
	if (matchCase & MatchName)
		res |= (m_name == str);
	if (matchCase & MatchType)
		res |= (m_type == str);
	return res;
}



bool SymbolNode::addOrReplaceAttr( SymbolNodeAttr::Ptr attr )
{
	if (!attr)
		return false;
	m_attrList[attr->type()] = attr;
	return true;
}

void SymbolNode::copyNodeAttributes( const SymbolNode& other )
{
	m_attrList.clear();
	for (QHash<SymbolNodeAttr::AttrType, SymbolNodeAttr::Ptr>::ConstIterator pIter = other.m_attrList.constBegin();
		pIter != other.m_attrList.constEnd(); ++pIter)
	{
		SymbolNodeAttr::Ptr pValue = pIter.value()->duplicate();
		m_attrList[pIter.key()] = pValue;
	}
}

void SymbolNode::appendChild( const SymbolNode::Ptr& parent, const SymbolNode::Ptr& child, const SymbolInfo& childInfo )
{
	if (parent.isNull() || child.isNull())
		return;

	parent->m_childList[childInfo] = child;
	child->m_parent = parent;
}

const SymbolInfo SymbolNode::getSymInfo() const
{
	// root node
	if (m_parent.isNull())
		return SymbolInfo("root","root", SymbolInfo::Root);
	Ptr pParent = m_parent.toStrongRef();
	for (ConstChildIterator pChild = pParent->childConstBegin();
		pChild != pParent->childConstEnd(); ++pChild)
	{
		if (pChild.value().data() == this)
		{
			return pChild.key();
		}
	}
	return SymbolInfo("","", SymbolInfo::Unknown);
}

void SymbolNode::outEdges( unsigned edgeTypeMask, QList<SymbolEdge::Ptr>& edgeList )
{
	edgeList.clear();
	for (EdgeListMap::ConstIterator pEdge = m_outEdgeList.constBegin(); pEdge != m_outEdgeList.constEnd(); ++pEdge)
	{
		if (pEdge.value()->type() & edgeTypeMask)
			edgeList.push_back(pEdge.value());
	}
}

void SymbolNode::inEdges( unsigned edgeTypeMask, QList<SymbolEdge::Ptr>& edgeList )
{
	edgeList.clear();
	for (EdgeListMap::ConstIterator pEdge = m_inEdgeList.constBegin(); pEdge != m_inEdgeList.constEnd(); ++pEdge)
	{
		if (pEdge.value()->type() & edgeTypeMask)
			edgeList.push_back(pEdge.value());
	}
}

void SymbolNode::deleteAllEdges()
{
	// erase edge from its src node's outEdgeList
	int nRemove = 0;
	for (EdgeListMap::ConstIterator pInEdge = m_inEdgeList.constBegin(); pInEdge != m_inEdgeList.constEnd(); ++pInEdge)
	{
		SymbolEdge::Ptr edge = pInEdge.value();
		const SymbolNode::WeakPtr& pSrcNode = edge->srcNode();
		if (pSrcNode)
			nRemove = pSrcNode.toStrongRef()->m_outEdgeList.remove(edge->type(), edge);
		if (nRemove == 0)
			qDebug("failed to remove");
	}

	// erase edge from its tar node's inEdgeList
	for (EdgeListMap::ConstIterator pOutEdge = m_outEdgeList.constBegin(); pOutEdge != m_outEdgeList.constEnd(); ++pOutEdge)
	{
		SymbolEdge::Ptr edge = pOutEdge.value();
		const SymbolNode::WeakPtr& pTarNode = edge->tarNode();
		if (pTarNode)
			nRemove = pTarNode.toStrongRef()->m_inEdgeList.remove(edge->type(), edge);
		if (nRemove == 0)
			qDebug("failed to remove");
	}

	// clear both lists, then edges will be automatically deleted
	//SymbolEdge::m_nInstance -= m_inEdgeList.size() + m_outEdgeList.size();
	int totRemove = m_inEdgeList.size() + m_outEdgeList.size();
	if (totRemove != 0)
	{
		qDebug("\tnode remove %d edges",totRemove);
	}
	m_inEdgeList.clear();
	m_outEdgeList.clear();
}

const SymbolPath SymbolNode::getSymPath() const
{
	SymbolPath path;
	const SymbolNode* curNode = this;
	for (WeakPtr wpParent = m_parent; !wpParent.isNull();)
	{
		Ptr pParent = wpParent.toStrongRef();
		for (ConstChildIterator pChild = pParent->childConstBegin();
			pChild != pParent->childConstEnd(); ++pChild)
		{
			if (pChild.value().data() == curNode)
			{
				path.addParentSymbol(pChild.key());
				break;
			}
		}
		wpParent = pParent->m_parent;
	}
	return path;
}

void SymbolNode::findChild( unsigned childTypeMask, QList<Ptr>& childList ) const
{
	childList.clear();
	for (ConstChildIterator pChild = childConstBegin();
		pChild != childConstEnd(); ++pChild)
	{
		if (pChild.key().elementType() & childTypeMask)
			childList.push_back(pChild.value());
	}
}

bool SymbolNode::isChildExist( unsigned childTypeMask) const
{
	for (ConstChildIterator pChild = childConstBegin();
		pChild != childConstEnd(); ++pChild)
	{
		if (pChild.key().elementType() & childTypeMask)
			return true;
	}
	return false;
}

bool SymbolNode::isGlobalSymbol() const
{
	SymbolInfo info = getSymInfo();
	SymbolInfo::ElementType type = info.elementType();
	unsigned impossibleType = SymbolInfo::Block | SymbolInfo::Root |SymbolInfo::Namespace | SymbolInfo::NonpublicMember| SymbolInfo::Project;
	if (type == SymbolInfo::Unknown || (type & impossibleType))
		return false;

	for(SymbolNode::WeakPtr wpParent = m_parent; !wpParent.isNull(); )
	{
		SymbolNode::Ptr pParent = wpParent.toStrongRef();
		SymbolInfo pntinfo = pParent->getSymInfo();
		SymbolInfo::ElementType pntType = pntinfo.elementType();

		if (pntType & SymbolInfo::Project)
			return true;

		if ((pntType == SymbolInfo::Unknown) || 
			(pntType & (SymbolInfo::All & ~(SymbolInfo::Block | SymbolInfo::Namespace))))
			return false;

		wpParent = pParent->m_parent;
	}
	return false;
}

void SymbolNode::getChildList( QList<Ptr>& childList ) const
{
	childList.clear();
	for (ConstChildIterator pChild = childConstBegin(); pChild != childConstEnd(); ++pChild)
		childList.push_back(pChild.value());
}


void SymbolTree::print(			  unsigned nodeMask /*= SymbolInformation::All*/, 
								  unsigned attrMask /*= SymNodeAttr::ATTR_ALL*/ ,
								  unsigned edgeMask /*= SymbolEdge::EDGE_NONE*/,
								  unsigned edgeType /*= SymbolNode::EDGE_ALL*/)const
{
	if (!m_root)
		return;
	qDebug() << "\n\n\n\n";
#ifdef TREE_USE_RECURSIVE_OUTPUT
	m_root->printTree(0, nodeMask | SymbolInfo::Root, attrMask, edgeMask, edgeType);
#else
	SymbolNode::SmartDepthIterator pIter(
		m_root,
		SymbolNode::SmartDepthIterator::PREORDER,
		nodeMask);
	for(SymbolNode::Ptr node; node = *pIter; ++pIter)
	{
		SymbolInfo info = node->getSymInfo();
		QString identStr = "\t" + QString(1*(pIter.currentTreeLevel()-1), QLatin1Char('\t'));
		QString fillStr  = "\t" + QString(5*(pIter.currentTreeLevel()-1), QLatin1Char('-'));
		qDebug() << node->timeStamp() << qPrintable(fillStr  + info.elementTypeName() + " " + info.name());

		for (SymbolNode::ConstAttrIterator pAttr = node->attrConstBegin(); 
			pAttr != node->attrConstEnd(); ++pAttr)
		{
			SymbolNodeAttr::Ptr attr = pAttr.value();
			if (!attr.isNull() &&(attr->type() & attrMask))
			{
				QString titleStr = identStr + "### " + QString::number(pAttr.key()) + "\n";
				QString attrStr  = attr->toString();
				attrStr = identStr + attrStr.replace("\n", "\n"+identStr);

				qDebug() << qPrintable(titleStr) << qPrintable(attrStr);
			}
		}

		if (edgeType & SymbolNode::EDGE_IN)
		{
			for (SymbolNode::ConstEdgeIterator pEdge = node->inEdgeConstBegin(); 
				pEdge != node->inEdgeConstEnd(); ++pEdge)
			{
				SymbolEdge::Ptr edge = pEdge.value();
				if (!edge.isNull() &&(edge->type() & edgeMask))
				{
					QString titleStr = identStr + "### inEdge ### " + QString::number(pEdge.key()) + "\n";
					QString edgeStr  = edge->toString();
					edgeStr = identStr + edgeStr.replace("\n", "\n"+identStr);

					qDebug() << qPrintable(titleStr) << qPrintable(edgeStr);
				}
			}
		}
		if (edgeType & SymbolNode::EDGE_OUT)
		{
			for (SymbolNode::ConstEdgeIterator pEdge = node->outEdgeConstBegin(); 
				pEdge != node->outEdgeConstEnd(); ++pEdge)
			{
				SymbolEdge::Ptr edge = pEdge.value();
				if (!edge.isNull() &&(edge->type() & edgeMask))
				{
					QString titleStr = identStr + "### outEdge ### " + QString::number(pEdge.key()) + "\n";
					QString edgeStr  = edge->toString();
					edgeStr = identStr + edgeStr.replace("\n", "\n"+identStr);

					qDebug() << qPrintable(titleStr) << qPrintable(edgeStr);
				}
			}
		}
	}
#endif
}

void SymbolTree::setRoot( const SymbolNode::Ptr& root, unsigned* pLastHighestTimeStamp)
{
	unsigned lastHighestTimeStamp = 0;
	if (pLastHighestTimeStamp)
		lastHighestTimeStamp = *pLastHighestTimeStamp;
	else if (!m_root.isNull())
		lastHighestTimeStamp = m_root->timeStamp();
	m_root = root;
	clearDirtyProject();
	checkDirtyNodes(lastHighestTimeStamp);
}

void SymbolTree::checkDirtyNodes( unsigned highestCleanStamp )
{
	m_dirtyProjList.clear();

	SymbolNode::Ptr root = m_root;
	SymbolNode::SmartDepthIterator pProj(root, 
		SmartDepthIterator::PREORDER, 
		SymbolInfo::Project,
		SymbolInfo::Project);

	for (SymbolNode::Ptr proj; proj = *pProj; ++pProj)
	{
		if (proj->timeStamp() > highestCleanStamp)
			m_dirtyProjList.insert(proj);
	}
}

void SymbolTree::getGlobalSymbols( const SymbolNode::Ptr& projNode, QList<SymbolNode::Ptr>& globalNodes )
{
	SymbolNode::SmartDepthIterator it(projNode, SymbolNode::SmartDepthIterator::PREORDER,
		SymbolInfo::Class | SymbolInfo::FuncPublic | SymbolInfo::Enum | SymbolInfo::VarPublic,
		SymbolInfo::All & ~(SymbolInfo::NonpublicMember | SymbolInfo::Unknown | SymbolInfo::SignalSlot | SymbolInfo::Block)
		);

	for (SymbolNode::Ptr pNode; pNode = *it; ++it)
	{
		if (pNode->isGlobalSymbol())
		{
			globalNodes.push_back(pNode);
		}
	}
}

void SymbolTree::getNonProjChild( const SymbolNode::Ptr& projNode, QList<SymbolNode::Ptr>& childList )
{
	projNode->findChild(
		SymbolInfo::All        &
		~(SymbolInfo::Project) ,
		childList);
}



int CodeAtlas::SymbolNode::numInEdges( unsigned edgeTypeMask )
{
	int nEdges = 0;
	for (EdgeListMap::ConstIterator pEdge = m_inEdgeList.constBegin(); pEdge != m_inEdgeList.constEnd(); ++pEdge)
	{
		if (pEdge.value()->type() & edgeTypeMask)
			nEdges++;
	}
	return nEdges;
}

int CodeAtlas::SymbolNode::numOutEdges( unsigned edgeTypeMask )
{
	int nEdges = 0;
	for (EdgeListMap::ConstIterator pEdge = m_outEdgeList.constBegin(); pEdge != m_outEdgeList.constEnd(); ++pEdge)
	{
		if (pEdge.value()->type() & edgeTypeMask)
			nEdges++;
	}
	return nEdges;
}

const SymbolInfo::ElementType CodeAtlas::SymbolNode::getElementType() const
{
	// root node
	if (m_parent.isNull())
		return SymbolInfo::Root;
	Ptr pParent = m_parent.toStrongRef();
	for (ConstChildIterator pChild = pParent->childConstBegin();
		pChild != pParent->childConstEnd(); ++pChild)
	{
		if (pChild.value().data() == this)
		{
			return pChild.key().elementType();
		}
	}
	return SymbolInfo::Unknown;
}
