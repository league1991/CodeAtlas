#include "StdAfx.h"
#include "SymbolNodeAttr.h"

using namespace CodeAtlas;
#ifdef USING_QT_4
using namespace Qt4ProjectManager;
#elif defined(USING_QT_5)
using namespace QmakeProjectManager;
#endif

QHash<QString, int> SymbolWordAttr::m_wordIdxMap;
QList<QString>		SymbolWordAttr::m_wordList;

SymbolNodeAttr& SymbolNodeAttr::operator=( const SymbolNodeAttr& s )
{
	return copy(s);
}

SymbolNodeAttr::Ptr SymbolNodeAttr::duplicate()const
{
	SymbolNodeAttr::Ptr newNode = creator();
	(*newNode) = (*this);
	return newNode;
}



bool SymbolNodeAttr::isSameType( const SymbolNodeAttr& s ) const
{
	return m_type == s.m_type;
}

SymbolNodeAttr& CodeAttr::copy( const SymbolNodeAttr& s )
{
	if (isSameType(s))
	{
		CodeAttr& sc = (CodeAttr&)s;
		m_code = sc.m_code;
	}
	return *this;
}

SymbolNodeAttr& CodeAttr::unite( const SymbolNodeAttr& s )
{
	if (isSameType(s))
	{
		CodeAttr& sc = (CodeAttr&)s;
		m_code += ("\n" + sc.m_code);
	}
	return *this;
}

SymbolNodeAttr& CodeAttr::substract( const SymbolNodeAttr& s )
{
	if (isSameType(s))
	{
		CodeAttr& sc = (CodeAttr&)s;
		m_code.replace(sc.m_code, "");
	}
	return *this;
}


/*!
Adds information about symbol location from a \location.
\sa SymbolLocation, removeSymbolLocation, symbolLocations
*/

void LocationAttr::addSymbolLocation(const SymbolLocation &location)
{
	symbolLocations.insert(location);
}

/*!
Adds information about symbol locations from \a locations.
\sa SymbolLocation, removeSymbolLocation, symbolLocations
*/

void LocationAttr::addSymbolLocation(const QSet<SymbolLocation> &locations)
{
	symbolLocations.unite(locations);
}

/*!
Removes information about \a location.
\sa SymbolLocation, addSymbolLocation, symbolLocations
*/

void LocationAttr::removeSymbolLocation(const SymbolLocation &location)
{
	symbolLocations.remove(location);
}

/*!
Removes information about \a locations.
\sa SymbolLocation, addSymbolLocation, symbolLocations
*/

void LocationAttr::removeSymbolLocations(const QSet<SymbolLocation> &locations)
{
	symbolLocations.subtract(locations);
}

/*!
Gets information about symbol positions.
\sa SymbolLocation, addSymbolLocation, removeSymbolLocation
*/

const QSet<SymbolLocation>& LocationAttr::getSymbolLocations() const
{
	return symbolLocations;
}

LocationAttr::LocationAttr() :SymbolNodeAttr(SymbolNodeAttr::ATTR_LOCATION),symbolLocations()
{

}

LocationAttr::LocationAttr( const QSet<SymbolLocation>& l ) :SymbolNodeAttr(SymbolNodeAttr::ATTR_LOCATION),symbolLocations(l)
{

}

SymbolNodeAttr& LocationAttr::copy( const SymbolNodeAttr& s )
{
	if (isSameType(s))
	{
		symbolLocations = ((const LocationAttr&)s).symbolLocations;
	}
	return *this;
}

SymbolNodeAttr& LocationAttr::unite( const SymbolNodeAttr& s )
{
	if (isSameType(s))
	{
		symbolLocations.unite(((const LocationAttr&)s).symbolLocations);
	}
	return *this;
}

SymbolNodeAttr& LocationAttr::substract( const SymbolNodeAttr& s )
{
	if (isSameType(s))
	{
		symbolLocations.subtract(((const LocationAttr&)s).symbolLocations);
	}
	return *this;
}

SymbolNodeAttr& ChildInfoAttr::copy( const SymbolNodeAttr& s )
{
	if (ChildInfoAttr* pS = castTo<ChildInfoAttr>(s))
	{
		m_nChildFunctions = pS->m_nChildFunctions;
		m_nChildVariables = pS->m_nChildVariables;
	}
	return *this;
}

SymbolNodeAttr& ChildInfoAttr::unite( const SymbolNodeAttr& s )
{
	if (ChildInfoAttr* pS = castTo<ChildInfoAttr>(s))
	{
		m_nChildFunctions += pS->m_nChildFunctions;
		m_nChildVariables += pS->m_nChildVariables;
	}
	return *this;
}

SymbolNodeAttr& ChildInfoAttr::substract( const SymbolNodeAttr& s )
{
	if (ChildInfoAttr* pS = castTo<ChildInfoAttr>(s))
	{
		m_nChildFunctions -= pS->m_nChildFunctions;
		m_nChildVariables -= pS->m_nChildVariables;
	}
	return *this;
}


void SymbolWordAttr::clearGlobalWordMap()
{
	m_wordIdxMap.clear();
	m_wordList.clear();
}

int SymbolWordAttr::addOrGetWord( const QString& word )
{
	if (!m_wordIdxMap.contains(word))
	{
		m_wordIdxMap.insert(word, m_wordList.size());
		m_wordList.push_back(word);
	}
	return m_wordIdxMap[word];
}

void SymbolWordAttr::setWords( const SymbolNode::Ptr& node )
{
	SymbolInfo info = node->getSymInfo();
	QString wordStr = info.name() + " " + info.type();
	CodeAttr::Ptr pCode = node->getAttr<CodeAttr>();
	if (pCode)
		wordStr += " " + pCode->getCode();
	QStringList list = WordExtractor::normalizeWords(wordStr);

	for (int ithWord = 0; ithWord < list.size(); ++ithWord)
	{
		const QString& word = list[ithWord];
		int idx = addOrGetWord(word);
		QMap<int,float>::Iterator pIdx = m_wordWeightMap.find(idx);
		if (pIdx != m_wordWeightMap.end())
			pIdx.value()++;
		else
			m_wordWeightMap[idx] = 1;
	}
}

SymbolData SymbolWordAttr::getSymbolData( const SymbolNode::Ptr& node )
{
	SymbolInfo info = node->getSymInfo();
	QStringList list = WordExtractor::normalizeWords(info.name());
	list << WordExtractor::normalizeWords(info.type());
	CodeAttr::Ptr pCode = node->getAttr<CodeAttr>();
	if (pCode)
	{
		list << WordExtractor::normalizeWords(pCode->getCode());
	}
	SymbolData item;
	for (int ithWord = 0; ithWord < list.size(); ++ithWord)
	{
		const QString& word = list[ithWord];
		int idx = addOrGetWord(word);
		if (!item.m_wordWeightMap.contains(idx))
			item.m_wordWeightMap[idx] = 1;
		else
			item.m_wordWeightMap[idx]++;
	}

	return item;
}

void SymbolWordAttr::computeStatistics()
{
	float totalCount = 0;
	QMap<int,float>::ConstIterator pWord;
	for (pWord = m_wordWeightMap.begin(); pWord != m_wordWeightMap.end(); ++pWord)
		totalCount += pWord.value();
	m_totalWeight = totalCount;
	return;
}

void SymbolWordAttr::mergeWords( const SymbolWordAttr& item )
{
	QMap<int, float>::ConstIterator pWordMap;
	for (pWordMap = item.m_wordWeightMap.constBegin(); pWordMap != item.m_wordWeightMap.constEnd(); ++pWordMap)
	{
		if (m_wordWeightMap.contains(pWordMap.key()))
			m_wordWeightMap[pWordMap.key()] += pWordMap.value();
		else
			m_wordWeightMap[pWordMap.key()] =  pWordMap.value();
	}
	m_totalWeight = item.m_totalWeight;
}

void SymbolWordAttr::mergeWords( const Document::Ptr& pDoc )
{
	QByteArray utf8Src = pDoc->utf8Source();
	QString src = QString::fromUtf8(utf8Src.constData(), utf8Src.size());
	QStringList list = WordExtractor::normalizeWords(src);
	mergeWords(list);
}

void SymbolWordAttr::mergeWords( const QStringList& list )
{
	for (int ithWord = 0; ithWord < list.size(); ++ithWord)
	{
		const QString& word = list[ithWord];
		int idx = addOrGetWord(word);
		if (!m_wordWeightMap.contains(idx))
			m_wordWeightMap[idx] = 1;
		else
			m_wordWeightMap[idx]++;
	}
}

void SymbolWordAttr::mergeWords( const QString& src )
{
	QStringList list = WordExtractor::normalizeWords(src);
	mergeWords(list);
}

SymbolNodeAttr& SymbolWordAttr::copy( const SymbolNodeAttr& s )
{
	if (SymbolWordAttr* pS = castTo<SymbolWordAttr>(s))
	{
		m_wordWeightMap = pS->m_wordWeightMap;
		m_totalWeight   = pS->m_totalWeight;
	}
	return *this;
}

SymbolNodeAttr& SymbolWordAttr::unite( const SymbolNodeAttr& s )
{
	if (SymbolWordAttr* pS = castTo<SymbolWordAttr>(s))
	{
		mergeWords(*pS);
		m_totalWeight += pS->m_totalWeight;
	}
	return *this;
}

void SymbolWordAttr::clear()
{
	m_wordWeightMap.clear();m_totalWeight = 0;
}

QString SymbolWordAttr::toString() const
{
	QString res;
	for (QMap<int,float>::ConstIterator pWord = m_wordWeightMap.constBegin();
		pWord != m_wordWeightMap.constEnd(); ++pWord)
	{
		const QString& word = m_wordList[pWord.key()];
		res += (word + QString(": %1  ").arg(pWord.value()));
	}
	return res;
}

double SymbolWordAttr::cosSimilarity( const SymbolWordAttr& other ) const
{
	const QMap<int,float>& wM0 = m_wordWeightMap;
	const QMap<int,float>& wM1 = other.m_wordWeightMap;
	if (wM0.size() == 0 || wM1.size() == 0)
		return 0.f;

	QMap<int,float>::ConstIterator pW0 = wM0.constBegin();
	QMap<int,float>::ConstIterator pW1 = wM1.constBegin();
	double lw0 = 0.0, lw1 = 0.0, innerProduct = 0.0;
	while (pW0 != wM0.constEnd() || pW1 != wM1.constEnd())
	{
		while( pW0 != wM0.constEnd() && 
			(pW1 == wM1.constEnd() || pW0.key() < pW1.key()))
		{
			lw0 += pW0.value() * pW0.value();	
			++pW0;
		}
		while( pW1 != wM1.constEnd() && 
			(pW0 == wM0.constEnd() || pW1.key() < pW0.key()))
		{
			lw1 += pW1.value() * pW1.value();	
			++pW1;
		}
		if (pW0 != wM0.constEnd() && pW1 != wM1.constEnd()
			&& pW0.key() == pW1.key())
		{
			float v0 = pW0.value();
			float v1 = pW1.value();
			innerProduct += v0 * v1;
			lw0 += v0 * v0;
			lw1 += v1 * v1;
			++pW0; ++pW1;
		}
	}
	return innerProduct / sqrt(lw0 * lw1);
}


void SymbolWordAttr::mergeWords( const SymbolData& item )
{
	QMap<int, float>::ConstIterator pWordMap;
	for (pWordMap = item.m_wordWeightMap.constBegin(); pWordMap != item.m_wordWeightMap.constEnd(); ++pWordMap)
	{
		if (m_wordWeightMap.contains(pWordMap.key()))
			m_wordWeightMap[pWordMap.key()] += pWordMap.value();
		else
			m_wordWeightMap[pWordMap.key()] =  pWordMap.value();
	}
}

void ProjectAttr::setGlobalSymList( const QList<SymbolNode::Ptr>& symList )
{
	m_globalSymbolList = symList;
}

SymbolNodeAttr& ProjectAttr::copy( const SymbolNodeAttr& s )
{
	if (ProjectAttr* pS = castTo<ProjectAttr>(s))
	{
		m_globalSymbolList = pS->m_globalSymbolList;
		m_isDeepParse = pS->m_isDeepParse;
		m_projType    = pS->m_projType;
		m_fileNameList= pS->m_fileNameList;
	}
	return *this;
}

SymbolNodeAttr::Ptr ProjectAttr::creator()const
{
	return SymbolNodeAttr::Ptr(new ProjectAttr);
}

#ifdef USING_QT_4
ProjectAttr::ProjectType ProjectAttr::typeFromQt4ProjType( Qt4ProjectType type )
{
	switch(type)
	{
	case Qt4ProjectManager::ApplicationTemplate:
		return PROJ_PRO_APP;
	case Qt4ProjectManager::LibraryTemplate:
		return PROJ_PRO_LIB;
	case Qt4ProjectManager::ScriptTemplate:
		return PROJ_PRO_SCRIPT;
	case Qt4ProjectManager::AuxTemplate:
		return PROJ_PRO_AUX;
	case Qt4ProjectManager::SubDirsTemplate:
		return PROJ_PRO_SUBDIR;
	default:
	case Qt4ProjectManager::InvalidProject:
		return PROJ_UNKNOWN;
	}
}
#endif
#ifdef USING_QT_5
CodeAtlas::ProjectAttr::ProjectType CodeAtlas::ProjectAttr::typeFromQt5ProjType( QmakeProjectType type )
{
	switch(type)
	{
	case ApplicationTemplate:
		return PROJ_PRO_APP;
	case LibraryTemplate:
		return PROJ_PRO_LIB;
	case ScriptTemplate:
		return PROJ_PRO_SCRIPT;
	case AuxTemplate:
		return PROJ_PRO_AUX;
	case SubDirsTemplate:
		return PROJ_PRO_SUBDIR;
	default:
	case InvalidProject:
		return PROJ_UNKNOWN;
	}
}
#endif

SymbolNodeAttr& FuzzyDependAttr::copy( const SymbolNodeAttr& s )
{
	if (FuzzyDependAttr* pS = castTo<FuzzyDependAttr>(s))
	{
		m_childDependMatrix = pS->m_childDependMatrix;
		m_childEdgeWeight   = pS->m_childEdgeWeight;
		m_childLevel        = pS->m_childLevel;
	}
	return *this;
}

QString FuzzyDependAttr::toString() const
{
	QString res;
	res += "edge-vtx matrix\n";
	for (int k=0; k<m_childDependMatrix.outerSize(); ++k)
	{
		for (SparseMatrix::InnerIterator it(m_childDependMatrix,k); it; ++it)
		{
			res += QString("(%1 vtx,\t%2 edge) = %3\n").arg(it.row()).arg(it.col()).arg(it.value());
		}
	}

	res += "edge weight vector\n";
	for (int i = 0; i < m_childEdgeWeight.size(); ++i)
	{
		res += QString("%1\t").arg(m_childEdgeWeight[i]);
	}

	res += "child level vector\n";
	for (int i = 0; i < m_childEdgeWeight.size(); ++i)
	{
		res += QString("%1\t").arg(m_childLevel[i]);
	}
	return res;
}

SymbolNodeAttr& FuzzyDependAttr::unite( const SymbolNodeAttr& s )
{
	return *this;
}




SymbolNodeAttr& UIElementAttr::copy( const SymbolNodeAttr& s )
{
	if (UIElementAttr* pS = castTo<UIElementAttr>(s))
	{
		m_position = pS->m_position;
		m_radius   = pS->m_radius;
		NodeUIItem* parent = (NodeUIItem*)pS->getUIItem()->parentItem();
		buildOrUpdateUIItem(pS->getNode().toStrongRef(), parent);
	}
	return *this;
}

void UIElementAttr::buildOrUpdateUIItem(	const SymbolNode::Ptr& thisNode, 
											NodeUIItem * parent )
{
	m_node = thisNode.toWeakRef();
	m_uiItem = NodeUIItem::creator(thisNode, parent);
	m_uiItem->setPos(m_position);
	m_uiItem->setEntityRadius(m_radius);
	m_uiItem->setLevel(m_level);
	m_uiItem->buildUI();
}

void UIElementAttr::resetUIItem( const QSharedPointer<SymbolNode>& thisNode, NodeUIItem* parent )
{
	m_uiItem = NodeUIItem::creator(thisNode, parent);
	m_uiItem->setPos(m_position);
	m_uiItem->setEntityRadius(m_radius);
	m_uiItem->buildUI();
}

QString UIElementAttr::toString() const
{
	QString res;
	res += QString("pos: %1, %2   radius: %3\n").arg(m_position.x()).arg(m_position.y()).arg(m_radius);
	return res;
}

void CodeAtlas::ProjectAttr::setProjectPath( const SymbolPath& path )
{
	m_projPath = path;
}



bool CodeAtlas::UIElementAttr::setVisualHull( const QVector<QPointF>& convexHull )
{
	if (convexHull.size() == 0)
		return false;
	m_localHull = QSharedPointer<QPolygonF>(new QPolygonF(convexHull));
	return true;
}

QSharedPointer<QPolygonF> CodeAtlas::UIElementAttr::getVisualHull()
{
	return m_localHull;
// 	if (m_localHull.isNull())
// 		return m_localHull;
// 	return QSharedPointer<QPolygonF>(new QPolygonF(m_localHull->translated(m_position)));
}

void CodeAtlas::UIElementAttr::setEntryInfo( const QVector<QPointF>& inEntries, const QVector<QPointF>& outEntries, const QVector<QPointF>& inNormals, const QVector<QPointF>& outNormals )
{
	m_inEntries = inEntries;
	m_outEntries = outEntries;
	m_inNormals = inNormals;
	m_outNormals = outNormals;
}
