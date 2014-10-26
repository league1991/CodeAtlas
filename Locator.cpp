#include "StdAfx.h"
#include "locator.h"

using namespace CodeAtlas;

const float Locator::m_avgClassDist = 20;

Locator::Locator(void)
{
}

Locator::~Locator(void)
{
}

// int Locator::addOrGetWord( const QString& word )
// {
// 	if (!m_wordIdxMap.contains(word))
// 	{
// 		m_wordIdxMap.insert(word, m_wordList.size());
// 		m_wordList.push_back(word);
// 	}
// 	return m_wordIdxMap[word];
// }

void Locator::compute( const SymbolTreeBuilder& collector )
{
	SymbolTree& tree = collector.getSymbolTree();
	SymbolPath path;
	SymbolNode::ConstPtr root = tree.getRoot();
	QHash<SymbolPath, SymbolData> symbolList;

	collectWords(path, *root, symbolList, 0);

	buildProjDepend(collector.getDependencyData());
	computeProjLevel();

	updateProjectData(collector.getDependencyData());
}

SymbolData Locator::collectWords( const SymbolPath& path, const SymbolNode& node,
								  QHash<SymbolPath, SymbolData>& curSymbolList,
								  int   depth)
{
	const SymbolInfo* curInfo = path.getLastSymbol();

	// if this node is not new, return cache
	if (curInfo && curInfo->isProject() && m_projectWordList.contains(path) &&
		m_projectWordList[path].m_timeStamp >= node.timeStamp())
	{
		ProjectData& projWord = m_projectWordList[path];
		curSymbolList  = projWord.m_symbolData;
		return projWord.m_projectData;
	}

	SymbolData thisItem;
	bool isAllProjectChild = true;
	SymbolNode::ConstChildIterator pChild;
	for (pChild = node.childConstBegin(); pChild != node.childConstEnd(); ++pChild)
	{
		const SymbolInfo&   childInfo = pChild.key();
		const SymbolNode::Ptr& childNode = pChild.value();

		SymbolPath     childPath = path;
		childPath.addChildSymbol(childInfo);

		// get child node's vocabulary and vocabulary list
		QHash<SymbolPath, SymbolData> childSymbolList;
		SymbolData childItem    = SymbolWordAttr::getSymbolData(childNode);
		SymbolData childSubItem = collectWords(childPath, *childNode, childSymbolList, depth+1);
		childItem.mergeWords(childSubItem);

		// class item or top level function will be added 
		if (childInfo.isClass() || 
			(curInfo && (curInfo->isProject() || curInfo->isNamespace()) && childInfo.isFunction()))
		{
			childItem.computeStatistics();
			curSymbolList[childPath] = childItem;
		}
		thisItem.mergeWords(childItem);

		// merge vocabulary list
		for (QHash<SymbolPath, SymbolData>::Iterator pChildList = childSymbolList.begin();
			 pChildList != childSymbolList.end(); ++pChildList)
		{
			pChildList.value().computeStatistics();
			curSymbolList[pChildList.key()] = pChildList.value();
		}

		isAllProjectChild &= childInfo.isProject();
	}

	// if current node is a lowest-level project, its vocabulary list is added.
	if (curInfo && curInfo->isProject() && !isAllProjectChild && curSymbolList.size() != 0)
	{
		ProjectData& projWord  = m_projectWordList[path];
		projWord.m_symbolData  = curSymbolList;
		projWord.m_projectData = thisItem;
		projWord.m_timeStamp   = node.timeStamp();					// only update new projects
		projWord.m_isDirty     = true;
	}
	return thisItem;
}




void SymbolData::mergeWords( const SymbolData& item )
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

// SymbolData Locator::getWordItem( const SymbolInformation& info, const SymbolNode::Ptr& node)
// {
// 	QStringList list = m_wordExtractor.normalizeWords(info.name());
// 	list << m_wordExtractor.normalizeWords(info.type());
// 	CodeAttr::Ptr pCode = node->getAttr<CodeAttr>();
// 	if (pCode)
// 	{
// 		list << m_wordExtractor.normalizeWords(pCode->getCode());
// 	}
// 	SymbolData item;
// 	for (int ithWord = 0; ithWord < list.size(); ++ithWord)
// 	{
// 		const QString& word = list[ithWord];
// 		int idx = addOrGetWord(word);
// 		if (!item.m_wordIdxMap.contains(idx))
// 			item.m_wordIdxMap[idx] = 1;
// 		else
// 			item.m_wordIdxMap[idx]++;
// 	}
// 	 
// 	return item;
// }

void Locator::printWords()const
{
	for (QHash<SymbolPath, ProjectData>::ConstIterator pProj = m_projectWordList.constBegin();
		 pProj != m_projectWordList.constEnd(); ++pProj)
	{
		const ProjectData& proj = pProj.value();

		qDebug() << pProj.key().toString();
		QHash<SymbolPath, SymbolData>::ConstIterator pIter;
		for (pIter = proj.m_symbolData.begin(); pIter != proj.m_symbolData.end(); ++pIter)
		{
			qDebug() << pIter.key().toString();
			QString str;

			QMap<int,float>::ConstIterator pWordMap;
			for (pWordMap = pIter.value().m_wordWeightMap.constBegin();
				pWordMap != pIter.value().m_wordWeightMap.constEnd(); ++pWordMap)
			{
				str += "\t" + SymbolWordAttr::getWord(pWordMap.key()) + " " +QString::number(pWordMap.value()) + "\n";			
			}
			qDebug() << str;
		}
	}
}

void Locator::clear()
{
//	m_symbolWordList.clear();
	m_projectWordList.clear();

//	SymbolWordAttr::clearAllWords();
}

void Locator::buildDocTermMat(const QHash<SymbolPath, SymbolData>& symbolWordList,
									SparseMatrix& docTermMat,
									Eigen::VectorXd& radiusVec)
{
	int nSymbols = symbolWordList.size();
	int nWords   = SymbolWordAttr::totalWords();
	QVector<float> wordCountPerDoc(nSymbols,0.f);	// total number of words for each doc
	QVector<float> docCountPerWord(nWords  ,0.f);	// total number of doc   for each word

	docTermMat = SparseMatrix(nSymbols, nWords);
	radiusVec.resize(nSymbols);
	QHash<SymbolPath, SymbolData>::ConstIterator pSymbol;
	int ithSymbol = 0;
	for (pSymbol =  symbolWordList.begin(); 
		 pSymbol != symbolWordList.end(); ++pSymbol, ++ithSymbol)
	{
		const SymbolData& item = pSymbol.value();
		wordCountPerDoc[ithSymbol] = item.getTotalWordCount();

		QMap<int,float>::ConstIterator pWord;
		for (pWord = item.m_wordWeightMap.begin(); 
			 pWord != item.m_wordWeightMap.end(); ++pWord)
		{
			int wordId = pWord.key();
			float wordCount = pWord.value();
 			docCountPerWord[wordId]    += 1;
			docTermMat.insert(ithSymbol, wordId) = wordCount;
		}
		radiusVec(ithSymbol) = item.getRadius();
	}
	docTermMat.makeCompressed();
}


void Locator::saveMatsToFile( const QString& fileName )
{
	m_wordLocator.saveMatsToFile(fileName);
} 



void Locator::getSymbolPositions( QList<SymbolPosition>& posList )
{
	for (QHash<SymbolPath, ProjectData>::ConstIterator pProj = m_projectWordList.constBegin();
		pProj != m_projectWordList.constEnd(); ++pProj)
	{
		const ProjectData& proj = pProj.value();
		QHash<SymbolPath, SymbolData>::ConstIterator pSym;
		for (pSym = proj.m_symbolData.begin(); pSym != proj.m_symbolData.end(); ++pSym)
		{
			SymbolPosition pos;
			pos.m_path = pSym.key();
			pos.m_position = pSym.value().getLocalPos() + proj.m_projectData.getLocalPos();
			pos.m_radius   = pSym.value().getRadius();
			pos.m_level    = pSym.value().m_level;
			posList.push_back(pos);
		}
	}
}

void Locator::compute2DPos()
{
	float avgRadius = 0;
	int ithProj = 0;
	QVector<float> projRadiusArray;
	QHash<SymbolPath, SymbolData> projSymbolWordList;
	for (QHash<SymbolPath, ProjectData>::Iterator pProj = m_projectWordList.begin();
		 pProj != m_projectWordList.end(); ++pProj, ++ithProj)
	{
		ProjectData& proj = pProj.value();
		if (!proj.m_isDirty)
			continue;
		QVector<QVector2D>pos;

		// compute items layout in a proj
		float radius;
		layoutByGraph(proj.m_symbolData, pos, 4.f, &radius);
		avgRadius += radius;
		projRadiusArray.append(radius);

		int ithPos = 0;
		for (QHash<SymbolPath, SymbolData>::Iterator pSym = proj.m_symbolData.begin();
			 pSym != proj.m_symbolData.end(); ++pSym)
		{
			pSym.value().setLocalPos(pos[ithPos++]);
		}

		proj.m_projectData.computeStatistics();
		projSymbolWordList[pProj.key()] = proj.m_projectData;
	}
	avgRadius /= ithProj;

	// compute project layout
	QVector<QVector2D>pos;
	int ithPos = 0;
	float allRadius;
	layoutByWord(projSymbolWordList, pos, 1.f, &allRadius, &projRadiusArray);
	for (QHash<SymbolPath, ProjectData>::Iterator pProj = m_projectWordList.begin();
		pProj != m_projectWordList.end(); ++pProj)
	{
		pProj.value().m_projectData.setLocalPos(pos[ithPos++]);
	}


// 	for (QHash<SymbolPath, ProjectData>::Iterator pProj = m_projectWordList.begin();
// 		pProj != m_projectWordList.end(); ++pProj)
// 	{
// 		ProjectData& proj = pProj.value();
// 		const QVector2D& offset = proj.m_projectWordItem.m_position2D;
// 
// 		for (QHash<SymbolPath, SymbolWordItem>::Iterator pSym = proj.m_symbolWordList.begin();
// 			 pSym != proj.m_symbolWordList.end(); ++pSym)
// 		{
// 			pSym->m_position2D += offset;
// 		}
// 	}

}
void Locator::layoutByGraph(const QHash<SymbolPath, SymbolData>& symbolWord, 
							QVector<QVector2D>& pos2D,
							float sparseFactor, 
							float* radius,
							QVector<float>* projRadius)
{
	Eigen::MatrixXd distMat;
	Eigen::VectorXd radiusVec;
	Eigen::VectorXd alignVec;
	buildPathDistMat(symbolWord, distMat, radiusVec, alignVec);

	// use external radius data
	if (projRadius)
	{
		radiusVec.resize(projRadius->size());
		for (int i = 0; i < projRadius->size(); ++i)
			radiusVec[i] = (*projRadius)[i];
	}

	StressMDSSolver solver;
	solver.setSparseFactor(sparseFactor);
	solver.setUseExternalInitPos(false);
	solver.setUsePostProcess(true);
	for (int ithSym = 0; ithSym < distMat.rows(); ++ithSym)
	{
		MDSSolver::MDSData data;
		//data.m_dataVec = m_docTermMatLSI.row(ithDoc);
		data.m_dataVec = Eigen::VectorXd(1);
		data.m_init2DPos = QVector2D(0,0);
		data.m_radius   = radiusVec[ithSym];
		//data.m_alignVal = alignVec(ithSym); 
		solver.addData(data);
	}
	solver.setExternalDissimilarityMat(distMat);	
	solver.compute();
	//solver.print2DPosition();

	pos2D = solver.get2DPosition();
	QRectF rect;
	float  r;
	WordLocator::compute2DStatistics(pos2D, rect, r);
	if (radius)
		*radius = r + 0.1f;
}

void Locator::layoutByWord(const QHash<SymbolPath, SymbolData>& symbolWord, 
								 QVector<QVector2D>& pos2D,
								 float sparseFactor, 
								 float* radius,
								 QVector<float>* projRadius)
{
	// get doc term matrix and radius of each data
	SparseMatrix docTermMat;
	Eigen::VectorXd radiusVec;
	buildDocTermMat(symbolWord, docTermMat, radiusVec);

	// use external radius data
	if (projRadius)
	{
		radiusVec.resize(projRadius->size());
		for (int i = 0; i < projRadius->size(); ++i)
			radiusVec[i] = (*projRadius)[i];
	}

	// compute 2D position
	m_wordLocator.setDocTermMat(docTermMat, radiusVec);
	m_wordLocator.setUseTfIdfMeasure(false);
	m_wordLocator.compute(sparseFactor); 
	//m_wordLocator.saveMatsToFile("H:/Programs/QtCreator/qt-creator_master/src/plugins/MyPlugin/CodeAtlas/codeData.m");
	pos2D = m_wordLocator.getOri2DPositions();
	if (radius)
		*radius = m_wordLocator.getOri2DRadius() + 0.1f;
}

void SymbolData::computeStatistics()
{
	float totalCount = 0;
	QMap<int,float>::ConstIterator pWord;
	for (pWord = m_wordWeightMap.begin(); pWord != m_wordWeightMap.end(); ++pWord)
		totalCount += pWord.value();
	m_totalWordCount = totalCount;

	m_radius    = qSqrt(1 + m_totalWordCount * .1f);
	return;
}

float SymbolData::getRadius() const
{
	return m_radius;
}

void Locator::buildProjDepend(const SymbolTreeBuilder::DependencyData& dependData)
{
	const SymbolTreeBuilder::DependDictPtr projDepend = dependData.m_projDepend;
	for (SymbolTreeBuilder::DependDict::ConstIterator pDep = projDepend->constBegin();
		 pDep != projDepend->constEnd(); ++pDep)
	{
		const SymbolPath& proj1 = pDep.key();
		const SymbolPath& proj2 = pDep.value();

		if (!m_projectWordList.contains(proj1) ||
			!m_projectWordList.contains(proj2) ||
			proj1 == proj2)
			continue;

		m_projectWordList[proj1].m_projectData.m_lowLevelSym.insert(proj2);
		m_projectWordList[proj2].m_projectData.m_highLevelSym.insert(proj1);
	}
}

void Locator::computeProjLevel()
{
	QSet<SymbolPath> projProcessed;
	QSet<SymbolPath> curLowestSet;
	for (QHash<SymbolPath, ProjectData>::Iterator pProj = m_projectWordList.begin();
		 pProj != m_projectWordList.end(); ++pProj)
	{
		ProjectData& proj = pProj.value();
		SymbolData&  data = proj.m_projectData;
		bool isLowest = data.m_lowLevelSym.size() == 0;
		bool isHighest= data.m_highLevelSym.size()== 0;
		if (isLowest && isHighest)
		{	// isolated project
			proj.m_projectData.m_level = 0;
			projProcessed.insert(pProj.key());
		}
		else if (isLowest)
			curLowestSet.insert(pProj.key());
	}

	int curLevel = 0;
	for (; curLowestSet.size() != 0; curLevel++)
	{
		// set current lowest level
		for (QSet<SymbolPath>::ConstIterator pProjPath = curLowestSet.constBegin();
			 pProjPath != curLowestSet.constEnd(); pProjPath++)
		{
			m_projectWordList[*pProjPath].m_projectData.m_level = curLevel;
			projProcessed.insert(*pProjPath);
		}

		QSet<SymbolPath> nextLowestSet;
		QSet<SymbolPath> candidateSet;
		for (QSet<SymbolPath>::ConstIterator pProjPath = curLowestSet.constBegin();
			pProjPath != curLowestSet.constEnd(); pProjPath++)
		{
			const ProjectData& projData = m_projectWordList[*pProjPath];
			const SymbolData&  data     = projData.m_projectData;

			// insert projects that only depends on processed project to next lowest set
			for (QSet<SymbolPath>::ConstIterator pHighProj = data.m_highLevelSym.constBegin();
				pHighProj != data.m_highLevelSym.constEnd(); pHighProj++)
			{
				// ignore self-link or loop node
				if (projProcessed.contains(*pHighProj))
					continue;
				ProjectData& highProj = m_projectWordList[*pHighProj];
				SymbolData & highData = highProj.m_projectData;
				if ((highData.m_lowLevelSym-projProcessed).size() == 0)
					nextLowestSet.insert(*pHighProj);
				else
					candidateSet.insert(*pHighProj);
			}
		}

		// encounter loop
		if (nextLowestSet.size() == 0 && candidateSet.size() != 0)
			curLowestSet = candidateSet;
		else
			curLowestSet = nextLowestSet;
	}
}

void Locator::computeSymbolLevel(ProjectData& proj)
{
	QSet<SymbolPath> symProcessed;
	QSet<SymbolPath> curLowestSet;
	for (QHash<SymbolPath, SymbolData>::Iterator pSym = proj.m_symbolData.begin();
		pSym != proj.m_symbolData.end(); ++pSym)
	{
		SymbolData&  data = pSym.value();
		bool isLowest = data.m_lowLevelSym.size() == 0;
		bool isHighest= data.m_highLevelSym.size()== 0;
		if (isLowest && isHighest)
		{	// isolated project
			data.m_level = 0;
			symProcessed.insert(pSym.key());
		}
		else if (isLowest)
			curLowestSet.insert(pSym.key());
	}

	int maxLevel = 0;
	for (int curLevel = 0; curLowestSet.size() != 0; curLevel++)
	{
		maxLevel = curLevel;

		// set current lowest level
		for (QSet<SymbolPath>::ConstIterator pSym = curLowestSet.constBegin();
			pSym != curLowestSet.constEnd(); pSym++)
		{
			proj.m_symbolData[*pSym].m_level = curLevel;
			symProcessed.insert(*pSym);
		}

		QSet<SymbolPath> nextLowestSet;
		QSet<SymbolPath> candidateSet;
		for (QSet<SymbolPath>::ConstIterator pSym = curLowestSet.constBegin();
			pSym != curLowestSet.constEnd(); pSym++)
		{
			const SymbolData&  data     = proj.m_symbolData[*pSym];

			// insert projects that only depends on processed project to next lowest set
			for (QSet<SymbolPath>::ConstIterator pHighSym = data.m_highLevelSym.constBegin();
				pHighSym != data.m_highLevelSym.constEnd(); pHighSym++)
			{
				// ignore self-link or loop node
				if (symProcessed.contains(*pHighSym))
					continue;
				SymbolData & highData = proj.m_symbolData[*pHighSym];
				if ((highData.m_lowLevelSym-symProcessed).size() == 0)
					nextLowestSet.insert(*pHighSym);
				else
					candidateSet.insert(*pHighSym);
			}
		}

		// encounter loop
		if (nextLowestSet.size() == 0 && candidateSet.size() != 0)
			curLowestSet = candidateSet;
		else
			curLowestSet = nextLowestSet;
	}
	proj.m_nSymbolLevel = maxLevel + 1;
}
void Locator::buildSymbolDepend( const SymbolTreeBuilder::DependencyData& dependData )
{
	const SymbolTreeBuilder::DependDictPtr symDepend = dependData.m_symbolDepend;
	for (SymbolTreeBuilder::DependDict::ConstIterator pDep = symDepend->constBegin();
		pDep != symDepend->constEnd(); ++pDep)
	{
		const SymbolPath& sym1 = pDep.key();
		const SymbolPath& sym2 = pDep.value();

		// ignore inter-project links
		const SymbolInfo* proj1 = sym1.getLastProjSymbol();
		const SymbolInfo* proj2 = sym2.getLastProjSymbol();
		SymbolPath topSym1, topSym2;
		sym1.getTopLevelItemPath(topSym1);
		sym2.getTopLevelItemPath(topSym2);
		ProjectData& proj = m_projectWordList[sym1.getProjectPath()];

		if (!proj1 || !proj2 || !(*proj1 == *proj2) ||
			!proj.m_symbolData.contains(topSym1)    ||
			!proj.m_symbolData.contains(topSym2)    ||
			topSym1 == topSym2)
			continue;

		SymbolData&  symData1 = proj.m_symbolData[topSym1];
		SymbolData&  symData2 = proj.m_symbolData[topSym2];
		symData1.m_lowLevelSym.insert(topSym2);
		symData2.m_highLevelSym.insert(topSym1);
	}
}

void Locator::updateProjectData( const SymbolTreeBuilder::DependencyData& dependData )
{
	for (QHash<SymbolPath, ProjectData>::Iterator pProj = m_projectWordList.begin();
		pProj != m_projectWordList.end(); ++pProj)
	{
		ProjectData& projData = pProj.value();
		if (projData.m_isDirty)
		{
			buildSymbolDepend(dependData);
			computeSymbolLevel(projData);
		}
	}
	compute2DPos();
	for (QHash<SymbolPath, ProjectData>::Iterator pProj = m_projectWordList.begin();
		pProj != m_projectWordList.end(); ++pProj)
	{
		ProjectData& projData = pProj.value();
		projData.m_isDirty = false;
	}
}

void Locator::buildPathDistMat( const QHash<SymbolPath, SymbolData>& symbolWordList, 
							    Eigen::MatrixXd& distMat, 
								Eigen::VectorXd& radiusVec,
								Eigen::VectorXd& alignVec)
{
	// add to path index map
	QHash<SymbolPath, int> pathIdxMap;
	QList<QString> pathStrVec;
	int ithPath = 0;
	for (QHash<SymbolPath, SymbolData>::ConstIterator pPath = symbolWordList.constBegin();
		pPath != symbolWordList.constEnd(); ++pPath, ++ithPath)
	{
		pathIdxMap[pPath.key()] = ithPath;
		pathStrVec.push_back(pPath.key().getLastSymbol()->toString());
	}

	// build adjencency matrix
	int n = ithPath;
	ithPath = 0;
	radiusVec.resize(n);
	alignVec = Eigen::VectorXd::Ones(n);
	Eigen::MatrixXf adjMat = Eigen::MatrixXf::Constant(n,n,-1);
	for (QHash<SymbolPath, SymbolData>::ConstIterator pPath = symbolWordList.constBegin();
		pPath != symbolWordList.constEnd(); ++pPath, ++ithPath)
	{
		const SymbolData& symData = pPath.value();
		const SymbolPath& symPath = pPath.key();
		int   srcIdx = pathIdxMap[pPath.key()];

		for (QSet<SymbolPath>::ConstIterator pChild = symData.m_lowLevelSym.constBegin();
			pChild != symData.m_lowLevelSym.constEnd(); ++pChild)
		{
			QHash<SymbolPath, int>::ConstIterator pTar = pathIdxMap.find(*pChild);
			if (pTar == pathIdxMap.constEnd())
				continue;
			int tarIdx = pTar.value();
			adjMat(srcIdx, tarIdx) = adjMat(tarIdx, srcIdx) = 1;
		}

		radiusVec(ithPath) = symData.getRadius();
		if(const SymbolInfo* info = symPath.getLastSymbol())
		{
			alignVec(ithPath) = MDSSolver::getAlignVal(info->name());
		}
	}

	// build distance matrix
	MatrixXf dMat;
	GraphUtility::computePairDist(adjMat, dMat);
	distMat = dMat.cast<double>();

	// save mats for debug
	QString matStr = GlobalUtility::toMatlabString(adjMat, "adjMat");
	matStr +=        GlobalUtility::toMatlabString(dMat, "distMat");
	matStr +=        GlobalUtility::toMatlabString(pathStrVec, "entityName");
	matStr +=        "adjMat(find(adjMat<0)) = 0;\n";
	GlobalUtility::saveString("H:\\Programs\\QtCreator\\qt-creator_master\\src\\plugins\\MyPlugin\\CodeAtlas\\tests\\2-20\\matStr.m", matStr);

	// fill distances of different connect components 
	float maxDist= distMat.maxCoeff() * 2.f;
	for (int i = 0; i < distMat.rows(); i++)
		for (int j = 0; j < distMat.cols(); j++)
			if (distMat(i,j) == -1)
				distMat(i,j) = maxDist;
}



