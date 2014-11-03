#include "stdafx.h"
#include "ComponentLayouter.h"

using namespace CodeAtlas;

void ComponentLayouter::layoutByGraph( QVector<Component>& compInfo )
{
	//	qDebug()<< "by graph";
	QList<VectorXf>     radiusMat;
	QList<MatrixXf>		compPos;
	for (int ithComp = 0; ithComp < compInfo.size(); ++ithComp)
	{
		Component& comp = compInfo[ithComp];
		int nCompVtx = comp.m_pVEIncidenceMat->rows();
		int nCompEdge= comp.m_pVEIncidenceMat->cols();
		if (nCompVtx == 1 || nCompVtx == 0 || nCompEdge == 0)
		{
			comp.m_localPos2D.resize(1,2);
			comp.m_localPos2D(0,0) = comp.m_localPos2D(0,1) = 0.f;
			comp.m_radius = comp.m_vtxRadius(0);
			continue;
		}
		VectorXf rVec = comp.m_vtxRadius.head(nCompVtx);

#ifdef OGDF_GRAPH_LAYOUT
		if(!graphLayout(*comp.m_pVEIncidenceMat, rVec, comp.m_localPos2D, comp.m_radius, 0.8))
		{
			MatrixXf distMat;
			VectorXf edgeLength = VectorXf::Ones(nCompEdge);
			GraphUtility::computePairDist(*comp.m_pVEIncidenceMat, distMat, &edgeLength);
			mds(distMat.cast<double>(), rVec, comp.m_hashID, comp.m_localPos2D, comp.m_radius, 1.2f, 0.15f, rVec.minCoeff());
		}
#else
 		MatrixXf distMat;
 		VectorXf edgeLength = VectorXf::Ones(nCompEdge).cwiseQuotient(comp.m_edgeWeight);
 		GraphUtility::computePairDist(*comp.m_pVEIncidenceMat, distMat, &edgeLength);
 		mds(distMat.cast<double>(), rVec, comp.m_hashID, comp.m_localPos2D, comp.m_radius);
#endif
		
		//ComponentLayouter::orthoGraphLayout(*comp.m_pVEIncidenceMat, rVec, comp.m_localPos2D, comp.m_radius, comp.m_edgePath);

		// 		MatrixXd laplacian;
		// 		GraphUtility::incidence2Laplacian(*comp.m_pVEIncidenceMat, comp.m_edgeWeight, laplacian);
		// 		laplacianEigMap(laplacian, rVec, comp.m_hashID, comp.m_localPos2D, comp.m_radius);
	}
}

void ComponentLayouter::layoutByWord( QVector<Component>& compInfo, float& finalRadius )
{
	//	qDebug()<< "by word";
	int nComp = compInfo.size();
	if (nComp == 1)
	{
		Component& comp = compInfo[0];
		comp.m_compPos2D.resize(2);
		comp.m_compPos2D[0] = comp.m_compPos2D[1] = 0.f;
		finalRadius = comp.m_radius;
		return;
	}
	MatrixXd distMat(nComp, nComp);
	for (int i = 0; i < nComp; ++i)
	{
		distMat(i,i) = 0.f;
		for (int j = i+1; j < nComp; ++j)
		{
			Component& compI = compInfo[i];
			Component& compJ = compInfo[j];
			double cosVal = compI.m_wordAttr.cosSimilarity(compJ.m_wordAttr);
			distMat(i,j) = distMat(j,i) = 1.0 - cosVal;
		}
	}

	VectorXf rVec;
	VectorXi hashVec;
	rVec.resize(nComp);
	hashVec.resize(nComp);
	for (int ithComp = 0; ithComp < nComp; ++ithComp)
	{
		rVec[ithComp] = compInfo[ithComp].m_radius;
		hashVec[ithComp] = compInfo[ithComp].m_compHash;
	}

	MatrixXf finalPos2D;
	Layouter::mds(distMat, rVec, hashVec, finalPos2D, finalRadius, 0.8f, 0.01f, 0);

	for (int ithComp = 0; ithComp < nComp; ++ithComp)
		compInfo[ithComp].m_compPos2D = finalPos2D.row(ithComp);
} 




bool ComponentLayouter::compute(   const SparseMatrix*	vtxEdgeMatrix, 
								   const VectorXd*		edgeWeight,
								   const QList<SymbolNode::Ptr>& childList,
								   MatrixXf& childPos,
								   VectorXf& childRadius,
								   float& totalRadius)
{
	CHECK_ERRORS_RETURN_BOOL(m_status);

	// check edge data
	int nVtx1 = childList.size();
	bool hasEdgeData = (vtxEdgeMatrix != NULL && edgeWeight != NULL );
	if (hasEdgeData)
	{
		int nVtx0 = vtxEdgeMatrix->rows();
		int nEdge0= vtxEdgeMatrix->cols();
		int nEdge1 = edgeWeight->size();
		if (!(nVtx0 == nVtx1 && nEdge0 == nEdge1 && nVtx0 > 0 && nEdge0 > 0))
			m_status |= WARNING_INVALID_EDGE_DATA;
	}
	else
		m_status |= WARNING_NO_EDGE;
	bool isVtxEdgeMatValid = (m_status & (WARNING_NO_EDGE | WARNING_INVALID_EDGE_DATA)) == 0;

	// fill child radius
	LayoutSetting::getChildrenRadius(childList, childRadius);

	QVector<Component>     compInfo;
	VectorXi	  vtxCompIdx, vtxIdx, edgeCompIdx, edgeIdx;
	QList<SparseMatrix> compVEMat;
	int nComp = 0; 
	if (isVtxEdgeMatValid)
	{
		GraphUtility::splitConnectedComponents(*vtxEdgeMatrix, vtxCompIdx, vtxIdx, edgeCompIdx, edgeIdx, compVEMat);
		nComp = compVEMat.size();
	}
	else
	{
		vtxCompIdx.resize(nVtx1);
		vtxIdx.resize(nVtx1);
		for (int ithVtx = 0; ithVtx < nVtx1; ++ithVtx)
		{
			vtxCompIdx[ithVtx] = ithVtx;
			vtxIdx[ithVtx]  = 0;
			compVEMat.push_back(SparseMatrix());
		}
		nComp = nVtx1;
	}

	// fill component information array
	compInfo.resize(nComp);
	for (int ithComp = 0; ithComp < nComp; ++ithComp)
	{
		Component& comp = compInfo[ithComp];
		comp.m_vtxRadius.resize(childRadius.size());
		comp.m_hashID.resize(childRadius.size());
		comp.m_pVEIncidenceMat = &compVEMat[ithComp];
		comp.m_edgeWeight.resize(compVEMat[ithComp].cols());
	}
	for (int ithVtx = 0; ithVtx < vtxCompIdx.size(); ++ithVtx)
	{
		Component& comp = compInfo[vtxCompIdx[ithVtx]];
		comp.m_vtxRadius[vtxIdx[ithVtx]] = childRadius[ithVtx];
		unsigned vtxHash = childList[ithVtx]->getSymInfo().hash();
		comp.m_hashID[vtxIdx[ithVtx]]    = vtxHash;
		comp.m_compHash                  = comp.m_compHash ^ vtxHash;
	}
	if (edgeWeight)
	{
		for (int ithEdge = 0; ithEdge < edgeCompIdx.size(); ++ithEdge)
		{
			compInfo[edgeCompIdx[ithEdge]].m_edgeWeight[edgeIdx[ithEdge]] = (*edgeWeight)[ithEdge];
		}
	}

	// layout within each components
	if (isVtxEdgeMatValid)
		layoutByGraph(compInfo);
	else
	{
		// set component result directly
		for (int ithComp = 0; ithComp < nComp; ++ithComp)
		{
			Component& comp = compInfo[ithComp];
			float r = childRadius[ithComp];
			comp.m_vtxRadius.setConstant(1, r);
			comp.m_radius    = r;
			comp.m_localPos2D.setZero(1,2);
		}
	}

	// layout by word, first fill word attributes
	for (int ithChild = 0; ithChild < childList.size(); ++ithChild)
	{
		const SymbolNode::Ptr& child = childList[ithChild]; 
		if (SymbolWordAttr::Ptr wordAttr = child->getAttr<SymbolWordAttr>())
		{
			int ithComp = vtxCompIdx[ithChild];
			Component& comp = compInfo[ithComp];	
			comp.m_wordAttr.unite(*wordAttr);
		}			
	}
	float finalRadius = 1.f;
	layoutByWord(compInfo, finalRadius);

	// set children's location
	childPos.resize(childList.size(), 2);
	childRadius.resize(childList.size());
	for (int ithChild = 0; ithChild < childList.size(); ++ithChild)
	{
		const SymbolNode::Ptr& child = childList[ithChild];
		int compID = vtxCompIdx[ithChild];
		int newID  = vtxIdx[ithChild];

		Component& comp = compInfo[compID];

		childPos(ithChild, 0) = comp.m_compPos2D[0] + comp.m_localPos2D(newID,0);
		childPos(ithChild, 1) = comp.m_compPos2D[1] + comp.m_localPos2D(newID,1);
	}
	totalRadius = finalRadius;
	return true;
}




using namespace ogdf;
void ComponentLayouter::orthoGraphLayout(const SparseMatrix& veIncidenceMat, 
										const VectorXf& rVec, 
										MatrixXf& finalPos2D, 
										float& finalRadius , 
										QList<MatrixXf>& edgeList )
{
	typedef ogdf::Graph Graph;
	typedef ogdf::GraphAttributes GraphAttributes;

	int nVtx = rVec.size();
	int nEdge= veIncidenceMat.cols();

	Graph G;
	GraphAttributes GA(G,
		GraphAttributes::nodeGraphics | GraphAttributes::edgeGraphics );

	std::vector<node> nodeArray;
	std::vector<edge> edgeArray;
	for (int i = 0; i < nVtx; ++i)
	{
		nodeArray.push_back(G.newNode());
		GA.width(nodeArray.back()) = rVec[i]*2.f;
		GA.height(nodeArray.back())= rVec[i]*2.f;
	}

	for (int i = 0; i < nEdge; ++i)
	{
		int src, tar;
		GraphUtility::getVtxFromEdge(veIncidenceMat, i, src, tar);
		edgeArray.push_back(G.newEdge(nodeArray[src], nodeArray[tar]));
	}

	PlanarizationLayout pl;

// 	FastPlanarSubgraph *ps = new FastPlanarSubgraph;
// 	ps->runs(100);
// 	VariableEmbeddingInserter *ves = new VariableEmbeddingInserter;
// 	ves->removeReinsert(EdgeInsertionModule::rrAll);
// 
// 	pl.setSubgraph(ps);
// 	pl.setInserter(ves);
// 
// 	EmbedderMinDepthMaxFaceLayers *emb = new EmbedderMinDepthMaxFaceLayers;
// 	pl.setEmbedder(emb);
// 
// 	OrthoLayout *ol = new OrthoLayout;
// 	ol->separation(20.0);
// 	ol->cOverhang(0.4);
// 	ol->setOptions(2+4);

	//pl.setPlanarLayouter(ol);

	pl.call(GA);

	MatrixXd pos2D(nVtx, 2);
	for (int v = 0; v < nodeArray.size(); ++v)
	{
		double x = GA.x(nodeArray[v]);
		double y = GA.y(nodeArray[v]);
		pos2D(v, 0) = x;
		pos2D(v, 1) = y;
		//		cout << v << "th vtx: " << x << " " << y << endl;
	}
	VectorXd halfSize,offset;
	GeometryUtility::moveToOrigin(pos2D, rVec.cast<double>(), halfSize, &offset);
	finalPos2D  = pos2D.cast<float>();
	finalRadius = halfSize.norm();//(size[0] > size[1] ? size[0] : size[1]) * 0.5f;


	for(int e = 0; e < edgeArray.size(); ++e)
	{
		DPolyline& p = GA.bends(edgeArray[e]);
		int nPnts    = p.size();
		MatrixXf pntMat(nPnts,2);
		int ithPnt   = 0;
		for (DPolyline::iterator pLine = p.begin(); pLine != p.end(); ++pLine, ++ithPnt)
		{
			pntMat(ithPnt, 0) = (*pLine).m_x + offset[0];
			pntMat(ithPnt, 1) = (*pLine).m_y + offset[1];
		}
	}

}




bool CodeAtlas::ComponentLayouter::compute()
{
	CHECK_ERRORS_RETURN_BOOL(m_status);

	SymbolNode::Ptr node = m_parent;
	// for a interior node, find attribute
	FuzzyDependAttr::Ptr fuzzyAttr = node->getAttr<FuzzyDependAttr>();


#ifndef ONLY_USE_WORD_SIMILARITY
	if (fuzzyAttr.isNull() || 
		fuzzyAttr->edgeWeightVector().size() <= 0 || 
		fuzzyAttr->vtxEdgeMatrix().cols() <= 0 ||
		fuzzyAttr->vtxEdgeMatrix().rows() <= 0)
	{
		m_status |= WARNING_NO_EDGE;
		if (!ComponentLayouter::compute(NULL, NULL, m_childList, m_nodePos, m_nodeRadius, m_totalRadius))
			trivalLayout();
	}
	else
	{
		// fill data used to compute layout
		SparseMatrix& vtxEdgeMatrix = fuzzyAttr->vtxEdgeMatrix();
		VectorXd&	  edgeWeight	= fuzzyAttr->edgeWeightVector();
		
#ifdef CHOOSE_IMPORTANT_EDGES
		// filter edges		
		VectorXi degreeVec;
		degreeVec.setZero(vtxEdgeMatrix.rows());
		for (int ithEdge = 0; ithEdge < vtxEdgeMatrix.cols(); ++ithEdge)
		{
			int src, tar;
			GraphUtility::getVtxFromEdge(vtxEdgeMatrix, ithEdge, src, tar);
			degreeVec[src]++;
			degreeVec[tar]++;
		}

		float maxVal = degreeVec.maxCoeff();
		float minVal = degreeVec.minCoeff();
		const float ratio = 0.2f;
		float threshold = min(minVal + (maxVal - minVal) * ratio, 3.f);

		std::vector<Triplet> tripletArray;
		std::vector<double> filteredEdgeArray;
		for (int ithEdge = 0; ithEdge < edgeWeight.size(); ++ithEdge)
		{
			int src, tar;
			GraphUtility::getVtxFromEdge(vtxEdgeMatrix, ithEdge, src, tar);
			if (min(degreeVec[src], degreeVec[tar]) <= threshold)
			{
				tripletArray.push_back(Triplet(src, filteredEdgeArray.size() ,1.0));
				tripletArray.push_back(Triplet(tar, filteredEdgeArray.size() ,-1.0));

				filteredEdgeArray.push_back(edgeWeight[ithEdge]);
			}
		}
		SparseMatrix filteredVEMat(vtxEdgeMatrix.rows(), filteredEdgeArray.size());
		filteredVEMat.setFromTriplets(tripletArray.begin(), tripletArray.end());
		VectorXd filteredEdgeVec(filteredEdgeArray.size());
		memcpy(filteredEdgeVec.data(), &filteredEdgeArray[0], sizeof(double)*filteredEdgeArray.size());

		if (!ComponentLayouter::compute(&filteredVEMat, &filteredEdgeVec, m_childList, m_nodePos, m_nodeRadius, m_totalRadius))
			trivalLayout();
#else
		if (!ComponentLayouter::compute(&vtxEdgeMatrix, &edgeWeight, m_childList, m_nodePos, m_nodeRadius, m_totalRadius))
			trivalLayout();
#endif
		qDebug("compute edge routes");
		DelaunayCore::DelaunayRouter router;
		router.setSmoothParam(0.5, 4); 
		computeEdgeRoute(router);
	}

#else
	if (!ComponentLayouter::compute(NULL, NULL, m_childList, m_nodePos, m_nodeRadius, m_totalRadius))
		trivalLayout();
	if (!fuzzyAttr.isNull())
	{
		m_status &= ~WARNING_NO_EDGE;
		qDebug("compute edge routes");
		DelaunayCore::DelaunayRouter router;
		router.setSmoothParam(0.5, 4); 
		computeEdgeRoute(router);
	}
#endif

	if ((m_status & WARNING_TRIVAL_LAYOUT) == 0)
		computeVisualHull(m_totalRadius * 0.05);

	return true;
}



void CodeAtlas::ComponentLayouter::setNodes( const QList<SymbolNode::Ptr>& nodeList, const SymbolNode::Ptr& parent /*= SymbolNode::Ptr()*/ )
{
	Layouter::setNodes(nodeList, parent);
	m_trivalLayouter.setNodes(nodeList, parent);
	m_status |= m_trivalLayouter.getStatus();
}


