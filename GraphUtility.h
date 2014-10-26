#pragma once
#include <QPainterPathStroker>
using namespace Eigen;
namespace CodeAtlas{
class GraphUtility
{
public:
	typedef Eigen::SparseMatrix<double> SparseMatrix;
	typedef Eigen::Triplet<double>		Triplet;
	GraphUtility(void);
	~GraphUtility(void);

	struct Vertex
	{
		QVector<int>	m_inVtx;		// vertices that has edges pointing to this vtx
		QVector<int>	m_outVtx;		// vertices that this vtx has edges pointing to
	};

	// find connected componets
	// laplacian matrix  (nVtx, nVtx)	laplacian matrix of vertices, if vi, vj is not adjacent, laplacian(vi, vj) == 0
	// treated as undirected graph
	static bool findConnectedComponents(const Eigen::SparseMatrix<double>& laplacian, 
										Eigen::VectorXi& compVec,
										int& nComp);

	static bool findConnectedComponentsVE(const Eigen::SparseMatrix<double>& vtxEdgeMatrix, 
										Eigen::VectorXi& compVec,
										int& nComp);

	// split connected components
	// vtxEdgeMatrix	(nVtx, nEdge)	vertex-edge incidence matrix
	// vtxCompIdx		(1, nVtx)		new component idx of each vertex
	// vtxIdx			(1, nVtx)		new index in the new incidence matrix of each vertex
	// edgeCompIdx		(1, nVtx)		new component idx of each edge
	// edgeIdx			(1, nVtx)		new index in the new incidence matrix of each edge
	// compArray						component array, each entry is the vertex-edge incidence matrix of a component
	//									for isolated vertex, the matrix is empty
	static void splitConnectedComponents(	const SparseMatrix& vtxEdgeMatrix, 
											VectorXi& vtxCompIdx, 
											VectorXi& vtxIdx, 
											VectorXi& edgeCompIdx, 
											VectorXi& edgeIdx, 
											QList<SparseMatrix>& compArray);

	// compute pair distance by floyd-warshall algorithm
	// adjMat		adjecency matrix, adjMat(i,j) is the distance between vi and vj,
	//              if i,j is not adjacent, adjMat(i,j) = -1
	// distMat      matrix storing shortest path lengths between point pairs, if i,j is not connected, distMat(i,j) = -1
	static bool computePairDist(	const Eigen::MatrixXf& adjMat, Eigen::MatrixXf& distMat);

	// convert vertex-edge incidence matrix to adjacency matrix
	// adjMat		if vi,vj is different and adjacent, adjMat(i,j) = edge length, adjMat(i,i) = 0, 
	//              otherwise -1
	// edgeLength   specify length of each edge, if set to NULL, all is 1
	static bool incidence2AdjMat(	const SparseMatrix& vtxEdgeMatrix, 
									MatrixXf& adjMat,
									const VectorXf* edgeLength = NULL);

	// convert vertex-edge incidence matrix to laplacian matrix, not consider the direction of every edge
	// if two edges connecting same nodes but have different directions, they are merged to a edge with sum of both weights
	// laplacian	laplacian matrix
	// edgeLength   specify length of each edge, used as weight
	static bool incidence2Laplacian(const SparseMatrix& vtxEdgeMatrix,
									const VectorXf&     edgeWeightVec,
									MatrixXd& laplacian);

	static bool computeVtxLevel(	const SparseMatrix& vtxEdgeMatrix,
									VectorXi& levelVec,
									int baseLevel = s_defaultBaseLevel);

	// compute pair dist based on vertex-edge incidence matrix
	static bool computePairDist(	const SparseMatrix& vtxEdgeMatrix, Eigen::MatrixXf& distMat, const VectorXf* edgeLength = NULL);
	static bool incidence2VtxArray( const SparseMatrix& vtxEdgeMatrix, QVector<Vertex>& vtxArray);


	static bool getVtxFromEdge(		const SparseMatrix& vtxEdgeMatrix, const int ithEdge, 
									int& srcVtx, int& tarVtx);

	static bool getSubGraph( const SparseMatrix& vtxEdgeMatrix, const bool *const isVtxValid, SparseMatrix& subGraphVEMat);
	static const int			s_defaultBaseLevel	= 1;
	static const int			s_srcTag			= 1;
	static const int			s_tarTag			= -1;
private:
	struct Component 
	{
		Component():m_nVtx(0), m_nEdge(0){}
		std::vector<Triplet>	m_triVec;
		int						m_nVtx;
		int						m_nEdge;
	};

};


}