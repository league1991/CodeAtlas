#pragma once

namespace CodeAtlas
{
	class SymbolTreeBuilder : public QObject
	{
		Q_OBJECT
	public:
		typedef QMultiHash<SymbolPath, SymbolPath>	DependDict;
		typedef QSharedPointer<DependDict>			DependDictPtr; 
		struct DependencyData
		{
			DependDictPtr							m_symbolDepend;		// symbol dependency data  (symbol S -> Type T)
			DependDictPtr							m_projDepend;		// project dependency data (project P1 -> project P2)
		};

		explicit SymbolTreeBuilder(QObject *parent = 0);
		~SymbolTreeBuilder();

		bool canFetchMore(QStandardItem *item) const;
		void fetchMore(QStandardItem *item, bool skipRoot = false) const;

		// time stamp
		unsigned getCurrentTimeStamp();
		void     setCurrentTimeStamp(unsigned timeStamp);

		// get parse result
		SymbolTree&           getSymbolTree()const;
		const DependencyData& getDependencyData()const{return d->dependData;}

		// external look up
		void findByType(const QString& key, QSet<SymbolPath>& pathList);

		// symbol modifier
		// the first added modidier will be called first
		void addModifier(const SymbolModifier::Ptr& modifier);
		void deleteAllModidier(){d->modifierList.clear();}

		SymbolNode::Ptr			findNodeInDoc(QString& fileName, CPlusPlus::Symbol* symbol)const;
		QList<SymbolNode::Ptr>	findProjNodeInDoc(QString& fileName)const;
	signals:
		//! File list is changed
		void filesAreRemoved();

//		void treeDataUpdate(QSharedPointer<QStandardItem> result);

		void resetDataDone();

	public slots:
		void clearCacheAll();
		void clearCache();

		void update();

		void setFileList(const QStringList &fileList);
		void removeFiles(const QStringList &fileList);

		void resetData(const CPlusPlus::Snapshot &snapshot);
		void resetDataToCurrentState();

		void updateDocumentCache(const CPlusPlus::Document::Ptr &doc);

		void setFlatMode(bool flat);

	protected slots:
		void onResetDataDone();

	private:
		// SymbolPath in Dict is not complete, project part is ignored.
		typedef QMultiHash<QString, SymbolPath>		SymbolDict;					
		typedef QSharedPointer<SymbolDict>			SymbolDictPtr;

		struct CachedDocData
		{
			SymbolNode::Ptr							m_symTree;
			SymbolDictPtr							m_nameDict;			// used to cache name (variable/function)
			SymbolDictPtr                           m_typeDict;			// used to cache type name (class/enum/struct)
		};
		struct CachedPrjData
		{
			CachedPrjData(): m_isDeepParse(true){}
			bool									m_isDeepParse;		// parse classes in the project
			SymbolNode::Ptr							m_symTree;
			SymbolPath                              m_projPath;
			SymbolDictPtr							m_nameDict;			// used to cache name (variable/function)
			SymbolDictPtr                           m_typeDict;			// used to cache type name (class/enum/struct)
		};

		// functions listed below are used to update trees	
		void				 updateTree();		// update symbol tree
		SymbolNode::Ptr		 parse();

		SymbolNode::ConstPtr getParseDocumentTree(const CPlusPlus::Document::Ptr &doc, bool isDeepParse);
		SymbolNode::ConstPtr getCachedOrParseDocumentTree(const CPlusPlus::Document::Ptr &doc, bool isDeepParse);
		SymbolNode::Ptr		 getParseProjectTree(const QStringList &fileList, const QString &projectId, const SymbolPath& path);
		SymbolNode::Ptr      getCachedOrParseProjectTree(const QStringList &fileList, const QString &projectId, const SymbolPath& path);

		QStringList          addProjectNode(    SymbolNode::Ptr &item,
												const ProjectExplorer::ProjectNode *node,
												const SymbolPath& path);

		void					 addProject(SymbolNode::Ptr& item, 
											const QStringList& fileList,
											const SymbolPath&  projPath,
											const QString&     projectId = QString());

		void					 addSymbol(	CachedDocData& docCache,
											const SymbolPath& parentPath, 
											CPlusPlus::Symbol *childSymbol, 
											const QByteArray& curDocSource, const LookupContext& context, 
											int& nFunctions, int& nVariables);

		// modifier
		void applyModifiers();
		void clearModifierCache();

		// update dependency data of updated subtree, called after updateTree
		void                     updateDependData();
		void                     printDependData()const;

		SymbolNode::ConstPtr	 findItemByRoot(const QStandardItem *item, bool skipRoot = false) const;
		QStringList				 projectNodeFileList(const ProjectExplorer::FolderNode *node) const;
		SymbolNode::Ptr		     createFlatTree(const QStringList &projectList);

		static QByteArray        getSource(const QString &fileName,const CppTools::CppModelManagerInterface::WorkingCopy &workingCopy);
		static Document::Ptr     getContext(const QString& fileName, QByteArray& source, LookupContext& context);
		static Document::Ptr	 getContext(const QString& fileName, 
											const QByteArray& unpreprocessedSource, 
											QByteArray& preprocessedSource, 
											LookupContext& context);
		class SymbolColletorPrivate
		{
		public:
			//! Constructor
			SymbolColletorPrivate() : flatMode(false), m_curTimeStamp(0), m_treeLastUpdateTimeStamp(0) {}

			//! Get document from documentList
			CPlusPlus::Document::Ptr document(const QString &fileName) const;

			//! timer
			//QPointer<QTimer> timer;

			//! Current document list
			QHash<QString, CPlusPlus::Document::Ptr> documentList;

			//! revision used to speed up computations
			QHash<QString, unsigned> cachedDocTreesRevision;			//! Parsed documents' revision			
			QHash<QString, unsigned> cachedPrjTreesRevision;			//! Parsed projects' revision

			//! cache used to speed up computations
			QHash<QString, CachedDocData> cachedDocTrees;				//! Parsed documents 
			QHash<QString, CachedPrjData> cachedPrjTrees;				//! Merged trees for projects. Not const - projects might be substracted/added
			QHash<QString, QStringList>   cachedPrjFileLists;			//! Cached file lists for projects (non-flat mode)

			//! dependency data
			DependencyData dependData;									//! dependency data of projects and classes

// 			QList<VariableRefEdge::Ptr> varEdgeList;
// 			QList<ProjDependEdge::Ptr>  projEdgeList;
			//! write lock
			QReadWriteLock prjLocker;									//! Projects read write lock
			QReadWriteLock docLocker;									//! Documents read write lock
			QReadWriteLock rootItemLocker;								//! Root item read write lock

			// other
			QSet<QString> fileList;										//! List for files which has to be parsed

			//! Parsed root item
			//SymbolNode::Ptr rootItem;
			SymbolTree    symbolTree;

			QList<SymbolModifier::Ptr> modifierList;

			//! Flat mode
			bool flatMode;

			// time stamp used for other classes to keep synchronized with DB
			unsigned m_curTimeStamp;				// current time stamp, set externally
			unsigned m_treeLastUpdateTimeStamp;		// time stamp set after update() is invoked,
													// used to trace new nodes since last update().
		};
		//! Private class data pointer
		SymbolColletorPrivate *d;
	};


}