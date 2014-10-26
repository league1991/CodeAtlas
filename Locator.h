#pragma once

namespace CodeAtlas{




class Locator
{
public:
	struct SymbolPosition
	{
		SymbolPath m_path;
		QVector2D  m_position;
		float      m_radius;
		int        m_level;
	};
	struct ProjectData
	{
		// information
		QHash<SymbolPath, SymbolData>		 m_symbolData;			// word item, 2D positions, levels of symbols in this project
		int                                  m_nSymbolLevel;		// number of different symbol levels
		SymbolData							 m_projectData;

		// update flag
		unsigned                             m_timeStamp;
		bool                                 m_isDirty;				// if 2D position of m_symbolWordList need to be updated, set dirty
	};

	Locator(void);
	~Locator(void);
	void clear();
	void compute(const SymbolTreeBuilder& collector);
	void printWords()const;
	void saveMatsToFile(const QString& fileName);

	void getSymbolPositions(QList<SymbolPosition>& posList);
	const QHash<SymbolPath, ProjectData>& getProjectData(){return m_projectWordList;}

private:
	typedef Eigen::SparseMatrix<double> SparseMatrix;

	// recursive call
	// path          SymbolPath of current node
	// node          current node
	// curSymbolList list of current node and its children's vocabulary
	// depth         recursive call depth
	// return value  vocabulary of current node
	SymbolData     collectWords(const SymbolPath& path, const SymbolNode& node, 
		                        QHash<SymbolPath, SymbolData>& curSymbolList, int depth);

// 	int            addOrGetWord(const QString& word);
// 	SymbolData     getWordItem (const SymbolInformation& info, const SymbolNode::Ptr& node);

	// build matrix for dimensional reduction
	// build document-term matrix
	void		   buildDocTermMat(const QHash<SymbolPath, SymbolData>& symbolWordList, 
		                           SparseMatrix& docTermMat, Eigen::VectorXd& radiusVec);

	void           buildPathDistMat(const QHash<SymbolPath, SymbolData>& symbolWordList,
									Eigen::MatrixXd& distMat, Eigen::VectorXd& radiusVec, 
									Eigen::VectorXd& alignVec);

	void           compute2DPos();

	void		   layoutByWord( const QHash<SymbolPath, SymbolData>& symbolWord, 
		                         QVector<QVector2D>& pos2D, 
								 float avgDist, float* radius = NULL, QVector<float>* projRadius = NULL);
	void		   layoutByGraph(const QHash<SymbolPath, SymbolData>& symbolWord, 
								QVector<QVector2D>& pos2D,
								float sparseFactor, 
								float* radius = NULL,
								QVector<float>* projRadius = NULL);

	// build project dependency graph
	void           buildProjDepend(const SymbolTreeBuilder::DependencyData& dependData);
	void           computeProjLevel();
	// build symbol dependency graph in each project, ignore inter-project dependency
	void           updateProjectData(const SymbolTreeBuilder::DependencyData& dependData);
	void           buildSymbolDepend(const SymbolTreeBuilder::DependencyData& dependData);
	void           computeSymbolLevel(ProjectData& proj);

	QHash<SymbolPath, ProjectData> m_projectWordList;

// 	QHash<QString, int>   m_wordIdxMap;
// 	QList<QString>        m_wordList;
// 	WordExtractor         m_wordExtractor;	
	WordLocator           m_wordLocator;

	static const float    m_avgClassDist;
};

}