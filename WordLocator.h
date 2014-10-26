#pragma once

namespace CodeAtlas{
// this class computes docs' 2D locations based on their vocabulary
class WordLocator
{
	typedef Eigen::SparseMatrix<double> SparseMatrix;
public:
	WordLocator(void);
	~WordLocator(void);

	void                  setTopicCount(int nTopic){m_topicCount = nTopic;}
	void                  setUseTfIdfMeasure(const bool isUse){m_useTfIdfMeasure = isUse;}
	void                  setDocTermMat(const SparseMatrix& docTermMat, 
		                                const Eigen::VectorXd& docRadius);

	bool                  compute(float sparseFactor = 4.f);

	const QVector<QVector2D>& getOri2DPositions(){return m_position2D;}
	QRectF                getOri2DCoordRange(){return m_coordRange;}
	float                 getOri2DRadius(){return m_radius;}

	void                  getNormalized2DPositions(QVector<QVector2D>& pos, float avgDist, float* radius = NULL);
	void                  saveMatsToFile( const QString& fileName );

	static void           compute2DDist(const QVector<QVector2D>& pos, float& avgDist, float& minDist, QVector2D& centroid);
	static void           movePoints(QVector<QVector2D>& pos, const QVector2D& offset);
	static void			  compute2DStatistics(const QVector<QVector2D>& position, QRectF& coordRange, float& radius);

private:
	void                  computeTfIdf();
	void                  computeLSI();
	void				  compute2DPos(float sparseFactor);

	bool                  m_useTfIdfMeasure;	// If set, the doc-term matrix will be transformed in advance according to tf-idf measure
	int                   m_topicCount;			// number of target topics, which is the dimension of the matrix after LSI

	SparseMatrix          m_docTermCountMat;    // each row a doc, column a term
	SparseMatrix          m_docTermMat;			// computed from docTermCountMat, each row a doc, column a term
	Eigen::MatrixXd       m_docTermMatLSI;		// processed m_docTermMat
	Eigen::MatrixXd       m_docTopicMatLSI;		// document-topic matrix, each row a doc, column a topic

	Eigen::VectorXd       m_docRadius;			// radius of each doc point
	QVector<QVector2D>    m_position2D;			// 2D position of each doc
	QRectF                m_coordRange;			// bounding rectangle
	float                 m_radius;		
};
}
