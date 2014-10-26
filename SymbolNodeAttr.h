#pragma once
#include "symbolpath.h"

namespace CodeAtlas
{
	class SymbolLocation;
	class SymbolNode;
	class WordExtractor;
	class SymbolData;
	class UIItem;
	class NodeUIItem;

#ifdef USING_QT_4
	using namespace Qt4ProjectManager;
#endif
#ifdef USING_QT_5
	using namespace QmakeProjectManager;
#endif


	// base attribute class
	// a custom attribute must derived from SymNodeAttr, then implement the following interface:
	// 1. constructor		set its own AttrType
	// 2. copy()			copy attribute
	// 3. unite()			currently only used in SymbolCollector
	// 4. substract()		currently not used
	// 5. creator()			create a new object and return its QSharedPointer address

	// Here are all its derived class:
	// [class name]		[built location]
	// CodeAttr			SymbolTreeBuilder
	// LocationAttr		SymbolTreeBuilder
	// ChildInfoAttr	SymbolTreeBuilder
	// ProjectAttr		FuzzyDependBuilder
	// SymbolWordAttr	WordCollector
	// FuzzyDependAttr  FuzzyDependBuilder
	// UIElementAttr	UIElementLocator
	// GlobalSymAttr    FuzzyDependBuilder
	class SymbolNodeAttr
	{
	public:
		typedef QSharedPointer<SymbolNodeAttr> Ptr;
		enum AttrType
		{
			ATTR_CODE		= (0x1),
			ATTR_LOCATION	= (0x1) << 1,
			ATTR_CHILDINFO  = (0x1) << 2,
			ATTR_PROJECT	= (0x1) << 3,
			ATTR_SYMWORD	= (0x1) << 4,
			ATTR_FUZZYDEPEND= (0x1) << 5,
			ATTR_UIELEMENT  = (0x1) << 6,
			ATTR_GLOBAL_SYM = (0x1) << 7,

			ATTR_NONE		= 0,
			ATTR_ALL		= 0xffffffff
		};

		SymbolNodeAttr(AttrType type):m_type(type){}
		virtual ~SymbolNodeAttr(void){}
		AttrType             type()const{return m_type;}

		// virtual functions
		virtual SymbolNodeAttr&		copy(const SymbolNodeAttr& s) = 0;
		virtual SymbolNodeAttr&     unite(const SymbolNodeAttr& s){return *this;}
		virtual SymbolNodeAttr&     substract(const SymbolNodeAttr& s){return *this;}
		virtual QString             toString()const{return QString("");}

		SymbolNodeAttr&				operator=(const SymbolNodeAttr& s);
		SymbolNodeAttr::Ptr			duplicate()const;
		bool						isSameType(const SymbolNodeAttr& s)const;

		template<class T> static T* castTo(const SymbolNodeAttr& s)
		{
			if (s.m_type == T::classType())
				return (T*)&s;
			return NULL;
		}
	protected:
		virtual SymbolNodeAttr::Ptr	creator()const = 0;

		AttrType             m_type;
	};

	// built in SymbolTreeBuilder
	class CodeAttr: public SymbolNodeAttr
	{
	public:
		typedef QSharedPointer<CodeAttr> Ptr;
		CodeAttr():SymbolNodeAttr(SymbolNodeAttr::ATTR_CODE){}
		CodeAttr(const QString& code):SymbolNodeAttr(SymbolNodeAttr::ATTR_CODE), m_code(code){}
		inline static AttrType     classType(){return ATTR_CODE;}

		void				setCode(const QString& code){m_code = code;}
		const QString&		getCode(){return m_code;}

		// virtual functions
		SymbolNodeAttr&        copy(const SymbolNodeAttr& s);
		SymbolNodeAttr&		unite(const SymbolNodeAttr& s);
		SymbolNodeAttr&		substract(const SymbolNodeAttr& s);
		QString             toString()const{return m_code;}
	protected:
		SymbolNodeAttr::Ptr    creator()const{return Ptr(new CodeAttr);}
	private:
		QString              m_code;					//   code of this symbol, only function code is added currently
	};

	// built in SymbolTreeBuilder
	class LocationAttr:public SymbolNodeAttr
	{ 
	public:
		typedef QSharedPointer<LocationAttr> Ptr;
		LocationAttr();
		LocationAttr(const QSet<SymbolLocation>& l);
		inline static AttrType     classType(){return ATTR_LOCATION;}

		void addSymbolLocation(const SymbolLocation &location);
		void addSymbolLocation(const QSet<SymbolLocation> &locations);
		void removeSymbolLocation(const SymbolLocation &location);
		void removeSymbolLocations(const QSet<SymbolLocation> &locations);
		const QSet<SymbolLocation>& getSymbolLocations() const;

		// virtual functions
		SymbolNodeAttr&     copy(const SymbolNodeAttr& s);
		SymbolNodeAttr&		unite(const SymbolNodeAttr& s);
		SymbolNodeAttr&		substract(const SymbolNodeAttr& s);
	protected:
		SymbolNodeAttr::Ptr    creator()const{return Ptr(new LocationAttr);}
	private:
		QSet<SymbolLocation> symbolLocations;			//! symbol locations
	};

	// built in FuzzyDependBuilder
	class GlobalSymAttr:public SymbolNodeAttr
	{
	public:
		typedef QSharedPointer<GlobalSymAttr> Ptr;
		GlobalSymAttr():SymbolNodeAttr(ATTR_GLOBAL_SYM){}
		inline static AttrType classType(){return ATTR_GLOBAL_SYM;}
		SymbolNodeAttr&     copy(const SymbolNodeAttr& s){return *this;}
	protected:
		SymbolNodeAttr::Ptr creator()const{return Ptr(new GlobalSymAttr);}
	};

	// built in SymbolTreeBuilder
	class ChildInfoAttr:public SymbolNodeAttr
	{
	public:
		typedef QSharedPointer<ChildInfoAttr> Ptr;
		ChildInfoAttr():SymbolNodeAttr(ATTR_CHILDINFO), m_nChildFunctions(0), m_nChildVariables(0){}
		inline static  AttrType		 classType(){return ATTR_CHILDINFO;}

		// child information
		unsigned&                childFunctions(){return m_nChildFunctions;}
		unsigned&                childVariables(){return m_nChildVariables;}
		const unsigned&          childFunctions()const{return m_nChildFunctions;}
		const unsigned&          childVariables()const{return m_nChildVariables;}

		// virtual functions
		SymbolNodeAttr&        copy(const SymbolNodeAttr& s);
		SymbolNodeAttr&		unite(const SymbolNodeAttr& s);
		SymbolNodeAttr&		substract(const SymbolNodeAttr& s);
	private:
		SymbolNodeAttr::Ptr    creator()const{return Ptr(new ChildInfoAttr);}
		unsigned             m_nChildFunctions;			//   number of functions of this node, 0 when this node is a variable
		unsigned             m_nChildVariables;			//   number of variables of this node, 0 when this node is a variable
	};

	// built in FuzzyDependBuilder
	class ProjectAttr:public SymbolNodeAttr
	{
	public:
		enum ProjectType
		{
			PROJ_UNKNOWN	= (0x1 << 0),
			PROJ_PRI		= (0x1 << 1),
			PROJ_PRO_APP	= (0x1 << 2),
			PROJ_PRO_LIB	= (0x1 << 3),
			PROJ_PRO_SUBDIR	= (0x1 << 4),
			PROJ_PRO_SCRIPT = (0x1 << 5),
			PROJ_PRO_AUX	= (0x1 << 6),

			PROJ_GENERATE_BIN	= PROJ_PRO_APP | PROJ_PRO_LIB,
			PROJ_PRO			= PROJ_PRO_APP | PROJ_PRO_LIB | PROJ_PRO_SUBDIR | PROJ_PRO_SCRIPT | PROJ_PRO_AUX
		};
		typedef QSharedPointer<ProjectAttr> Ptr;
		ProjectAttr():SymbolNodeAttr(SymbolNodeAttr::ATTR_PROJECT), m_projType(PROJ_UNKNOWN), m_isDeepParse(false){}
		inline static AttrType	classType(){return SymbolNodeAttr::ATTR_PROJECT;}

		// set global symbols
		void					setGlobalSymList(const QList<QSharedPointer<SymbolNode>>& symList);
		const QList<QSharedPointer<SymbolNode>>& getGlobalSymList(){return m_globalSymbolList;}

		void					setFileList(const QStringList& fileList){m_fileNameList = fileList;}
		SymbolNodeAttr&			copy(const SymbolNodeAttr& s);
		bool&					isDeepParse(){return m_isDeepParse;}

		// project type
		ProjectType				getProjectType()const{return m_projType;}
		void					setProjectType(ProjectType type){m_projType = type;}

		void					setProjectPath(const SymbolPath& path);
		const SymbolPath&		getProjectPath()const{return m_projPath;}

		void					setDisplayName(const QString& name){m_displayName = name;}
		const QString&			getDisplayName()const{return m_displayName;}

#ifdef USING_QT_4
		static ProjectType		typeFromQt4ProjType(Qt4ProjectType type);
#endif
#ifdef USING_QT_5
		static ProjectType		typeFromQt5ProjType(QmakeProjectType type);
#endif

	private:
		SymbolNodeAttr::Ptr        creator()const;

		QString								m_displayName;
		SymbolPath							m_projPath;
		ProjectType							m_projType;
		QList<QSharedPointer<SymbolNode>>	m_globalSymbolList;
		QStringList							m_fileNameList;
		bool								m_isDeepParse;
	};

	// built in WordCollector
	class SymbolWordAttr: public SymbolNodeAttr
	{
	public:
		typedef QSharedPointer<SymbolWordAttr> Ptr;
		SymbolWordAttr():SymbolNodeAttr(SymbolNodeAttr::ATTR_SYMWORD), m_totalWeight(0.f){}
		inline static AttrType  classType(){return SymbolNodeAttr::ATTR_SYMWORD;}

		void					clear();
		void					setWords(const QSharedPointer<SymbolNode>& node);
		void					mergeWords( const SymbolData& item );
		void					mergeWords( const SymbolWordAttr& item);
		void					mergeWords( const Document::Ptr& pDoc);
		void					mergeWords( const QString& src);


		void					computeStatistics();

		SymbolNodeAttr&			copy(const SymbolNodeAttr& s);
		SymbolNodeAttr&			unite(const SymbolNodeAttr& s);

		QString					toString()const;

		inline const QMap<int,float>&  getWordWeightMap()const{return m_wordWeightMap;}
		inline float			getTotalWordWeight(){return m_totalWeight;}
		int						nWords()const{return m_wordWeightMap.size();}

		// static functions
		static void				clearGlobalWordMap();
		// get the index from the global word pool, if not exist, and one and return the new idx
		static int				addOrGetWord(const QString& word);
		static SymbolData		getSymbolData( const QSharedPointer<SymbolNode>& node);
		static inline const QString&	getWord(int idx) {	return m_wordList[idx];	}
		static inline int				totalWords() { return m_wordList.size();}

		// cosine value of two word frequency vectors
		double					cosSimilarity(const SymbolWordAttr& other)const;

	private:
		SymbolNodeAttr::Ptr	    creator()const{return Ptr(new SymbolWordAttr);}
		void					mergeWords(const QStringList& list);
		// int-> wordIdx, float->weight
		QMap<int,float>				    m_wordWeightMap;
		float							m_totalWeight;

		static QHash<QString, int>		m_wordIdxMap;
		static QList<QString>			m_wordList;
	};

	// built in FuzzyDependBuilder
	class FuzzyDependAttr:public SymbolNodeAttr
	{
	public:
		struct DependPair
		{
			DependPair(	const QWeakPointer<SymbolNode>& def, 
						const QWeakPointer<SymbolNode>& ref, 
						float edgeWeight = 1.f):
						m_defNode(def), m_refNode(ref), m_edgeWeight(edgeWeight){}

			QWeakPointer<SymbolNode>	m_defNode, m_refNode;
			float						m_edgeWeight;
		};

		typedef Eigen::SparseMatrix<double>     SparseMatrix;
		typedef QSharedPointer<FuzzyDependAttr> Ptr;
		typedef QList<DependPair>				DependPairList;

		FuzzyDependAttr():SymbolNodeAttr(SymbolNodeAttr::ATTR_FUZZYDEPEND){}
		inline static AttrType	classType(){return SymbolNodeAttr::ATTR_FUZZYDEPEND;}

		SymbolNodeAttr&			copy(const SymbolNodeAttr& s);
		SymbolNodeAttr&			unite(const SymbolNodeAttr& s);

		// get vertex-edge matrix, each row a vtx, column a edge, 
		// the srcVtx marked to 1, and the tarVtx marked to -1
		SparseMatrix&			vtxEdgeMatrix(){return m_childDependMatrix;}
		Eigen::VectorXd&		edgeWeightVector(){return m_childEdgeWeight;}
		Eigen::VectorXi&        levelVector(){return m_childLevel;}
		DependPairList&			dependPairList(){return m_childDependPair;}

		QString					toString()const;
	private:
		SymbolNodeAttr::Ptr        creator()const{return FuzzyDependAttr::Ptr(new FuzzyDependAttr);}
		Eigen::SparseMatrix<double>  m_childDependMatrix;
		Eigen::VectorXd				 m_childEdgeWeight;
		Eigen::VectorXi				 m_childLevel;
		DependPairList				 m_childDependPair;
	};

	class UIElementAttr:public SymbolNodeAttr
	{
	public:
		typedef QSharedPointer<UIElementAttr> Ptr;

		UIElementAttr():SymbolNodeAttr(SymbolNodeAttr::ATTR_UIELEMENT), m_radius(0.f), m_level(1.f){}
		inline static AttrType	classType(){return SymbolNodeAttr::ATTR_UIELEMENT;}

		SymbolNodeAttr&						copy(const SymbolNodeAttr& s);

		float&								radius(){return m_radius;}
		QPointF&							position(){return m_position;}
		const QWeakPointer<SymbolNode>&		getNode()const{return m_node;}
		float&								level(){return m_level;}
		bool								setVisualHull(const QVector<QPointF>& convexHull);
		QSharedPointer<QPolygonF>			getVisualHull();

		void								resetUIItem(const QSharedPointer<SymbolNode>& thisNode, NodeUIItem* parent);
		void								buildOrUpdateUIItem( const QSharedPointer<SymbolNode>& thisNode, NodeUIItem * parent = NULL);
		const QSharedPointer<NodeUIItem>&   getUIItem()const{return m_uiItem;}

		QString								toString()const;

		void								setEntryInfo(const QVector<QPointF>& inEntries, const QVector<QPointF>& outEntries,
														 const QVector<QPointF>& inNormals, const QVector<QPointF>& outNormals);

		const QVector<QPointF>&				getInEntries(){return m_inEntries;}
		const QVector<QPointF>&				getOutEntries(){return m_outEntries;}
		const QVector<QPointF>&				getInNormals(){return m_inNormals;}
		const QVector<QPointF>&				getOutNormals(){return m_outNormals;}
	private:
		SymbolNodeAttr::Ptr creator()const{return UIElementAttr::Ptr(new UIElementAttr);}

		QWeakPointer<SymbolNode>		m_node;
		QSharedPointer<NodeUIItem>		m_uiItem;
		QPointF							m_position;
		QVector<QPointF>				m_inEntries, m_outEntries;
		QVector<QPointF>				m_inNormals, m_outNormals;

		float							m_radius;
		float							m_level;
		QSharedPointer<QPolygonF>		m_localHull;
	};
}