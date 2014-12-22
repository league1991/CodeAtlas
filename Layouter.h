#pragma once

#define CHECK_ERRORS_RETURN_VOID(x) {if(x & ERRORS)return;}
#define CHECK_ERRORS_RETURN_BOOL(x) {if(x & ERRORS)return false;}
#define CHECK_RETURN_BOOL(x,y) {if(x & y)return false;}
#define CHECK_RETURN_VOID(x,y) {if(x & y)return;}

#define EDGE_ROUTE_USE_BSPLINE
//#define WRITE_DELAUNAY_MESH
using namespace Eigen;
   
namespace CodeAtlas
{
	class LayoutSetting
	{
	public:
		static void		getNodeRadius(const SymbolNode::Ptr& node, float& radius);
		static void		getChildrenRadius( const QList<SymbolNode::Ptr>& childList, VectorXf& radiusVec );

		static inline float	code2Radius(const QString& str)
		{	return s_baseRadius + qSqrt(float(str.length())) * 2.0;	}

		static inline float word2Radius(const float totalWordCount)
		{	return s_baseRadius + qSqrt(totalWordCount * s_wordAvgLength) * 2.0;	}
		

		static const float s_baseRadius;
	private:
		static const float s_wordAvgLength;
	};


	class Layouter
	{
	public:	
		typedef Eigen::Triplet<double>		Triplet;
		typedef Eigen::SparseMatrix<double> SparseMatrix;
		typedef Eigen::MatrixXf				MatrixXf;
		typedef Eigen::VectorXd				VectorXd;

		enum Status
		{
			NO_ERROR_OR_WARNING	   = 0,
			// errors
			ERROR_UNKNOWN		   = (0x1),
			ERROR_PARENT_INVALID   = (0x1) << 1,
			ERROR_CHILDREN_INVALID = (0x1) << 2,
			ERROR_LAYOUT_FAILED    = (0x1) << 3,
			// warnings
			WARNING_NO_EDGE		   = (0x1) << 4,
			WARNING_NO_VISUAL_HULL = (0x1) << 5,
			WARNING_TRIVAL_LAYOUT  = (0x1) << 6,
			WARNING_INVALID_EDGE_DATA= (0x1) << 7,
			WARNING_USE_DEFAULT_EDGE_WEIGHT  = (0x1) << 8,

			ERRORS = ERROR_CHILDREN_INVALID | ERROR_PARENT_INVALID | ERROR_UNKNOWN | ERROR_LAYOUT_FAILED,
			WARNINGS = WARNING_NO_EDGE | WARNING_TRIVAL_LAYOUT | WARNING_INVALID_EDGE_DATA | WARNING_NO_VISUAL_HULL,
		};

		struct EdgeParam
		{
			EdgeParam():m_weight(1.0){}
			MatrixXf m_points;
			float    m_weight;
		};
		struct NodeParam
		{
			QVector<QPointF> m_inPnts, m_outPnts, m_inNors, m_outNors;
		};

		Layouter():m_totalRadius(LayoutSetting::s_baseRadius), m_status(NO_ERROR_OR_WARNING), m_paddingFactor(1.1f){};
		virtual ~Layouter(){}
		virtual void			clear();

		virtual void			setNodes(const QList<SymbolNode::Ptr>& nodeList, const SymbolNode::Ptr& parent);
		virtual bool			compute(){return false;}

		const MatrixXf&			getNodePos()const{return m_nodePos;}
		const VectorXf&			getNodeRadius()const{return m_nodeRadius;}
		float					getTotalRadius(bool withPadding = true)const;
		const QVector<EdgeParam>& getEdgeParams()const{return m_edgeParam;}
		const QVector<QPointF>& getVisualHull()const{return m_visualHull;}

		virtual void			writeResults();

		unsigned				getStatus(){return m_status;}

		// layout computation kernel
		static void			mds(const MatrixXd& distMat,
			const VectorXf& radiusVec,
			const VectorXi& hashID,
			MatrixXf& finalPos2D,
			float&    finalRadius,
			float     sparseFactor = 1.f,
			float  paddingRatio = 0.1f,
			float  minPadding  = LayoutSetting::s_baseRadius * 10);

		static void			laplacianEigMap(
			const MatrixXd& laplacian,
			const VectorXf& radiusVec,
			const VectorXi& hashID,
			MatrixXf& finalPos2D,
			float&    finalRadius,
			float     sparseFactor = 1.3f);


	protected:
		void	trivalLayout();
		static bool	graphLayout(const SparseMatrix& veMat, 
							const VectorXf& radiusVec,
							MatrixXf& finalPos2D,
							float&    finalRadius,
							float     sparseFactor = 1.f);
		bool    computeEdgeRoute(DelaunayCore::DelaunayRouter router);
		bool	computeVisualHull(float padding);

		static QPainterPath    mat2Path(const MatrixXf& pntMat);
		static QPolygonF		getRect(const QPointF& center, float radius);

		unsigned				m_status;			

		// input nodes
		QList<SymbolNode::Ptr>  m_childList;		// nodes to layout
		SymbolNode::Ptr			m_parent;			// the parent of nodes in m_nodeList

		// input attributes
		VectorXf				m_nodeRadius;

		// results
		QVector<EdgeParam>		m_edgeParam;		// ui presentation of edges
		QVector<NodeParam>		m_childParam;	
		MatrixXf				m_nodePos;
		float					m_totalRadius;
		QVector<QPointF>		m_visualHull;

		// params
		float					m_paddingFactor;
	};

	// layout helper class
	// if no enough info found, layout children using trival method
	class TrivalLayouter:public Layouter
	{
	public:
		TrivalLayouter():Layouter(){}
		bool        compute();

	private:
		bool		compute(const QList<SymbolNode::Ptr>& childList, MatrixXf& pos2D, VectorXf& radiusVec, float& radius);

		bool        computePos();
		bool		computeEdgeRoute();
		bool		computeVisualHull();

	};

}