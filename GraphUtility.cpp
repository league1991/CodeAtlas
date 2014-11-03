#include "StdAfx.h"
#include "GraphUtility.h"
using namespace CodeAtlas;
GraphUtility::GraphUtility(void)
{
}

GraphUtility::~GraphUtility(void)
{
}

bool CodeAtlas::GraphUtility::computePairDist( const Eigen::MatrixXf& adjMat, Eigen::MatrixXf& distMat )
{
	int r = adjMat.rows();
	int c = adjMat.cols();
	if (r <= 0 || c <= 0 || r != c)
		return false;

	distMat = adjMat;
	int n   = adjMat.rows();

	// floyd algorithm
	for (int k = 0; k < n; ++k)
	{
		for (int i = 0; i < n; ++i)
		{
			for (int j = 0; j < n; ++j)
			{
				float Dik = distMat(i,k);
				float Dkj = distMat(k,j);
				float Dij = distMat(i,j);
				if (Dik == -1 || Dkj == -1)
					continue;
				if (Dij == -1)
				{
					distMat(i,j) = Dik + Dkj;
					continue;
				}
				if (Dik + Dkj < Dij)
				{
					distMat(i,j) = Dik + Dkj;
					continue;
				}
			}
		}
	}

	return true;
}

bool CodeAtlas::GraphUtility::computePairDist( const SparseMatrix& vtxEdgeMatrix, 
											  Eigen::MatrixXf& distMat,
											  const VectorXf* edgeLength)
{
	MatrixXf adjMat;
	incidence2AdjMat(vtxEdgeMatrix, adjMat, edgeLength);
	return computePairDist(adjMat, distMat);
}

bool CodeAtlas::GraphUtility::findConnectedComponents( const Eigen::SparseMatrix<double>& laplacian, 
													  Eigen::VectorXi& compVec, 
													  int& nComp )
{
	int nVtx = laplacian.rows();
	if (nVtx <= 0 || nVtx != laplacian.cols())
		return false;

	compVec.resize(nVtx);
	if (nVtx == 1)
	{
		compVec[0] = 0;
		nComp = 1;
		return false;
	}

	// fill idx array, used as a waiting list
	struct Idx{	Idx *prev, *next; };
	Idx* idxBuffer = new Idx[nVtx+2];
	Idx* idxHeader = idxBuffer;
	Idx* idxArray  = idxBuffer+1;
	for (int i=0; i< nVtx; ++i)
	{
		idxArray[i].prev = idxArray + i-1;
		idxArray[i].next = idxArray + i+1;
		compVec[i] = -1;
	}
	idxHeader->next = idxArray;
	idxArray[nVtx-1].next = NULL;

	int ithComp = 0;
	int* vtxQueue = new int[nVtx+1];
	// iterate until there is no waiting vtx
	while (idxHeader->next != NULL)
	{
		// fetch first unused node as root
		int rootIdx = idxHeader->next - idxArray;
		compVec[rootIdx] = ithComp;

		// find all vtx in a component
		// breath first traversal
		int front = 0, end = 1;
		vtxQueue[front] = rootIdx;
		while (front != end)
		{
			int frontVtx = vtxQueue[front++];

			// remove front vtx from waiting list
			idxArray[frontVtx].prev->next = idxArray[frontVtx].next;
			if (idxArray[frontVtx].next)
				idxArray[frontVtx].next->prev = idxArray[frontVtx].prev;

			// find neighbors of front and push new ones into queue
			for (SparseMatrix::InnerIterator it(laplacian, frontVtx); it; ++it)
			{
				int vtx = it.row();
				if (vtx == frontVtx)
					continue;

				if (it.value() != 0.f && compVec[vtx] == -1)
				{
					vtxQueue[end++] = vtx;
					compVec[vtx] = ithComp;
				}
			}
		}

		ithComp++;
	}
	delete[] idxBuffer;
	delete[] vtxQueue;
	nComp = ithComp;
	return true;
}

bool CodeAtlas::GraphUtility::findConnectedComponentsVE( const Eigen::SparseMatrix<double>& vtxEdgeMatrix, Eigen::VectorXi& compVec, int& nComp )
{
	SparseMatrix laplacian = vtxEdgeMatrix * vtxEdgeMatrix.transpose();
	return findConnectedComponents(laplacian, compVec, nComp);
}

void CodeAtlas::GraphUtility::splitConnectedComponents(const SparseMatrix& vtxEdgeMatrix, 
													   VectorXi& vtxCompIdx, 
													   VectorXi& vtxIdx,
													   VectorXi& edgeCompIdx,
													   VectorXi& edgeIdx,
													   QList<SparseMatrix>& compArray)
{
	int nVtx = vtxEdgeMatrix.rows();
	int nEdge= vtxEdgeMatrix.cols();
	if (nVtx <= 0 || nEdge <= 0)
		return;

	int nComp;
	SparseMatrix laplacian = vtxEdgeMatrix * vtxEdgeMatrix.transpose();
	findConnectedComponents(laplacian, vtxCompIdx, nComp);

	// all the vertices are connected to each other
	compArray.clear();
	vtxIdx.resize(nVtx);
	edgeIdx.resize(nEdge);
	edgeCompIdx.resize(nEdge);
	if (nComp == 1)
	{
		compArray.push_back(vtxEdgeMatrix);
		for (int i = 0; i < nVtx; ++i)
		{
			vtxCompIdx[i] = 0;
			vtxIdx[i] = i;
		}
		for (int i = 0; i < nEdge; ++i)
		{
			edgeCompIdx[i] = 0;
			edgeIdx[i] = i;
		}
		return;
	}

	std::vector<Component> compInfo;
	compInfo.resize(nComp);
	for (int i = 0; i < nVtx; ++i)
	{
		int ithComp = vtxCompIdx[i];
		vtxIdx[i] = compInfo[ithComp].m_nVtx++;
	}

	for (int ithEdge = 0; ithEdge < vtxEdgeMatrix.outerSize(); ++ithEdge)
	{
		SparseMatrix::InnerIterator it(vtxEdgeMatrix,ithEdge);
		int vtxOldIdx[2];
		double edgeTag[2];

		vtxOldIdx[0]	= it.row();		
		edgeTag[0]      = it.value();
		++it;
		vtxOldIdx[1]    = it.row();
		edgeTag[1]		= it.value();

		int ithComp     = vtxCompIdx[vtxOldIdx[0]];

		Component& component = compInfo[ithComp];

		component.m_triVec.push_back(Triplet(vtxIdx[vtxOldIdx[0]], component.m_nEdge, edgeTag[0]));
		component.m_triVec.push_back(Triplet(vtxIdx[vtxOldIdx[1]], component.m_nEdge, edgeTag[1]));
		edgeIdx[ithEdge] = component.m_nEdge;
		edgeCompIdx[ithEdge]= ithComp;
		component.m_nEdge++;
	}

	for (int ithComp = 0; ithComp < nComp; ++ithComp)
	{
		Component& comp = compInfo[ithComp];

		if (comp.m_nEdge != 0)
		{
			compArray.push_back(SparseMatrix(comp.m_nVtx, comp.m_nEdge));
			SparseMatrix& mat = compArray[ithComp];

			mat.reserve(VectorXi::Constant(comp.m_nEdge,2));
			std::vector<Triplet>& triVec = comp.m_triVec;
			mat.setFromTriplets(triVec.begin(), triVec.end());
			mat.makeCompressed();
		}
		else
			compArray.push_back(SparseMatrix());
	}
}

bool CodeAtlas::GraphUtility::incidence2AdjMat( const SparseMatrix& vtxEdgeMatrix, MatrixXf& adjMat, const VectorXf* edgeLength )
{
	int nVtx = vtxEdgeMatrix.rows();
	if (nVtx <= 0)
		return false;

	adjMat.setConstant(nVtx, nVtx, -1);
	for (int ithEdge = 0; ithEdge < vtxEdgeMatrix.cols(); ++ithEdge)
	{
		SparseMatrix::InnerIterator it(vtxEdgeMatrix,ithEdge);

		int srcVtx = it.row();		
		++it;
		int tarVtx = it.row();

		float l = edgeLength ? (*edgeLength)[ithEdge] : 1.f;
		if (adjMat(srcVtx, tarVtx) == -1)
			adjMat(srcVtx, tarVtx) = adjMat(tarVtx, srcVtx) = l;
		else
		{
			float minL = qMin(adjMat(srcVtx, tarVtx), l);
			adjMat(srcVtx, tarVtx) = minL;
			adjMat(tarVtx, srcVtx) = minL;
		}
	}
	return true;
}

bool CodeAtlas::GraphUtility::incidence2VtxArray( const SparseMatrix& vtxEdgeMatrix,
											      QVector<Vertex>& vtxArray)
{
	int nVtx = vtxEdgeMatrix.rows();
	if (nVtx == 0)
		return false;

	vtxArray.clear();
	vtxArray.resize(nVtx);
	for (int ithEdge = 0; ithEdge < vtxEdgeMatrix.outerSize(); ++ithEdge)
	{
		SparseMatrix::InnerIterator it(vtxEdgeMatrix,ithEdge);

		int vtxIdx[2], edgeTag[2];
		vtxIdx[0]		= it.row();		
		edgeTag[0]      = it.value();
		++it;
		vtxIdx[1]       = it.row();
		edgeTag[1]		= it.value();

		int srcVtx      = edgeTag[0] == 1 ? vtxIdx[0] : vtxIdx[1];
		int tarVtx		= edgeTag[1] == -1? vtxIdx[1] : vtxIdx[0];

		vtxArray[srcVtx].m_outVtx.push_back(tarVtx);
		vtxArray[tarVtx].m_inVtx.push_back(srcVtx);
	}
	return true;
}

bool CodeAtlas::GraphUtility::computeVtxLevel( const SparseMatrix& vtxEdgeMatrix, VectorXi& levelVec, int baseLevel /*= 1*/ )
{
	int nVtx = vtxEdgeMatrix.rows();
	int nEdge= vtxEdgeMatrix.cols();
	if (nVtx == 0)
		return false;

	QVector<Vertex> vtxArray;
	if (!incidence2VtxArray(vtxEdgeMatrix, vtxArray))
		return false;

	levelVec = VectorXi::Constant(nVtx, baseLevel);
	QVector<bool> isEnqueued(nVtx, false);
	QQueue<int>  vtxQueue;
	vtxQueue.reserve(nVtx);
	int nVtxLeft = nVtx;

	// find nodes without in edges
	for (int ithVtx = 0; ithVtx < nVtx; ++ithVtx)
	{
		if (vtxArray[ithVtx].m_inVtx.size() == 0)
		{
			vtxQueue.enqueue(ithVtx);
			levelVec[ithVtx] = baseLevel;
			isEnqueued[ithVtx] = true;
			nVtxLeft--;
		}
	}

	while (1)
	{
		// resolve loop dependency, find unknown vtx with least in edges
		if (vtxQueue.size() == 0)
		{
			if (nVtxLeft <= 0)
				break;
			int minInEdge = nEdge+1;
			int minVtxID  = -1;
			for (int ithVtx = 1; ithVtx < nVtx; ++ithVtx)
			{
				if (isEnqueued[ithVtx])
					continue;
				int nInEdge = vtxArray[ithVtx].m_inVtx.size();
				if (nInEdge < minInEdge)
				{
					minInEdge = nInEdge;
					minVtxID  = ithVtx;
				}
			}
			vtxQueue.enqueue(minVtxID);
			isEnqueued[minVtxID] = true;
			nVtxLeft--;
		}
		int curIdx = vtxQueue.dequeue();

		Vertex& vtx  = vtxArray[curIdx];
		int curLevel = levelVec[curIdx];

		// collect from out vertices
		for (int ithOut = 0; ithOut < vtx.m_outVtx.size(); ++ithOut)
		{
			int outIdx= vtx.m_outVtx[ithOut];
			// skip processed vtx
			if (isEnqueued[outIdx])
				continue;

			// skip vtx with unprocessed in vtx
			Vertex& outVtx = vtxArray[outIdx];
			bool    isAllInEnqueued = true;
			for (int ithIn = 0; ithIn < outVtx.m_inVtx.size(); ++ithIn)
			{
				int inIdx = outVtx.m_inVtx[ithIn];
				if (!isEnqueued[inIdx])
				{
					isAllInEnqueued = false;
					break;
				}
			}
			levelVec[outIdx] = qMax(curLevel+1,levelVec[outIdx]);
			if (isAllInEnqueued)
			{
				vtxQueue.enqueue(outIdx);
				isEnqueued[outIdx] = true;
				nVtxLeft--;
			}
		}
	}
	return true;
}

bool CodeAtlas::GraphUtility::incidence2Laplacian( const SparseMatrix& vtxEdgeMatrix, const VectorXf& edgeWeightVec, MatrixXd& laplacian )
{
	int nVtx = vtxEdgeMatrix.rows();
	int nEdge= vtxEdgeMatrix.cols();
	if (nVtx <= 0 || nEdge <= 0 || edgeWeightVec.size() != nEdge)
		return false;

	laplacian = MatrixXd::Zero(nVtx, nVtx);
	for (int ithEdge = 0; ithEdge < vtxEdgeMatrix.outerSize(); ++ithEdge)
	{
		SparseMatrix::InnerIterator it(vtxEdgeMatrix,ithEdge);
		double wij = edgeWeightVec[ithEdge];

		int vtxI		= it.row();	
		++it;
		int vtxJ        = it.row();

		laplacian(vtxI,vtxJ) -= wij;
		laplacian(vtxJ,vtxI) -= wij;
		laplacian(vtxI,vtxI) += wij;
		laplacian(vtxJ,vtxJ) += wij;
	}
	return true;
}

bool CodeAtlas::GraphUtility::getVtxFromEdge( const SparseMatrix& vtxEdgeMatrix, const int ithEdge, int& srcVtx, int& tarVtx )
{
	SparseMatrix::InnerIterator it(vtxEdgeMatrix,ithEdge);

	int vtxIdx[2];
	double edgeTag[2];
	vtxIdx[0]		= it.row();		
	edgeTag[0]      = it.value();
	++it;
	vtxIdx[1]       = it.row();
	edgeTag[1]		= it.value();

	srcVtx      = edgeTag[0] == s_srcTag ? vtxIdx[0] : vtxIdx[1];
	tarVtx		= edgeTag[1] == s_tarTag? vtxIdx[1] : vtxIdx[0];
	return true;
}

bool CodeAtlas::GraphUtility::getSubGraph( const SparseMatrix& vtxEdgeMatrix, const bool *const isVtxValid, SparseMatrix& subGraphVEMat )
{
	std::vector<Triplet>	triVec;
	int ithValidEdge = 0;
	int nNodes = vtxEdgeMatrix.rows();
	int* vtxMap = new int[nNodes];
	int ithValidNode = 0;
	for (int i = 0; i < nNodes; ++i)
	{
		if (isVtxValid[i])
			vtxMap[i] = ithValidNode++;
	}

	for (int ithEdge = 0; ithEdge < vtxEdgeMatrix.cols(); ++ithEdge)
	{
		int src, tar;
		getVtxFromEdge(vtxEdgeMatrix, ithEdge, src, tar);
		if (isVtxValid[src] && isVtxValid[tar])
		{
			triVec.push_back(Triplet(vtxMap[src], ithValidEdge, s_srcTag));
			triVec.push_back(Triplet(vtxMap[tar], ithValidEdge, s_tarTag));
			ithValidEdge++;
		}
	}
	subGraphVEMat.resize(ithValidNode, ithValidEdge);
	if (ithValidEdge == 0)
		return false;

	delete[] vtxMap;

	subGraphVEMat.reserve(VectorXi::Constant(ithValidEdge,2));
	subGraphVEMat.setFromTriplets(triVec.begin(), triVec.end());
	subGraphVEMat.makeCompressed();
	return true;
}

bool CodeAtlas::GraphUtility::getSpanningTree( const SparseMatrix& vtxEdgeMatrix, const VectorXf& edgeLength, int* parentArray, int* parentEdgeLength /*= NULL*/, bool isMinTree /*= true*/, int rootIdx /*= 0*/ )
{
	int nVtx   = vtxEdgeMatrix.rows();
	if (nVtx <= 0)
		return false;

	// initialize parent array
	vector<int> setParentArray(nVtx);
	vector<int> subTreeSize(nVtx);
	for (int i = 0; i < nVtx; ++i)
	{
		parentArray[i] = i;
		setParentArray[i] = i;
		subTreeSize[i] = 1;
	}
	if (parentEdgeLength)
		for (int i = 0; i < nVtx; ++i)
			parentEdgeLength[i] = 0;

	int nEdge  = vtxEdgeMatrix.cols();
	if (nEdge <= 0)
		return true;

	// extract edge array
	vector<Edge> edgeArray;
	edgeArray.reserve(nEdge);
	for (int ithEdge = 0; ithEdge < nEdge; ++ithEdge)
	{
		int srcVtx, tarVtx;
		getVtxFromEdge(vtxEdgeMatrix, ithEdge, srcVtx, tarVtx);
		edgeArray.push_back(Edge(srcVtx, tarVtx, edgeLength[ithEdge]));
	}

	sort(edgeArray.begin(), edgeArray.end());

	// determine traversal order
	int begEdge = isMinTree ? 0 : nEdge-1;
	int endEdge = isMinTree ? nEdge : -1;
	int incrEdge = isMinTree ? 1 : -1;

	struct Node
	{
		vector<int> m_adjID;
		vector<float>m_adjLength;
		int			m_tag;
	};
	vector<Node> graph(nVtx);

	// find minimum/maximum spanning tree
	for (int ithEdge = begEdge; ithEdge != endEdge; ithEdge += incrEdge)
	{
		Edge& edge = edgeArray[ithEdge];
		int srcRoot = findRoot(&setParentArray[0], edge.m_src);
		int tarRoot = findRoot(&setParentArray[0], edge.m_tar);

		// if src and tar belong to the same connected component, ignore this edge
		if (srcRoot == tarRoot)
			continue;

		if (subTreeSize[srcRoot] > subTreeSize[tarRoot])
		{
			setParentArray[tarRoot] = srcRoot;
			subTreeSize[srcRoot] = subTreeSize[srcRoot] + subTreeSize[tarRoot];
		}
		else
		{
			setParentArray[srcRoot] = tarRoot;
			subTreeSize[tarRoot] = subTreeSize[tarRoot] + subTreeSize[srcRoot];
		}

		graph[edge.m_src].m_adjID.push_back(edge.m_tar);
		graph[edge.m_tar].m_adjID.push_back(edge.m_src);

		if (parentEdgeLength)
		{
			graph[edge.m_src].m_adjLength.push_back(edge.m_weight);
			graph[edge.m_tar].m_adjLength.push_back(edge.m_weight);
		}
	}

	// BFS traversal
	for (int i = 0; i < graph.size(); ++i)
		graph[i].m_tag = 0;

	queue<int> nodeQueue;
	nodeQueue.push(rootIdx);
	graph[rootIdx].m_tag = 1;
	while (!nodeQueue.empty())
	{
		int nodeIdx = nodeQueue.front();
		nodeQueue.pop();
		Node& node = graph[nodeIdx];

		// find neighbour
		for (int ithAdj = 0; ithAdj < node.m_adjID.size(); ++ithAdj)
		{
			int adjNodeID = node.m_adjID[ithAdj];
			Node& adjNode = graph[adjNodeID];
			if (adjNode.m_tag == 1)
				continue;

			// record parent data
			adjNode.m_tag = 1;
			nodeQueue.push(adjNodeID);
			parentArray[adjNodeID] = nodeIdx;
			if (parentEdgeLength)
				parentEdgeLength[adjNodeID] = node.m_adjLength[ithAdj];
		}
	}
	return true;
}

bool CodeAtlas::GraphUtility::computePairDistInTree( const SparseMatrix& vtxEdgeMatrix, Eigen::MatrixXf& distMat, const VectorXf* pEdgeWeight /*= NULL*/, bool isMaxTree /*= true*/ )
{
	// check and initialize distMat
	int nVtx   = vtxEdgeMatrix.rows();
	if (nVtx <= 0)
		return false;

	distMat.setConstant(nVtx, nVtx, FLT_MAX);
	for (int i = 0; i < nVtx; ++i)
		distMat(i,i) = 0;

	int nEdge  = vtxEdgeMatrix.cols();
	if (nEdge <= 0)
		return true;

	// initialize edge weight
	VectorXf edgeWeight;
	if (pEdgeWeight && pEdgeWeight->size() == nEdge)
	{
		edgeWeight = *pEdgeWeight;	// negate the edge weights 
		if (isMaxTree)
			edgeWeight *= -1;
	}
	else
		edgeWeight.setOnes(nEdge);

	// build graph data structure
	struct Node
	{
		Node():m_tag(0){}
		vector<int> m_adjID;
		vector<float>m_adjLength;
		int			m_tag;
	};
	vector<Node> nodeArray(nVtx);
	for (int ithEdge = 0; ithEdge != nEdge; ithEdge++)
	{
		int src, tar;
		getVtxFromEdge(vtxEdgeMatrix, ithEdge, src, tar);
		nodeArray[src].m_adjID.push_back(tar);
		nodeArray[tar].m_adjID.push_back(src);

		nodeArray[src].m_adjLength.push_back(edgeWeight[ithEdge]);
		nodeArray[tar].m_adjLength.push_back(edgeWeight[ithEdge]);
	}

	vector<int> nodeSet;
	nodeSet.reserve(nVtx);
	priority_queue<Edge> edgeQueue;

	// push root node
	const int rootIdx = 0;
	nodeSet.push_back(rootIdx);
	Node& root = nodeArray[rootIdx];
	root.m_tag = 1;
	for (int i = 0; i < root.m_adjID.size(); ++i)
		edgeQueue.push(Edge(rootIdx, root.m_adjID[i], root.m_adjLength[i]));

	while (!edgeQueue.empty())
	{
		Edge curEdge = edgeQueue.top();
		edgeQueue.pop();

		// ignore unnecessary edges
		Node& srcNode = nodeArray[curEdge.m_src];
		Node& tarNode = nodeArray[curEdge.m_tar];
		if (srcNode.m_tag == 1 && tarNode.m_tag == 1)
			continue;

		// find new node and compute its distance to all nodes in nodeSet
		int   newID    = srcNode.m_tag == 1 ? curEdge.m_tar : curEdge.m_src;
		int   parentID = srcNode.m_tag == 1 ? curEdge.m_src : curEdge.m_tar;
		for (int i = 0; i < nodeSet.size(); ++i)
		{
			int nodeID = nodeSet[i];
			// There is only one path from any node in nodeSet to the new node,
			// so the distance is the length of that path.
			float dist = distMat(nodeID, parentID) + 1;
			distMat(nodeID, newID) = distMat(newID, nodeID) = dist; 
		}

		// add new node to nodeSet
		Node& newNode = nodeArray[newID];
		newNode.m_tag = 1;
		nodeSet.push_back(newID);

		// add its neighbour edges
		for (int ithAdj = 0; ithAdj < newNode.m_adjID.size(); ++ithAdj)
		{
			int adjID = newNode.m_adjID[ithAdj];
			if (nodeArray[adjID].m_tag != 0)
				continue;
			edgeQueue.push(Edge(newID, adjID, newNode.m_adjLength[ithAdj]));
		}
	}
	return true;
}
