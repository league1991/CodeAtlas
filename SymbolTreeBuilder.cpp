#include "StdAfx.h"
#include "symboltreebuilder.h"

using namespace CodeAtlas;
using namespace ProjectExplorer;
#ifdef USING_QT_4
using namespace Qt4ProjectManager;
#elif defined(USING_QT_5)
using namespace QmakeProjectManager;
#endif

SymbolTreeBuilder::SymbolTreeBuilder(QObject *parent)
: QObject(parent),
d(new SymbolColletorPrivate())
{
// 	d->timer = new QTimer(this);
// 	d->timer->setSingleShot(true);
	// connect signal/slots
	// internal data reset
//	connect(this, SIGNAL(resetDataDone()), SLOT(onResetDataDone()), Qt::QueuedConnection);

	// timer for emitting changes
// 	connect(d->timer, SIGNAL(timeout()), SLOT(updateSymbolTree()), Qt::QueuedConnection);

}

/*!
Destructs the SymbolColletor object.
*/

SymbolTreeBuilder::~SymbolTreeBuilder()
{
	delete d;
}

/*!
Checks \a item for lazy data population of a QStandardItemModel.
*/

bool SymbolTreeBuilder::canFetchMore(QStandardItem *item) const
{
	SymbolNode::ConstPtr ptr = findItemByRoot(item);
	if (ptr.isNull())
		return false;
	return ptr->canFetchMore(item);
}

/*!
Checks \a item for lazy data population of a QStandardItemModel.
\a skipRoot skips the root item.
*/

void SymbolTreeBuilder::fetchMore(QStandardItem *item, bool skipRoot) const
{
	SymbolNode::ConstPtr ptr = findItemByRoot(item, skipRoot);
	if (ptr.isNull())
		return;
	ptr->fetchMore(item);
}

/*!
Switches to flat mode (without subprojects) if \a flat returns \c true.
*/

void SymbolTreeBuilder::setFlatMode(bool flatMode)
{
	if (flatMode == d->flatMode)
		return;

	// change internal
	d->flatMode = flatMode;

	// regenerate and resend current tree
	updateTree();
}

/*!
Returns the internal tree item for \a item. \a skipRoot skips the root
item.
*/

SymbolNode::ConstPtr SymbolTreeBuilder::findItemByRoot(const QStandardItem *item, bool skipRoot) const
{
	if (!item)
		return SymbolNode::ConstPtr();

	// go item by item to the root
	QList<const QStandardItem *> uiList;
	const QStandardItem *cur = item;
	while (cur) {
		uiList.append(cur);
		cur = cur->parent();
	}

	if (skipRoot && uiList.count() > 0)
		uiList.removeLast();

	QReadLocker locker(&d->rootItemLocker);

	// using internal root - search correct item
	SymbolNode::ConstPtr internal = d->symbolTree.getRoot();

	while (uiList.count() > 0) {
		cur = uiList.last();
		uiList.removeLast();
		const SymbolInfo &inf = AtlasUtils::symbolInformationFromItem(cur);
		internal = internal->child(inf);
		if (internal.isNull())
			break;
	}

	return internal;
}

/*!
Parses the class and produces a new tree.

\sa addProject
*/

// void SymbolTreeBuilder::computeProjBuildDependency()
// {
// 	foreach (const Project *prj, SessionManager::projects()) 
// 	{
// 		if (!prj)
// 			continue;
// 
// 		QString tarProjID ;
// 		QList<Project*> dependProjList = SessionManager::dependencies(prj);
// 	}
// }
SymbolNode::Ptr SymbolTreeBuilder::parse()
{
	QTime time;
	int debug = 0;
	if (debug)
		time.start();

	SymbolNode::Ptr rootItem(new SymbolNode(d->m_curTimeStamp));

	// check all projects
	foreach (const Project *prj, SessionManager::projects()) 
	{
		if (!prj)
			continue;

		SymbolNode::Ptr item;
		if (!d->flatMode)
			item = SymbolNode::Ptr(new SymbolNode(d->m_curTimeStamp));

		QString prjName(prj->displayName());
		QString prjType(prjName);
		if (prj->document())
			prjType = prj->projectFilePath().toString();
		SymbolInfo inf(prjName, prjType, SymbolInfo::Project);

		SymbolPath  projSymPath;
		projSymPath.addChildSymbol(inf);
		QStringList projectList = addProjectNode(item, prj->rootProjectNode(), projSymPath);

		if (d->flatMode) {
			// use prj path (prjType) as a project id
			//            addProject(item, prj->files(Project::ExcludeGeneratedFiles), prjType);
			//! \todo return back, works too long
			SymbolNode::Ptr flatItem = createFlatTree(projectList);
			item.swap(flatItem);
		}
		//SymbolNode::appendChild(rootItem, item, inf);
		//rootItem->appendChild(item, inf);
	}

	// "flat mode", add all project node directly
	for (QHash<QString, CachedPrjData>::Iterator pProj = d->cachedPrjTrees.begin();
		 pProj != d->cachedPrjTrees.end(); ++pProj)
	{
		CachedPrjData& projData = pProj.value();
		const QString& projectID = pProj.key();

		ProjectAttr::Ptr pProjAttr= projData.m_symTree->getOrAddAttr<ProjectAttr>();
		SymbolInfo inf(pProjAttr->getDisplayName(), projectID, SymbolInfo::Project);
		SymbolNode::appendChild(rootItem, projData.m_symTree, inf);
	}
	if (debug)
		qDebug() << "Class View:" << QDateTime::currentDateTime().toString()
		<< "Parsed in " << time.elapsed() << "msecs.";

	return rootItem;
}

/*!
Parses the project with the \a projectId and adds the documents
from the \a fileList to the tree item \a item.
*/

void SymbolTreeBuilder::addProject(SymbolNode::Ptr &item, 
								const QStringList &fileList,
								const SymbolPath& projPath,
								const QString &projectId)
{
	// recalculate cache tree if needed
	SymbolNode::Ptr prj(getCachedOrParseProjectTree(fileList, projectId, projPath));
// 	if (item.isNull())
// 		return;

	item = prj;
	// if there is an item - copy project tree to that item
	//SymbolNode::replaceRoot(prj, item);
}

/*!
Parses \a symbol and adds the results to \a item (as a parent).
*/

void SymbolTreeBuilder::addSymbol(CachedDocData& docCache,
							   const SymbolPath& parentPath, 
							   CPlusPlus::Symbol *childSymbol, 
							   const QByteArray& curDocSource, const LookupContext& context, 
							   int& nFunctions, int& nVariables)
{
	nFunctions = nVariables = 0;
	const SymbolNode::Ptr& rootItem  = docCache.m_symTree;
	const SymbolNode::Ptr parentItem = parentPath.getSymbolCount() == 0 ? rootItem : rootItem->findItem(parentPath);

	if (parentItem.isNull() || !childSymbol)
		return;

	//! \todo collect statistics and reorder to optimize
	if (childSymbol->isForwardClassDeclaration()
		|| childSymbol->isExtern()
		|| childSymbol->isFriend()
		|| childSymbol->isGenerated()
		|| childSymbol->isUsingNamespaceDirective()
		|| childSymbol->isUsingDeclaration()
		)
		return;

	// skip static local functions
	//    if ((!symbol->scope() || symbol->scope()->isClass())
	//        && symbol->isStatic() && symbol->isFunction())
	//        return;


	const CPlusPlus::Name *symbolName = childSymbol->name();
	if (symbolName && symbolName->isQualifiedNameId() && !childSymbol->isFunction())
		return;

	// construct child key (SymbolInformation)
	const CPlusPlus::Overview& overview = SymbolInfo::getNamePrinter();
	QString name = overview.prettyName(childSymbol->name()).trimmed();
	QString type = overview.prettyType(childSymbol->type()).trimmed();
	SymbolInfo childInfo(name, type, SymbolInfo::elementTypeFromSymbol(childSymbol));


	// add childItem to symbol tree
	SymbolNode::Ptr childItem;
	SymbolPath          childPath = parentPath;
	childPath.addChildSymbol(childInfo);
	if(childSymbol->isFunction())
	{	// if symbol is a function definition (include function body {...})
		// 1. find its complete symbol path and then construct childItem
		// 2. attach its code
		SymbolPath path;
		Function* func = childSymbol->asFunction();
		SymbolPath::findFunctionDefPath(func, context, path);

		SymbolNode::Ptr parent = rootItem;
		for (int ithSymbol = 0; ithSymbol < path.getSymbolCount()-1; ++ithSymbol)
		{
			const SymbolInfo* info = path.getSymbol(ithSymbol);
			SymbolNode::Ptr pNode = SymbolNode::addOrFindChild(parent, *info, d->m_curTimeStamp);
			pNode->setTimeStamp(d->m_curTimeStamp);
			parent = pNode;
		}
		if (path.getSymbolCount()!= 0)
		{
			childItem = SymbolNode::addOrFindChild(parent,*path.getLastSymbol(), d->m_curTimeStamp);
			childPath = path;
		}
		else
			childItem = SymbolNode::addOrFindChild(parentItem, childInfo, d->m_curTimeStamp);				// not found
 	}
	else
		childItem = SymbolNode::addOrFindChild(parentItem, childInfo, d->m_curTimeStamp);
	
	Scope* scope = childSymbol->asScope();
	if (scope && childInfo.elementType() != SymbolInfo::Block)
	{
		int beg = scope->startOffset();
		int end = scope->endOffset();
		childItem->getOrAddAttr<CodeAttr>()->setCode(curDocSource.mid(beg, end-beg));
	}

	// add child to inde
	if (childInfo.isVariable() || childInfo.isFunction())
		docCache.m_nameDict->insert(name, childPath);
	else if (childInfo.isClass() || childInfo.isEnum())
		docCache.m_typeDict->insert(name, childPath);

	// fill properties of child item
	// fill locations, locations are 1-based in Symbol, start with 0 for the editor
	SymbolLocation location(QString::fromUtf8(childSymbol->fileName() , childSymbol->fileNameLength()),
		childSymbol->line(), childSymbol->column() - 1);
	
	childItem->getOrAddAttr<LocationAttr>()->addSymbolLocation(location);

	// fill function and variable number
	nFunctions = childInfo.isFunction() ? 1 : 0;
	nVariables = childInfo.isVariable() ? 1 : 0;
	// add descendents
	// prevent showing a content of the functions
	// if (!childSymbol->isFunction()) 
	if(!(childInfo.elementType() & SymbolInfo::FunctionSignalSlot))
	{
		const CPlusPlus::Scope *scope = childSymbol->asScope();
		if (scope) 
		{
			for(CPlusPlus::Scope::iterator cur = scope->firstMember();cur != scope->lastMember();++cur) 
			{
				CPlusPlus::Symbol *curSymbol = *cur;
				if (!curSymbol)
					continue;

				int nFunc = 0, nVar = 0;
				addSymbol(docCache, childPath, curSymbol, curDocSource, context, nFunc, nVar);
				nFunctions += nFunc;
				nVariables += nVar;
			}
		}
	}
	ChildInfoAttr::Ptr pChildInfo = childItem->getOrAddAttr<ChildInfoAttr>();
	pChildInfo->childFunctions()  = nFunctions;
	pChildInfo->childVariables()  = nVariables;

	// do not add empty namespace
	// 	if (childSymbol->isNamespace() && childItem->childCount() == 0)
	// 		return;

}

/*!
Creates a flat tree from the list of projects specified by \a projectList.
*/

SymbolNode::Ptr SymbolTreeBuilder::createFlatTree(const QStringList &projectList)
{
	QReadLocker locker(&d->prjLocker);

	SymbolNode::Ptr item(new SymbolNode(d->m_curTimeStamp));
	foreach (const QString &project, projectList) {
		if (!d->cachedPrjTrees.contains(project))
			continue;
		SymbolNode::ConstPtr list = d->cachedPrjTrees[project].m_symTree;
		SymbolNode::add(list, item);
	}
	return item;
}

/*!
Parses the project with the \a projectId and adds the documents from the
\a fileList to the project. Updates the internal cached tree for this
project.
*/

SymbolNode::Ptr SymbolTreeBuilder::getParseProjectTree(const QStringList &fileList,
														const QString &projectId,
														const SymbolPath& projPath)
{
	//! \todo Way to optimize - for documentUpdate - use old cached project and subtract
	//! changed files only (old edition), and add curent editions
	CachedPrjData projCache;
	// copy old settings
	if (d->cachedPrjTrees.contains(projectId))
		projCache = d->cachedPrjTrees[projectId];
	// compute new data
	SymbolNode::Ptr projTree(new SymbolNode(d->m_curTimeStamp));

	projCache.m_symTree  = projTree;
	projCache.m_nameDict = SymbolDictPtr(new SymbolDict);
	projCache.m_typeDict = SymbolDictPtr(new SymbolDict);
	projCache.m_projPath = projPath;
	SymbolWordAttr::Ptr projWordAttr = projTree->getOrAddAttr<SymbolWordAttr>();

	unsigned revision = 0;
	foreach (const QString &file, fileList) {
		// ? locker for document?..
		const CPlusPlus::Document::Ptr &doc = d->document(file);
		if (doc.isNull())
			continue;

		revision += doc->revision();

		SymbolNode::ConstPtr docTree = getCachedOrParseDocumentTree(doc, projCache.m_isDeepParse);
		if (docTree.isNull())
			continue;

		// if project doesn't need to perform deep parse, just add the word attribute to project
		if (!projCache.m_isDeepParse)
		{
			SymbolWordAttr::Ptr pDocWordAttr = docTree->getAttr<SymbolWordAttr>();
			if (pDocWordAttr)
				projWordAttr->unite(*pDocWordAttr);
		}
		else
			SymbolNode::add(docTree, projTree);

		(*projCache.m_nameDict) += *(d->cachedDocTrees[doc->fileName()].m_nameDict);
		(*projCache.m_typeDict) += *(d->cachedDocTrees[doc->fileName()].m_typeDict);
	}
	if (!projCache.m_isDeepParse)
		projWordAttr->computeStatistics();

	// update the cache
	if (!projectId.isEmpty()) {
		QWriteLocker locker(&d->prjLocker);

		d->cachedPrjTrees[projectId]         = projCache;
		d->cachedPrjTreesRevision[projectId] = revision;
	}
	return projTree;
}

/*!
Gets the project with \a projectId from the cache if it is valid or parses
the project and adds the documents from the \a fileList to the project.
Updates the internal cached tree for this project.
*/

SymbolNode::Ptr SymbolTreeBuilder::getCachedOrParseProjectTree( const QStringList &fileList,
																const QString &projectId,
																const SymbolPath& projPath)
{
	d->prjLocker.lockForRead();

	// calculate current revision
	if (!projectId.isEmpty() && d->cachedPrjTrees.contains(projectId)) {
		// calculate project's revision
		unsigned revision = 0;
		foreach (const QString &file, fileList) {
			const CPlusPlus::Document::Ptr &doc = d->document(file);
			if (doc.isNull())
				continue;
			revision += doc->revision();
		}

		// if even revision is the same, return cached project
		if (revision == d->cachedPrjTreesRevision[projectId]) {
			d->prjLocker.unlock();
			return d->cachedPrjTrees[projectId].m_symTree;
		}
	}

	d->prjLocker.unlock();
	return getParseProjectTree(fileList, projectId, projPath);
}

/*!
Parses the document \a doc if it is in the project files and adds a tree to
the internal storage. Updates the internal cached tree for this document.

\sa parseDocument
*/

SymbolNode::ConstPtr SymbolTreeBuilder::getParseDocumentTree(const CPlusPlus::Document::Ptr &doc, bool isDeepParse)
{
	if (doc.isNull())
		return SymbolNode::ConstPtr();

	QString fileName = doc->fileName();
	if (!d->fileList.contains(fileName))
		return SymbolNode::ConstPtr();

	CachedDocData          docCache;
	SymbolNode::Ptr itemPtr(new SymbolNode(d->m_curTimeStamp));
	docCache.m_symTree  = itemPtr;
	docCache.m_nameDict = SymbolDictPtr(new SymbolDict);
	docCache.m_typeDict = SymbolDictPtr(new SymbolDict);

	// collect symbol word attribute
#ifdef USING_QT_4
	CppTools::Internal::CppModelManager* mgr = CppTools::Internal::CppModelManager::instance();
#elif defined(USING_QT_5)
	CppTools::CppModelManagerInterface *mgr = CppTools::CppModelManagerInterface::instance();
	if (!mgr)
		return SymbolNode::ConstPtr();
#endif
	const CppTools::CppModelManagerInterface::WorkingCopy workingCopy = mgr->workingCopy();
	const QByteArray unpreprocSrc = getSource(fileName, workingCopy);
	SymbolWordAttr::Ptr pWords    = itemPtr->getOrAddAttr<SymbolWordAttr>();
	pWords->clear();
	pWords->mergeWords(QString::fromUtf8(unpreprocSrc.constData(), unpreprocSrc.size()));
	pWords->computeStatistics();

	if (isDeepParse)
	{
		// add document symbols
		QByteArray preprocSrc;
		LookupContext context;
		Document::Ptr pDoc = getContext(fileName, unpreprocSrc, preprocSrc, context);
		unsigned total = pDoc->globalSymbolCount();
		for (unsigned i = 0; i < total; ++i)
		{
			int nFun, nVar;
			addSymbol(docCache, SymbolPath(), pDoc->globalSymbolAt(i), preprocSrc, context, nFun, nVar);

			// collect child information attribute
			ChildInfoAttr::Ptr pChildInfo = itemPtr->getOrAddAttr<ChildInfoAttr>();
			pChildInfo->childFunctions() += nFun;
			pChildInfo->childVariables() += nVar;
		}
		//doc->releaseSourceAndAST();
	}


	QWriteLocker locker(&d->docLocker);

	d->cachedDocTrees[fileName] = docCache;
	d->cachedDocTreesRevision[fileName] = doc->revision();
	d->documentList[fileName] = doc;

	return itemPtr;
}


Document::Ptr SymbolTreeBuilder::getContext(const QString& fileName, QByteArray& source, LookupContext& context)
{
#ifdef USING_QT_4
	CppTools::Internal::CppModelManager* mgr = CppTools::Internal::CppModelManager::instance();
	CPlusPlus::Snapshot snapshot = mgr->snapshot();
#elif defined(USING_QT_5)
	CppTools::CppModelManagerInterface *mgr = CppTools::CppModelManagerInterface::instance();
	if (!mgr)
		return CPlusPlus::Document::Ptr();
	CPlusPlus::Snapshot snapshot = mgr->snapshot();
#endif

	const CppTools::CppModelManagerInterface::WorkingCopy workingCopy = mgr->workingCopy();
	const QByteArray unpreprocessedSource = getSource(fileName, workingCopy);

	CPlusPlus::Document::Ptr doc = snapshot.preprocessedDocument(unpreprocessedSource, fileName);
	source = doc->utf8Source();

	doc->check();
	snapshot.insert(doc);
	context = LookupContext(doc, snapshot);
	return doc;
}

/*!
Gets the document \a doc from the cache or parses it if it is in the project
files and adds a tree to the internal storage.

\sa parseDocument
*/


SymbolNode::ConstPtr SymbolTreeBuilder::getCachedOrParseDocumentTree(const CPlusPlus::Document::Ptr &doc, bool isDeepParse)
{
	if (doc.isNull())
		return SymbolNode::ConstPtr();

	const QString &fileName = doc->fileName();
	d->docLocker.lockForRead();
	if (d->cachedDocTrees.contains(fileName)
		&& d->cachedDocTreesRevision.contains(fileName)
		&& d->cachedDocTreesRevision[fileName] == doc->revision()) 
	{
			d->docLocker.unlock();
			return d->cachedDocTrees[fileName].m_symTree;
	}
	d->docLocker.unlock();
	return getParseDocumentTree(doc, isDeepParse);
}

/*!
Parses the document \a doc if it is in the project files and adds a tree to
the internal storage.
*/

void SymbolTreeBuilder::updateDocumentCache(const CPlusPlus::Document::Ptr &doc)
{
	if (doc.isNull())
		return;

	const QString name = doc->fileName();

	// if it is external file (not in any of our projects)
	if (!d->fileList.contains(name))
		return;

	getParseDocumentTree(doc, true);

//	QTC_ASSERT(d->timer, return);
	
// 	bool isActive = d->timer->isActive();
// 	if (!d->timer->isActive()) 
// 		d->timer->start(400); //! Delay in msecs before an update
	return;
}

/*!
Requests to clear full internal stored data.
*/

void SymbolTreeBuilder::clearCacheAll()
{
	d->docLocker.lockForWrite();

	d->cachedDocTrees.clear();
	d->cachedDocTreesRevision.clear();
	d->documentList.clear();

	d->docLocker.unlock();

	clearCache();
}

/*!
Requests to clear internal stored data. The data has to be regenerated on
the next request.
*/

void SymbolTreeBuilder::clearCache()
{
	QWriteLocker locker(&d->prjLocker);

	// remove cached trees
	d->cachedPrjFileLists.clear();

	//! \todo where better to clear project's trees?
	//! When file is add/removed from a particular project?..
	d->cachedPrjTrees.clear();
	d->cachedPrjTreesRevision.clear();

	clearModifierCache();
}

/*!
Specifies the files that must be allowed for the parsing as a \a fileList.
Files outside of this list will not be in any tree.
*/

void SymbolTreeBuilder::setFileList(const QStringList &fileList)
{
	d->fileList.clear();
	d->fileList = QSet<QString>::fromList(fileList);
}

/*!
Removes the files defined in the \a fileList from the parsing.
*/

void SymbolTreeBuilder::removeFiles(const QStringList &fileList)
{
	if (fileList.count() == 0)
		return;

	QWriteLocker lockerPrj(&d->prjLocker);
	QWriteLocker lockerDoc(&d->docLocker);
	foreach (const QString &name, fileList) {
		d->fileList.remove(name);
		d->cachedDocTrees.remove(name);
		d->cachedDocTreesRevision.remove(name);
		d->documentList.remove(name);
		d->cachedPrjTrees.remove(name);
		d->cachedPrjFileLists.clear();
	}

	clearCacheAll();
	emit filesAreRemoved();
}

/*!
Fully resets the internal state of the code SymbolColletor to \a snapshot.
*/

void SymbolTreeBuilder::resetData(const CPlusPlus::Snapshot &snapshot)
{
	// clear internal cache
	clearCache();

	d->docLocker.lockForWrite();

	// copy snapshot's documents
	CPlusPlus::Snapshot::const_iterator cur = snapshot.begin();
	CPlusPlus::Snapshot::const_iterator end = snapshot.end();
	for (; cur != end; cur++)
		d->documentList[cur.key()] = cur.value();

	d->docLocker.unlock();

	// recalculate file list
	QStringList fileList;

	// check all projects
	foreach (const Project *prj, SessionManager::projects()) {
		if (prj)
			fileList += prj->files(Project::ExcludeGeneratedFiles);
	}
	setFileList(fileList);

//	emit resetDataDone();
	onResetDataDone();
}

/*!
Fully resets the internal state of the code SymbolColletor to the current state.

\sa resetData
*/

void SymbolTreeBuilder::resetDataToCurrentState()
{
	// get latest data
	CppTools::CppModelManagerInterface *codeModel = CppTools::CppModelManagerInterface::instance();
	if (codeModel)
		resetData(codeModel->snapshot());
}

/*!
Regenerates the tree when internal data changes.

\sa resetDataDone
*/

void SymbolTreeBuilder::onResetDataDone()
{
	// internal data is reset, update a tree and send it back
	updateTree();
}

/*!
Requests to emit a signal with the current tree state.
*/

void SymbolTreeBuilder::update()
{
	updateTree();
}

/*!
Sends the current tree to listeners.
*/

void SymbolTreeBuilder::updateTree()
{
	// stop timer if it is active right now
	//d->timer->stop();

	QTime t0 = QTime::currentTime();
	UIManager::instance()->lockAll();

	d->rootItemLocker.lockForWrite();
	d->symbolTree.setRoot(parse());
	d->rootItemLocker.unlock();

	// update dependency data
	updateDependData();
//	printDependData();

	// update tree time stamp
	d->m_treeLastUpdateTimeStamp = d->m_curTimeStamp;

	QTime t1 = QTime::currentTime();

	applyModifiers();
	UIManager::instance()->unlockAll();

	QTime t2 = QTime::currentTime();
	qDebug("tree build time: %dms, total modify time: %dms", t0.msecsTo(t1), t1.msecsTo(t2));


	// convert
//	QSharedPointer<QStandardItem> std(new QStandardItem());
//	d->rootItem->convertTo(std.data());
//	emit treeDataUpdate(std);
}

/*!
Generates a project node file list for the root node \a node.
*/

QStringList SymbolTreeBuilder::projectNodeFileList(const ProjectExplorer::FolderNode *node) const
{
	QStringList list;
	if (!node)
		return list;

	QList<ProjectExplorer::FileNode *> fileNodes = node->fileNodes();
	QList<ProjectExplorer::FolderNode *> subFolderNodes = node->subFolderNodes();

	foreach (const ProjectExplorer::FileNode *file, fileNodes) {
		//if (file->isGenerated())
		//	continue;

		list << file->path();
	}

	foreach (const ProjectExplorer::FolderNode *folder, subFolderNodes) {
		if (folder->nodeType() != ProjectExplorer::FolderNodeType
			&& folder->nodeType() != ProjectExplorer::VirtualFolderNodeType)
			continue;
		list << projectNodeFileList(folder);
	}

	return list;
}

/*!
Generates projects like the Project Explorer.
\a item specifies the item and \a node specifies the root node.

Returns a list of projects which were added to the item.
*/

QStringList SymbolTreeBuilder::addProjectNode( SymbolNode::Ptr &item,
										   const ProjectExplorer::ProjectNode *node,
										   const SymbolPath& symPath)
{
	QStringList projectList;
	if (!node)
		return projectList;

	const QString &nodePath = node->path();

	// our own files
	QStringList fileList;

	// try to improve parsing speed by internal cache
	if (d->cachedPrjFileLists.contains(nodePath)) {
		fileList = d->cachedPrjFileLists[nodePath];
	} else {
		fileList = projectNodeFileList(node);
		d->cachedPrjFileLists[nodePath] = fileList;
	}

	if (fileList.count() > 0) 
	{
		addProject(item, fileList, symPath, nodePath);
		ProjectAttr::Ptr projAttr = item->getOrAddAttr<ProjectAttr>();
		projAttr->setFileList(fileList);
		projAttr->isDeepParse() = d->cachedPrjTrees[nodePath].m_isDeepParse;
		projAttr->setProjectPath(symPath);
		projAttr->setDisplayName(node->displayName());

#ifdef USING_QT_4
		if (const Qt4PriFileNode* pQtProj = dynamic_cast<const Qt4PriFileNode*>(node))
			projAttr->setProjectType(ProjectAttr::PROJ_PRI);
		else if (const Qt4ProFileNode* pQtProj = dynamic_cast<const Qt4ProFileNode*>(node))
		{
			ProjectAttr::ProjectType projType = ProjectAttr::typeFromQt4ProjType(pQtProj->projectType());
			projAttr->setProjectType(projType);
		}
#elif defined(USING_QT_5)
		if (const QmakePriFileNode* pQtProj = dynamic_cast<const QmakePriFileNode*>(node))
			projAttr->setProjectType(ProjectAttr::PROJ_PRI);
		else if (const QmakeProFileNode* pQtProj = dynamic_cast<const QmakeProFileNode*>(node))
		{
			ProjectAttr::ProjectType projType = ProjectAttr::typeFromQt5ProjType(pQtProj->projectType());
			projAttr->setProjectType(projType);
		}
#endif
		projectList << nodePath;
	}


	// subnodes
	QList<ProjectExplorer::ProjectNode *> projectNodes = node->subProjectNodes();

	foreach (const ProjectExplorer::ProjectNode *project, projectNodes) 
	{
		SymbolNode::Ptr itemChild;

// 		qDebug() << project->path() << " hasTarget " << project->hasBuildTargets() << "type" << project->nodeType();
// 		if (const Qt4ProFileNode* pQt4Proj = dynamic_cast<const Qt4ProFileNode*>(project))
// 		{
// 			qDebug() << "project type "<< pQt4Proj->projectType();
// 		}
// 		else if(const Qt4PriFileNode* pQt4Proj = dynamic_cast<const Qt4PriFileNode*>(project))
// 			qDebug() << "others";

		SymbolInfo information(project->displayName(), project->path(), SymbolInfo::Project);
		SymbolPath childPath = symPath;
		childPath.addChildSymbol(information);

		projectList += addProjectNode(itemChild, project, childPath);

		// append child if item is not null and there is at least 1 child
		// && itemPrj->childCount() > 0
		if (item.isNull() || itemChild.isNull())
			continue; 

		SymbolTree::addEdge(itemChild, item, SymbolEdge::EDGE_PROJ_BUILD_DEPEND);
	}

	return projectList;
}
CPlusPlus::Document::Ptr SymbolTreeBuilder::SymbolColletorPrivate::document(const QString &fileName) const
{
	if (!documentList.contains(fileName))
		return CPlusPlus::Document::Ptr();
	return documentList[fileName];
}


unsigned SymbolTreeBuilder::getCurrentTimeStamp()
{
	return d->m_curTimeStamp;
}

void SymbolTreeBuilder::setCurrentTimeStamp( unsigned timeStamp )
{
	d->m_curTimeStamp = timeStamp;
}

SymbolTree& SymbolTreeBuilder::getSymbolTree()const
{
	return d->symbolTree;
}

QByteArray SymbolTreeBuilder::getSource( const QString &fileName, const CppTools::CppModelManagerInterface::WorkingCopy &workingCopy )
{
	if (workingCopy.contains(fileName)) {
		return workingCopy.source(fileName);
	} else {
		QString fileContents;
		::Utils::TextFileFormat format;
		QString error;
		QTextCodec *defaultCodec = Core::EditorManager::defaultTextCodec();
		::Utils::TextFileFormat::ReadResult result = ::Utils::TextFileFormat::readFile(
			fileName, defaultCodec, &fileContents, &format, &error);
		if (result != ::Utils::TextFileFormat::ReadSuccess)
			qWarning() << "Could not read " << fileName << ". Error: " << error;

		return fileContents.toUtf8();
	}
}

void SymbolTreeBuilder::findByType(const QString& key, QSet<SymbolPath>& pathList)
{
	pathList.clear();
	QHash<QString, CachedPrjData>::ConstIterator pProj;
	for (pProj = d->cachedPrjTrees.begin(); pProj != d->cachedPrjTrees.end(); ++pProj)
	{
		const CachedPrjData& projDat = pProj.value();
		SymbolDict::ConstIterator pItem = projDat.m_typeDict->find(key);

		for (; pItem!=projDat.m_typeDict->constEnd() && pItem.key()==key; ++pItem)
		{
			const SymbolInfo* info = pItem->getLastSymbol();
			if (info && info->matchNameOrType(key, SymbolInfo::MatchType))
			{
				SymbolPath p = projDat.m_projPath;			
				p.addChildPath(pItem.value());
				pathList.insert(p);
			}
		}					
	}
}
void SymbolTreeBuilder::updateDependData()
{
	d->dependData.m_projDepend = DependDictPtr(new DependDict);
	d->dependData.m_symbolDepend = DependDictPtr(new DependDict);

// 	d->varEdgeList.clear();
// 	d->projEdgeList.clear();

	for(QHash<QString, CachedPrjData>::ConstIterator pProj = d->cachedPrjTrees.constBegin();
		pProj != d->cachedPrjTrees.constEnd(); ++pProj)
	{
		const CachedPrjData& projData = pProj.value();
		for (SymbolDict::ConstIterator pSym = projData.m_nameDict->constBegin();
			 pSym != projData.m_nameDict->constEnd(); ++pSym)
		{
			const SymbolPath& path = *pSym;
			// get dependency dictionary
			const SymbolInfo* projInfo = projData.m_projPath.getLastProjSymbol();
			if (!projInfo)
				continue;

			const SymbolInfo* pNodeInfo = path.getLastSymbol();
			if (!pNodeInfo || !pNodeInfo->isVariable())
				continue;

			// add dependency according to variables
			// when variable of type A is defined in (class/function)B,
			// we say B depends A
			QStringList typeList;
			pNodeInfo->getNonBasicTypeElements(typeList);
			int ithDepend = 0;
			foreach(QString typeName, typeList)
			{
				// find best symbolpath of target
				QSet<SymbolPath> targetPath;
				findByType(typeName, targetPath);
				QSet<SymbolPath>::ConstIterator pBest = targetPath.constEnd();
				int bestScore = -1;
				for (QSet<SymbolPath>::ConstIterator pTarget = targetPath.constBegin();
					pTarget != targetPath.constEnd(); ++pTarget)
				{
					int score;
					if (((score = pTarget->findCommonLength(path)) > bestScore))
					{
						pBest = pTarget;
						bestScore = score;
					}
				}

				if (pBest == targetPath.constEnd())
					continue;
				SymbolPath pathWithProj = path;
				pathWithProj.addParentPath(projData.m_projPath);

				d->dependData.m_symbolDepend->insert(pathWithProj, *pBest);
				d->dependData.m_projDepend->insert(projData.m_projPath, pBest->getProjectPath());

				SymbolNode::Ptr root = d->symbolTree.getRoot();
				SymbolNode::Ptr referenceNode   = root->findItem(pathWithProj);
				SymbolNode::Ptr definitionNode  = root->findItem(*pBest);

				SymbolEdge::Ptr pEdge = SymbolTree::getOrAddEdge(definitionNode, referenceNode, SymbolEdge::EDGE_VARIABLE_REF);

				SymbolNode::Ptr refProjNode     = root->findItem(projData.m_projPath);
				SymbolNode::Ptr defProjNode		= root->findItem(pBest->getProjectPath());

				SymbolEdge::Ptr  pProjEdge = SymbolTree::getEdge(defProjNode, refProjNode, SymbolEdge::EDGE_PROJ_DEPEND);

				if (!pProjEdge)
				{
					//pProjEdge = SymbolTree::addEdge(defProjNode, refProjNode, SymbolEdge::EDGE_PROJ_DEPEND);
					//pProjEdge->strength() = 1;
				}
				else
					pProjEdge->strength()++;

// 				d->varEdgeList.push_back(pEdge);
// 				d->projEdgeList.push_back(pProjEdge);
			}
		}
	}
}

// void SymbolColletor::updateProjDependData( const SymbolPath& path, SymbolTreeNode::Ptr& pNode )
// {
// 	const SymbolInformation* pNodeInfo = path.getLastSymbol();
// 	if (!pNodeInfo)
// 		goto NODE_PROCESS_END;
// 
// 	bool isNewNode = pNode->timeStamp() > d->m_treeLastUpdateTimeStamp;
// 	// old project nodes, remain unchanged
// 	if (pNodeInfo->isProject() && !isNewNode)
// 		return;
// 	else if (pNodeInfo->isProject())
// 	{
// 		// get project needed to update
// 		QString projKey = pNodeInfo->type();
// 		QHash<QString, CachedPrjData>::Iterator pProj = d->cachedPrjTrees.find(projKey);
// 		if (pProj == d->cachedPrjTrees.end())
// 			goto NODE_PROCESS_END;
// 
// 		// clear old dependency data
// 		pProj.value().m_dependDict = DependDictPtr(new DependDict);
// 	}
// 	else if (pNodeInfo->isVariable())
// 	{
// 		// get dependency dictionary
// 		const SymbolInformation* projInfo = path.getLastProjSymbol();
// 		if (!projInfo)
// 			goto NODE_PROCESS_END;
// 		QString projKey = projInfo->type();
// 		QHash<QString, CachedPrjData>::Iterator pProj = d->cachedPrjTrees.find(projKey);
// 		if (pProj == d->cachedPrjTrees.end())
// 			goto NODE_PROCESS_END;
// 
// 		DependDict& dict = *(pProj.value().m_dependDict);
// 
// 		if (isNewNode || 1)
// 		{ 
// 			// add dependency according to variables
// 			// when variable of type A is defined in (class/function)B,
// 			// we say B depends A
// 			QStringList typeList;
// 			pNodeInfo->getNonBasicTypeElements(typeList);
// 			foreach(QString typeName, typeList)
// 			{
// 				// find best symbolpath of target
// 				QSet<SymbolPath> targetPath;
// 				findByType(typeName, targetPath);
// 				QSet<SymbolPath>::ConstIterator pBest = targetPath.constEnd();
// 				int bestScore = -1;
// 				for (QSet<SymbolPath>::ConstIterator pTarget = targetPath.constBegin();
// 					pTarget != targetPath.constEnd(); ++pTarget)
// 				{
// 					int score;
// 					if (((score = pTarget->findCommonLength(path)) > bestScore))
// 					{
// 						pBest = pTarget;
// 						bestScore = score;
// 					}
// 				}
// 
// 				if (pBest == targetPath.constEnd())
// 					continue;
// 				//pNode->addTypePath(*pBest);
// 				dict.insert(path, *pBest);
// 			}
// 		}
// 
// 		// add all item to dependDict
// // 		for (QSet<SymbolPath>::ConstIterator pPath = pNode->getTypePathSet().constBegin();
// // 			 pPath != pNode->getTypePathSet().constEnd(); ++pPath)
// // 		{
// // 			dict.insert(path, *pPath);
// // 		}
// 	}
// 
// NODE_PROCESS_END:
// 	for (SymbolTreeNode::ChildIterator pChild = pNode->childBegin();
// 		pChild != pNode->childEnd(); ++pChild)
// 	{
// 		SymbolPath childPath = path;
// 		childPath.addChildSymbol(pChild.key());
// 		updateProjDependData(childPath, pChild.value());
// 	}
// }

void SymbolTreeBuilder::printDependData() const
{
	qDebug() << "\n-------------- project dependency data ---------------";
	for (DependDict::ConstIterator pDepend = d->dependData.m_projDepend->constBegin();
		pDepend != d->dependData.m_projDepend->constEnd(); ++pDepend)
	{
		qDebug()	<< "\t"        << pDepend.key().getLastSymbol()->name()
					<< "\t\t-> " << pDepend.value().getLastSymbol()->name();				     
	}

// 	for (int i = 0; i < d->projEdgeList.size(); ++i)
// 	{
// 		SymbolInfo srcInfo =  d->projEdgeList[i]->srcNode().toStrongRef()->getSymInfo();
// 		SymbolInfo tarInfo =  d->projEdgeList[i]->tarNode().toStrongRef()->getSymInfo();
// 		qDebug()    << "\t"			<< srcInfo.name()
// 					<< "\t\t-> "	<< tarInfo.name() 
// 					<< "   "		<< d->projEdgeList[i]->dependStrength();
// 	}

	QSet<QString> depStr, edgeStr;
	qDebug() << "\n-------------- symbol dependency data ---------------";
	for (DependDict::ConstIterator pDepend = d->dependData.m_symbolDepend->constBegin();
		pDepend != d->dependData.m_symbolDepend->constEnd(); ++pDepend)
	{
		qDebug()	<< "\t"        << pDepend.key().getLastSymbol()->name()
					<< "\t\t-> " << pDepend.value().getLastSymbol()->name();

		depStr << (pDepend.value().getLastSymbol()->name() + " " + pDepend.key().getLastSymbol()->name());
	} 

// 	qDebug() << "\n\n\n";
// 	for (int i = 0; i < d->varEdgeList.size(); ++i)
// 	{
// 		SymbolInfo srcInfo =  d->varEdgeList[i]->srcNode().toStrongRef()->getSymInfo();
// 		SymbolInfo tarInfo =  d->varEdgeList[i]->tarNode().toStrongRef()->getSymInfo();
// 		qDebug()    << "\t"        << srcInfo.name()
// 			<< "\t\t-> " << tarInfo.name();
// 
// 		edgeStr << (srcInfo.name() + " " + tarInfo.name());
// 	}
}



void SymbolTreeBuilder::addModifier( const SymbolModifier::Ptr& modifier )
{
	d->modifierList.push_back(modifier);
}


void SymbolTreeBuilder::clearModifierCache()
{
	for (int ithModifier = 0; ithModifier < d->modifierList.size(); ++ithModifier)
	{
		d->modifierList[ithModifier]->clearCache();
	}
}

void SymbolTreeBuilder::applyModifiers()
{
	// modify tree
	for (int ithModifier = 0; ithModifier < d->modifierList.size(); ++ithModifier)
	{
		QTime t0 = QTime::currentTime();
		qDebug("################ begin apply modifier ################");
		d->modifierList[ithModifier]->prepare(d->symbolTree);
		d->modifierList[ithModifier]->modifyTree();
		d->modifierList[ithModifier]->updateDirtyData();
		QTime t1 = QTime::currentTime();
		qDebug("modify time: %d", t0.msecsTo(t1));
		qDebug("################  end apply modifier  ################");
	}
}

Document::Ptr SymbolTreeBuilder::getContext( const QString& fileName, const QByteArray& unpreprocessedSource, QByteArray& source, LookupContext& context )
{
#ifdef USING_QT_4
	CppTools::Internal::CppModelManager* mgr = CppTools::Internal::CppModelManager::instance();
	CPlusPlus::Snapshot snapshot = mgr->snapshot();
#elif defined(USING_QT_5)
	CppTools::CppModelManagerInterface *codeModel = CppTools::CppModelManagerInterface::instance();
	if (!codeModel)
		return CPlusPlus::Document::Ptr();
	CPlusPlus::Snapshot snapshot = codeModel->snapshot();
#endif

	CPlusPlus::Document::Ptr doc = snapshot.preprocessedDocument(unpreprocessedSource, fileName);
	source = doc->utf8Source();

	doc->check();
	snapshot.insert(doc);
	context = LookupContext(doc, snapshot);
	return doc;
}

QList<SymbolNode::Ptr> CodeAtlas::SymbolTreeBuilder::findProjNodeInDoc(QString& fileName)const
{
	QStringList projNameList;
	QList<SymbolNode::Ptr> projNodeList;
	for (QHash<QString, QStringList>::const_iterator pProj = d->cachedPrjFileLists.constBegin();
		pProj != d->cachedPrjFileLists.constEnd(); ++pProj)
	{
		if (pProj.value().contains(fileName))
		{
			projNameList << pProj.key();
			break;
		}
	}

	foreach(QString projName, projNameList)
	{
		QHash<QString, CachedPrjData>::const_iterator pProj = d->cachedPrjTrees.find(projName);
		if (pProj == d->cachedPrjTrees.constEnd())
			continue;
		projNodeList << pProj->m_symTree;
	}
	return projNodeList;
}
SymbolNode::Ptr CodeAtlas::SymbolTreeBuilder::findNodeInDoc( QString& fileName, CPlusPlus::Symbol* symbol )const
{
	if (fileName.length() == 0 && !symbol)
		return SymbolNode::Ptr();

	QList<SymbolNode::Ptr> projList = findProjNodeInDoc(fileName);
	if (projList.isEmpty())
		return SymbolNode::Ptr();

	SymbolNode::Ptr projRoot = projList[0];
	SymbolLocation location(QString::fromUtf8(symbol->fileName() , symbol->fileNameLength()),
		symbol->line(), symbol->column() - 1);

	// find matched symbol with max tree level
	int maxDepth = -1;
	SymbolNode::Ptr res;
	SmartDepthIterator iter(projRoot, DepthIterator::PREORDER, SymbolInfo::All & ~SymbolInfo::Project & ~SymbolInfo::Block, SymbolInfo::All & ~SymbolInfo::Block);
	for (SymbolNode::Ptr node; node = *iter; ++iter)
	{
		LocationAttr::Ptr loc = node->getAttr<LocationAttr>();
		if (!loc || iter.currentTreeLevel() < maxDepth)
			continue;

		if (loc->getSymbolLocations().contains(location))
			return node;
	}
	return SymbolNode::Ptr();
}
