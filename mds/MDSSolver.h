#pragma once

namespace CodeAtlas{
class MDSSolver
{
public:
	typedef Eigen::VectorXd VectorXd;
	typedef Eigen::MatrixXd MatrixXd;
	enum DistanceMeasure{MDS_EUCLIDEAN, MDS_COS, MDS_PEARSON};
	struct MDSData 
	{
		QVector2D       m_init2DPos;
		Eigen::VectorXd m_dataVec;		// feature vector in high dimemsion space
		double           m_radius;		// size of per data, used to prevent points to be too close
//		double           m_alignVal;		// the value helps to align the final configuration, must be positive
	};

	MDSSolver():m_useExternalInitPos(false), 
		        m_usePostProcess(true), 
				m_useExternalDisimilarityMat(false),
				m_sparseFactor(4.f){}

	// set or add data points
	void setData(const QList<MDSData>& dataArray){m_dataArray = dataArray;}
	void addData(const MDSData& data){m_dataArray.push_back(data);}

	void setDissimilarityMeasure(DistanceMeasure dist){m_oriDistType = dist;}
	void setUseExternalInitPos(bool isUse){m_useExternalInitPos = isUse;}
	void setUsePostProcess(bool isUse){m_usePostProcess = isUse;}
	// use external dissimilarity matrix, should be called after setData or all of addData
	bool setExternalDissimilarityMat(const Eigen::MatrixXd& distMat);

	const QVector<QVector2D>& get2DPosition()const{return m_position2D;}

	// compute
	virtual void compute()=0;

	void print2DPosition();
	void save2DPosToFile(const QString& fileName);

	void setSparseFactor(double sparseFactor){m_sparseFactor = sparseFactor;}

	// compute a value used to align mds result
	static double getAlignVal(const QString& str);
protected:
	void               computeDissimilarityMat();
	inline double       computeDissimilarity(int ithData, int jthData);
	void               postProcess();

	Eigen::MatrixXd    m_D;					// high dimensional pair distance
	QList<MDSData>     m_dataArray;			// data points
	QVector<QVector2D> m_position2D;		// position in 2D
	DistanceMeasure    m_oriDistType;

	bool               m_useExternalDisimilarityMat;	// use external disimilarity matrix
	bool               m_useExternalInitPos;			// use init 2D pos set externally
	bool               m_usePostProcess;				// use post processer to regularize data
	bool               m_useExternalAlignment;			// use external value to help align the final configuration, because the solution of MDS problem is not unique

	double              m_sparseFactor;		// factor to control sparsity
};

class ClassicalMDSSolver:public MDSSolver
{
public:
	void			compute();
	// D		 (nData, nData) dissimilarity matrix
	// resultMat (nData, nDim)	low-dimensional position
	static void		compute( const Eigen::MatrixXd& D, Eigen::MatrixXd& resultMat, int nDim = 2 );
private:
};

class StressMDSSolver:public MDSSolver
{
public:
	StressMDSSolver():MDSSolver(){m_useExternalInitPos = false;}
	void compute();
private:
	// compute init 2D pos by classical MDS
	void computeInitPos(const Eigen::MatrixXd& D, Eigen::MatrixXd& res);
};
class HitMDSSolver:public MDSSolver
{
public:
	HitMDSSolver(void);
	~HitMDSSolver(void);

	// compute MDS
	void               compute();
private:
	// preprocess function, compute some useful data
	void               preprocess();
	void               computeDMat();
	void               computeD2DMat();

	void               init2DPoint();
	void               update2DPoint(int ithPoint, const QVector2D& newPos);// deprecated, DO NOT use

	QVector2D          compute2DPointGrad(int ithPoint);				// deprecated, DO NOT use
	Eigen::MatrixXd	   compute2DPointApproxGrad();


	Eigen::MatrixXd    m_deltaD;			// high dimensional delta distance, (= m_D - m_meanD)
	double              m_meanD;             // mean distance
	double              m_deltaDSqSum;		// = sqrt(¦² deltaD(i,j)^2)
	int                m_nNonZero;			// number of non-zero distance in D

	Eigen::MatrixXd    m_D2D;				// 2D pair distance
	Eigen::MatrixXd    m_deltaD2D;			// 2D delta distance
	double              m_meanD2D;           // mean distance
	double              m_deltaDSqSum2D;		// = sqrt(¦² deltaD2D(i,j)^2)

	int                m_maxIter;			// max number of iteration
	double              m_stepSize;			// step size

};

class MDSPostProcesser
{
public:
	MDSPostProcesser(int maxIter = 10, 
		double sparseFactor = 6.f,
		double sizeFactor = 1.f,
		double paddingRatio = 1.0,
		double minPadding = 10):m_maxIter(maxIter), m_sparseFactor(sparseFactor), m_sizeFactor(sizeFactor), m_nData(0),m_radiusPaddingRatio(paddingRatio), m_minPadding(minPadding){}

	// set initial 2d data points
	// pos2D           initial 2d position
	// radius          radius of each point
	// alignWeight     weight used to align points (determine the rotation matrix), should use features that seldom change         
	void               set2DPos(const QVector<QVector2D>& pos2D, 
								const Eigen::VectorXd& radius,
								const Eigen::VectorXi* hashVec = NULL);

	// pos2D (nVtx,2)  initial 2d position
	void			   set2DPos(const Eigen::MatrixXd& pos2D, 
								const Eigen::VectorXd& radius,
								const Eigen::VectorXi* hashVec = NULL);

	void               compute();
	void               getFinalPos(QVector<QVector2D>& pos2D);
	void			   getFinalPos( Eigen::MatrixXd& pos2D );
	void			   getFinalBoundingRect(Eigen::VectorXd& minV, Eigen::VectorXd& maxV);
	double			   getFinalRadius();

	void               setSparseFactor(double factor){m_sparseFactor = factor;}
	void			   setSizeFactor(double factor) {m_sizeFactor = factor;}
private:
	// initialize 2D Pnt
	void               init2DPnt();
	double             computeAvgDist(const Eigen::MatrixXd& datPnt);
	double			   computeScaleRate( const Eigen::MatrixXd& datPnt , const Eigen::VectorXd& radiusVec);

	void               forceDirectedLayout();
	void			   normalizeResult();

	// sparseFactor = final avgDist / avgSize
	// the bigger, the sparser of final configurations
	double              m_sparseFactor;
	double			   m_radiusPaddingRatio;
	double			   m_minPadding;

	double			   m_sizeFactor;

	Eigen::MatrixXd    m_oriPos;
	Eigen::MatrixXd    m_pcaPos;
	Eigen::MatrixXd    m_resultPos;
 
	Eigen::VectorXi	   m_hashID;			// hash val for every element
	Eigen::VectorXd    m_radius;
	Eigen::VectorXd	   m_radiusWithPadding;
	Eigen::VectorXd	   m_minRect;
	Eigen::VectorXd    m_maxRect;

	int				   m_maxIter;
	int				   m_nData;
};
}