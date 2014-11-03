#include "stdafx.h"
#include "ClassLayouter.h"

using namespace CodeAtlas;
using namespace ogdf;
using namespace Eigen;
bool ClassLayouter::computeNormally()
{
	FuzzyDependAttr::Ptr fuzzyAttr = m_parent->getAttr<FuzzyDependAttr>();
	if (!fuzzyAttr) 
		return false;

	Graph G;
	GraphAttributes GA(G,
		GraphAttributes::nodeGraphics | GraphAttributes::edgeGraphics |
		GraphAttributes::nodeLabel    | GraphAttributes::nodeTemplate |
		GraphAttributes::edgeDoubleWeight);

	SparseMatrix& veMat = fuzzyAttr->vtxEdgeMatrix();
	VectorXd	  edgeWeight	= fuzzyAttr->edgeWeightVector();
	if (edgeWeight.size() != veMat.cols())
	{
		m_status |= WARNING_USE_DEFAULT_EDGE_WEIGHT;
		edgeWeight.setOnes(veMat.cols());
	}
	const int nNodes = veMat.rows();
	const int nEdges = veMat.cols();
	if (nNodes <= 0 || nNodes != m_childList.size() || nEdges < 1)
	{
		m_status |= WARNING_NO_EDGE;
		return false;
	}

	bool isUseOrthoLayout = nEdges < 50;

	vector<node> nodeArray;
	vector<edge> edgeArray;
	NodeArray<float> nodeSize(G);
	EdgeArray<double> edgeLength(G);
	for (int i = 0; i < nNodes; ++i)
	{
		nodeArray.push_back(G.newNode());
		float r = m_nodeRadius[i];
		GA.width(nodeArray.back()) = r*2;
		GA.height(nodeArray.back()) = r*2;
		nodeSize[nodeArray.back()] = r * 2;
	}

	float maxEdgeWeight = edgeWeight.maxCoeff();
	float minEdgeWeight = edgeWeight.minCoeff();
	for (int ithEdge = 0; ithEdge < nEdges; ++ithEdge)
	{
		int src, dst;
		GraphUtility::getVtxFromEdge(veMat, ithEdge, src, dst);
		edgeArray.push_back(G.newEdge(nodeArray[src], nodeArray[dst]));
		//GA.doubleWeight(edgeArray.back()) = edgeWeight[ithEdge];
		edgeLength[edgeArray.back()] = 1;//log(edgeWeight[ithEdge] + 1);
	}


	// add dummy vertices and edges in order to merge parallel edge segments attached to the same node
	VectorXi compVec;int nComp = 1;
	const float dummyNodeRadius = 0.01;
	if(m_isMergeEdgeEnd && isUseOrthoLayout && GraphUtility::findConnectedComponentsVE(	veMat, compVec, nComp ))
	{
		bool* isCompSet = new bool[nComp];
		for (int i = 0; i < nComp; ++i)
			isCompSet[i] = false;

		// add a dummy node and edge for each connect component
		for (int ithVtx = 0; ithVtx < compVec.size(); ++ithVtx)
		{
			int ithCmp = compVec[ithVtx];
			if (isCompSet[ithCmp] == false)
			{
				// add dummy node and set its radius
				nodeArray.push_back(G.newNode());							
				GA.width(nodeArray.back()) = dummyNodeRadius;
				GA.height(nodeArray.back()) = dummyNodeRadius;
				// add dummy edge
				edgeArray.push_back(G.newEdge(nodeArray[ithVtx], nodeArray.back()));
				isCompSet[ithCmp] = true;
			}
		}
		delete[] isCompSet;
	}


	MatrixXd pos;
	pos.resize(nNodes, 2);
	try
	{
		if (isUseOrthoLayout)
		{
			PlanarizationLayout layouter;
			OrthoLayout *ol = new OrthoLayout;
			float sep = max(m_nodeRadius.sum() / nNodes, LayoutSetting::s_baseRadius * 12);
			ol->separation(sep);
			ol->cOverhang(0.1);
			ol->setOptions(1+4);
			layouter.setPlanarLayouter(ol);
			layouter.call(GA);
			for (int v = 0; v < nNodes; ++v)
			{
				double x = GA.x(nodeArray[v]);
				double y = GA.y(nodeArray[v]);
				pos(v,0) = x;
				pos(v,1) = y;
			}
		}
		else
		{
			FMMMLayout layouter;
			//CircularLayout layouter;
			float avgRadius = m_nodeRadius.sum();
			layouter.unitEdgeLength(avgRadius * 4);
			//layouter.call(GA, edgeLength);
			layouter.call(GA);
			// 	layouter.resizeDrawing(true);
			// 	layouter.resizingScalar(3);

			for (int v = 0; v < nNodes; ++v)
			{
				double x = GA.x(nodeArray[v]);
				double y = GA.y(nodeArray[v]);
				pos(v,0) = x;
				pos(v,1) = y;
			}

			MDSPostProcesser m_postProcessor(5000, 0.8, 1.0, 1.0, LayoutSetting::s_baseRadius * 2);
			m_postProcessor.set2DPos(pos, m_nodeRadius.cast<double>());
			m_postProcessor.compute();
			m_postProcessor.getFinalPos(pos);
		}
	}
	catch(...)//AlgorithmFailureException e
	{
		return false;
	}

	VectorXd halfSize,offset;
	GeometryUtility::moveToOrigin(pos, m_nodeRadius.cast<double>(), halfSize, &offset);
	m_nodePos = pos.cast<float>();

	// postprocessing, and set edges
	float edgeBound[2] = {halfSize[0], halfSize[1]};
	if (isUseOrthoLayout)
	{
		for(int ithEdge = 0; ithEdge < nEdges; ++ithEdge)
		{
			DPolyline& p = GA.bends(edgeArray[ithEdge]);
			int nPoints = p.size();

			BSpline splineBuilder(5, BSpline::BSPLINE_OPEN_UNIFORM, true);
			int ithPnt = 0;
			for (DPolyline::iterator pLine = p.begin(); pLine != p.end(); ++pLine, ++ithPnt)
			{
				float px = (*pLine).m_x + offset[0];
				float py = (*pLine).m_y + offset[1];
				splineBuilder.addPoint(px,py);
				splineBuilder.addPoint(px,py);
			}
			splineBuilder.computeLine((nPoints) * 5);
			const QList<QPointF>& curve = splineBuilder.getCurvePnts();

			m_edgeParam.push_back(EdgeParam());
			EdgeParam& ep = m_edgeParam.back();
			ep.m_points.resize(curve.size(),2);
			for (int i = 0; i < curve.size(); ++i)
			{
				ep.m_points(i, 0) = curve[i].x();
				ep.m_points(i, 1) = curve[i].y();

				edgeBound[0] = qMax(edgeBound[0], (float)qAbs(curve[i].x()));
				edgeBound[1] = qMax(edgeBound[1], (float)qAbs(curve[i].y()));
			}
			ep.m_weight = edgeWeight[ithEdge];
		}
	}
	else
	{
		DelaunayCore::DelaunayRouter router;
		router.setLaneWidthRatio(2);
		router.setSmoothParam(0.5,2);
		router.setEndPointNormalRatio(0.8);
		computeEdgeRoute(router);
	}

	m_totalRadius = qSqrt(edgeBound[0]*edgeBound[0] + edgeBound[1]*edgeBound[1]);
	return true;
}


void CodeAtlas::ClassLayouter::writeResults()
{
	Layouter::writeResults();
}
 
bool CodeAtlas::ClassLayouter::computeCluster()
{
	FuzzyDependAttr::Ptr fuzzyAttr = m_parent->getAttr<FuzzyDependAttr>();
	if (!fuzzyAttr)
		return false;

	Graph G;
	ClusterGraph CG(G);
	ClusterGraphAttributes CGA(	CG, 
		ClusterGraphAttributes::nodeId|
		ClusterGraphAttributes::nodeGraphics|
		ClusterGraphAttributes::edgeGraphics);

	SparseMatrix& veMat = fuzzyAttr->vtxEdgeMatrix();
	vector<node> nodeArray;
	vector<edge> edgeArray;
	m_nodeRadius.resize(m_childList.size());
	SList<node>  publicList, protectedList, privateList;
	for (int i = 0; i < m_childList.size(); ++i)
	{
		nodeArray.push_back(G.newNode());
		float r;
		LayoutSetting::getNodeRadius(m_childList[i], r);
		m_nodeRadius[i] = r;
		CGA.width(nodeArray.back()) = r*2;
		CGA.height(nodeArray.back()) = r*2;

		SymbolInfo childInfo = m_childList[i]->getSymInfo();
		if (childInfo.elementType() & SymbolInfo::PublicMember)
			publicList.pushBack(nodeArray.back());
		else if (childInfo.elementType() & SymbolInfo::ProtectedMember)
			protectedList.pushBack(nodeArray.back());
		else if (childInfo.elementType() & SymbolInfo::PrivateMember)
			privateList.pushBack(nodeArray.back());
	}

	for (int ithEdge = 0; ithEdge < veMat.cols(); ++ithEdge)
	{
		int src, dst;
		GraphUtility::getVtxFromEdge(veMat, ithEdge, src, dst);
		edgeArray.push_back(G.newEdge(nodeArray[src], nodeArray[dst]));
	}

	cluster publicC, protectedC, privateC;
	if (publicList.size() != 0 && publicList.size() != m_childList.size())
		publicC = CG.createCluster(publicList);
	if (protectedList.size() && protectedList.size() != m_childList.size())
		protectedC = CG.createCluster(protectedList);
	if (privateList.size() && privateList.size() != m_childList.size())
		privateC = CG.createCluster(privateList);
 

	ClusterPlanarizationLayout cpl;
	ClusterOrthoLayout* col = new ClusterOrthoLayout;
	col->separation(10);
	col->margin(30);
	cpl.setPlanarLayouter(col);
	cpl.call (G, CGA, CG,false);

	MatrixXd pos;
	pos.resize(m_childList.size(), 2);
	for (int v = 0; v < nodeArray.size(); ++v)
	{
		double x = CGA.x(nodeArray[v]);
		double y = CGA.y(nodeArray[v]);
		pos(v,0) = x;
		pos(v,1) = y;
	}

	VectorXd halfSize,offset;
	GeometryUtility::moveToOrigin(pos, m_nodeRadius.cast<double>(), halfSize, &offset);
	m_nodePos = pos.cast<float>();

	for(int ithEdge = 0; ithEdge < edgeArray.size(); ++ithEdge)
	{
		DPolyline& p = CGA.bends(edgeArray[ithEdge]);
		int nPoints = p.size();
		EdgeParam ep;
		ep.m_points.setZero(nPoints,2);
		int ithPnt = 0;
		for (DPolyline::iterator pLine = p.begin(); pLine != p.end(); ++pLine, ++ithPnt)
		{
			ep.m_points(ithPnt, 0) = (*pLine).m_x + offset[0];
			ep.m_points(ithPnt, 1) = (*pLine).m_y + offset[1];
		}
		m_edgeParam.push_back(ep);
	}

	m_totalRadius = qSqrt(halfSize[0]*halfSize[0] + halfSize[1]*halfSize[1]);
	return true;
}

bool CodeAtlas::ClassLayouter::compute()
{
	CHECK_ERRORS_RETURN_BOOL(m_status);

	if (computeNormally())
	{
		if (!computeVisualHull(m_totalRadius * 0.05))
			m_status |= WARNING_NO_VISUAL_HULL;
	}
	else
		trivalLayout();

	return true;
}

