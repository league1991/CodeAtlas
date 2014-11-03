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

	// kruskal algorithm used to find spanning tree
	// the input graph must have only one connected component
	static bool getSpanningTree(const SparseMatrix& vtxEdgeMatrix, const VectorXf& edgeLength, 
								int* parentArray, int* parentEdgeLength = NULL,
								bool isMinTree = true, int rootIdx = 0);	
	
	// first, generate a maximum spanning tree using prim algorithm
	// then compute the pair distance in that tree
	// the input graph must have only one connected component
	// vtxEdgeMatrix	(nVtx, nEdge)	vertex-edge incidence matrix
	// distMat			(nVtx, nVtx)	pair distance matrix, record the distance of nodes in the maximum spanning tree
	// pEdgeWeight		(1,nEdge)		edge weight matrix used to determine which edge should be selected in the spanning tree
	// isMaxTree						if set to true, use the maximum spanning tree, otherwise used the minimum one
	static bool computePairDistInTree(	const SparseMatrix& vtxEdgeMatrix, Eigen::MatrixXf& distMat, const VectorXf* pEdgeWeight = NULL, bool isMaxTree = true);
	
	static const int			s_defaultBaseLevel	= 1;
	static const int			s_srcTag			= 1;
	static const int			s_tarTag			= -1;
private:

	static inline int findRoot(int* parent, int node)
	{
		if (parent[node] != node)
		{
			parent[node] = findRoot(parent, parent[node]);
		}
		return parent[node];
	}
	struct Component 
	{
		Component():m_nVtx(0), m_nEdge(0){}
		std::vector<Triplet>	m_triVec;
		int						m_nVtx;
		int						m_nEdge;
	};

	struct Edge
	{
		Edge(int src, int tar, float weight):m_src(src), m_tar(tar), m_weight(weight){}
		bool operator<(const Edge& other)const{return this->m_weight < other.m_weight;}

		int m_src, m_tar;
		float m_weight;
	};
};


}