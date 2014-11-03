#include "stdafx.h"
#include "Layouter.h"

using namespace CodeAtlas;
using namespace ogdf;

const float LayoutSetting::s_baseRadius = 10.f;
const float LayoutSetting::s_wordAvgLength = 6;

void LayoutSetting::getNodeRadius(const SymbolNode::Ptr& node, float& radius)
{
	SymbolInfo info = node->getSymInfo();
	if (info.elementType() & SymbolInfo::FunctionSignalSlot)
	{
		if (CodeAttr::Ptr codeAttr = node->getAttr<CodeAttr>())
			radius = code2Radius(codeAttr->getCode());
		else if (SymbolWordAttr::Ptr wordAttr = node->getAttr<SymbolWordAttr>())
			radius = word2Radius(wordAttr->getTotalWordWeight());
		else
			radius = s_baseRadius;
	}
	else if (info.elementType() & SymbolInfo::Variable)
	{
		if (info.isPrimaryType())
			radius = s_baseRadius * 5.f;
		else
			radius = s_baseRadius * 10.f;
	}
	else
	{
		// for project, class
		if (UIElementAttr::Ptr uiAttr = node->getAttr<UIElementAttr>())
			radius = qMax(uiAttr->radius(), s_baseRadius);
		else if (SymbolWordAttr::Ptr wordAttr = node->getAttr<SymbolWordAttr>())
			radius = word2Radius(wordAttr->getTotalWordWeight());
		else if (CodeAttr::Ptr codeAttr = node->getAttr<CodeAttr>())
			radius = code2Radius(codeAttr->getCode());
		else
			radius = s_baseRadius;
	}
	return;
}
void LayoutSetting::getChildrenRadius( const QList<SymbolNode::Ptr>& childList, VectorXf& radiusVec )
{
	radiusVec.resize(childList.size());

	for (int ithChild = 0; ithChild < childList.size(); ++ithChild)
	{
		const SymbolNode::Ptr& child = childList[ithChild];
		getNodeRadius(child, radiusVec[ithChild]);
	}
}

bool TrivalLayouter::compute( const QList<SymbolNode::Ptr>& childList, MatrixXf& pos2D, VectorXf& radiusVec, float& radius )
{
	if (childList.size() == 0)
		return false;
	VectorXf& rVec = radiusVec;
	LayoutSetting::getChildrenRadius(childList, rVec);

	pos2D.resize(childList.size(), 2);
	if (childList.size() == 1)
	{
		pos2D(0,0) = pos2D(0,1) = 0;
		radius = rVec[0];
		return true;
	}

	const float maxChildR = rVec.maxCoeff();
	const float twoPi	= 2.f * 3.14159265f;
	const float c		= rVec.sum() * 2.f;
	const float r		= c / twoPi + 1e-3;
	const float f		= 1.3f;
	float angle = 0; 

	for (int ithChild = 0; ithChild < childList.size(); ++ithChild)
	{
		const SymbolNode::Ptr& node = childList[ithChild];
		float l = rVec[ithChild] * 2.f;
		float halfAng = 0.5 * l / r;
		angle += halfAng;
		pos2D(ithChild, 0) = r*f * cos(angle);
		pos2D(ithChild, 1) = r*f * sin(angle);
		angle += halfAng;
	}
	radius = (r * f + maxChildR);

	return true;
}

void Layouter::mds(const MatrixXd& distMat, 
						   const VectorXf& radiusVec, 
						   const VectorXi& hashID,
						   MatrixXf& finalPos2D, 
						   float& finalRadius,
						   float  sparseFactor,
						   float  paddingRatio,
						   float  minPadding)
{
	if (distMat.rows() <= 0 || distMat.cols() <= 0 || radiusVec.size() <= 0)
	{
		finalPos2D = MatrixXf::Zero(1,2);
		finalRadius = 0;
		return;
	}
	if (distMat.rows() == 1 && distMat.cols() == 1 && radiusVec.size() == 1)
	{
		finalPos2D = MatrixXf::Zero(1,2);
		finalRadius = radiusVec[0];
		return;
	}
	MatrixXd pos2D, finalPosd;
	ClassicalMDSSolver::compute(distMat, pos2D); 
	
	VectorXd minPos, maxPos;
	MDSPostProcesser m_postProcessor(5000, sparseFactor, 1.0, paddingRatio, minPadding);
	m_postProcessor.set2DPos(pos2D, radiusVec.cast<double>(), &hashID);
	m_postProcessor.compute();
	m_postProcessor.getFinalPos(finalPosd);
	m_postProcessor.getFinalBoundingRect(minPos, maxPos);
	finalPos2D = finalPosd.cast<float>();
	// 	finalPos2D = pos2D.cast<float>();
	// 	minPos = pos2D.colwise().minCoeff();
	// 	maxPos = pos2D.colwise().maxCoeff();
	//  VectorXd size = maxPos - minPos;
	finalRadius = m_postProcessor.getFinalRadius();// size.norm() * 0.5;//(size[0] > size[1] ? size[0] : size[1]) * 0.5f;
}

void Layouter::laplacianEigMap( const MatrixXd& laplacian, const VectorXf& radiusVec, const VectorXi& hashID, MatrixXf& finalPos2D, float& finalRadius, float sparseFactor /*= 1.f*/ )
{
	if (laplacian.rows() <= 0 || laplacian.cols() <= 0 || radiusVec.size() <= 0)
	{
		finalPos2D = MatrixXf::Zero(1,2);
		finalRadius = 0;
		return;
	}
	if (laplacian.rows() == 1 && laplacian.cols() == 1 && radiusVec.size() == 1)
	{
		finalPos2D = MatrixXf::Zero(1,2);
		finalRadius = radiusVec[0];
		return;
	}
	MatrixXd pos2D, finalPosd;
	LaplacianSolver::compute(laplacian, pos2D); 

	VectorXd minPos, maxPos;
	MDSPostProcesser m_postProcessor(500, 1.0f, 1.0, 1.02, LayoutSetting::s_baseRadius);
	m_postProcessor.setSparseFactor(sparseFactor);
	m_postProcessor.set2DPos(pos2D, radiusVec.cast<double>(), &hashID);
	m_postProcessor.compute();
	m_postProcessor.getFinalPos(finalPosd);
	m_postProcessor.getFinalBoundingRect(minPos, maxPos);
	finalPos2D = finalPosd.cast<float>();
	// 	finalPos2D = pos2D.cast<float>();
	// 	minPos = pos2D.colwise().minCoeff();
	// 	maxPos = pos2D.colwise().maxCoeff();
	//  VectorXd size = maxPos - minPos;
	finalRadius = m_postProcessor.getFinalRadius();// size.norm() * 0.5;//(size[0] > size[1] ? size[0] : size[1]) * 0.5f;
}

bool TrivalLayouter::compute()
{
	CHECK_ERRORS_RETURN_BOOL(m_status);
	computePos();
	CHECK_ERRORS_RETURN_BOOL(m_status);
	computeEdgeRoute();
	CHECK_ERRORS_RETURN_BOOL(m_status);
	computeVisualHull();
	return true;
}
bool TrivalLayouter::computePos()
{
	CHECK_ERRORS_RETURN_BOOL(m_status);
	VectorXf& rVec = m_nodeRadius;
	m_nodePos.resize(m_childList.size(),2);

	// trival case
	if (m_childList.size() == 1)
	{
		m_totalRadius = rVec[0];
		m_nodePos(0,0) = m_nodePos(0,1) = 0.0f;
		return true;
	}

	const float maxChildR = rVec.maxCoeff();
	const float twoPi	= 2.f * 3.14159265f;
	const float c		= rVec.sum() * 4.f;
	const float f		= 1.3f;
	const float avgR	= (c / twoPi + 1e-3) * f;
	float anglePos = 0; 

	m_totalRadius = 0;
	for (int ithChild = 0; ithChild < m_childList.size(); ++ithChild)
	{
		const SymbolNode::Ptr& node = m_childList[ithChild];
		float r = rVec[ithChild];
		float d = r * 4.f;
		float angle = d / c * twoPi;
		float halfAng = 0.5 * angle;
		float R = avgR;
// 		if (angle < twoPi * 0.5)
// 			R = r / sin(halfAng);

		anglePos += halfAng;
		m_nodePos(ithChild, 0) = R * cos(anglePos);
		m_nodePos(ithChild, 1) = R * sin(anglePos);
		anglePos += halfAng;

		m_totalRadius = qMax(m_totalRadius, R + r);
	}
	return true;
}

bool TrivalLayouter::computeEdgeRoute()
{
	CHECK_ERRORS_RETURN_BOOL(m_status);
	FuzzyDependAttr::Ptr fuzzyAttr = m_parent->getAttr<FuzzyDependAttr>();

	if (fuzzyAttr.isNull())
		m_status |= WARNING_NO_EDGE;
	CHECK_RETURN_BOOL(m_status, WARNING_NO_EDGE);

	SparseMatrix& veMat = fuzzyAttr->vtxEdgeMatrix();
	VectorXd	  edgeWeight	= fuzzyAttr->edgeWeightVector();

	if (veMat.cols() <= 0)
		m_status |= WARNING_NO_EDGE;
	CHECK_RETURN_BOOL(m_status, WARNING_NO_EDGE);

	if (edgeWeight.size() != veMat.cols())
	{
		m_status |= WARNING_USE_DEFAULT_EDGE_WEIGHT;
		edgeWeight.setOnes(veMat.cols());
	}
	m_edgeParam.resize(veMat.cols());
	for (int ithEdge = 0; ithEdge < veMat.cols(); ++ithEdge)
	{
		int srcVtx, tarVtx;
		if(!GraphUtility::getVtxFromEdge(veMat, ithEdge, srcVtx, tarVtx))
			continue;

		float srcPntX = m_nodePos(srcVtx,0), srcPntY = m_nodePos(srcVtx,1);
		float tarPntX = m_nodePos(tarVtx,0), tarPntY = m_nodePos(tarVtx,1);
		float srcLength = sqrt(srcPntX*srcPntX + srcPntY*srcPntY);
		float tarLength = sqrt(tarPntX*tarPntX + tarPntY*tarPntY);
		float srcRatio  = (srcLength - m_nodeRadius[srcVtx]) / srcLength;
		float tarRatio  = (tarLength - m_nodeRadius[tarVtx]) / tarLength;

		BSpline splineBuilder(3);
		splineBuilder.addPoint(srcPntX*srcRatio, srcPntY*srcRatio);
		splineBuilder.addPoint(0,0);
		splineBuilder.addPoint(tarPntX*tarRatio, tarPntY*tarRatio);

		splineBuilder.computeLine(10);
		m_edgeParam[ithEdge].m_points = splineBuilder.getCurveMatrix();
		m_edgeParam[ithEdge].m_weight = edgeWeight[ithEdge];
	}

	return true;
}

bool TrivalLayouter::computeVisualHull()
{
	const int nSeg = 20;
	m_visualHull.resize(nSeg+1);
	float delta = (float)M_PI * 2 / nSeg;
	float angle = 0;
	for (int i = 0; i <= 20; ++i, angle +=delta)
	{
		m_visualHull[i].rx() = m_totalRadius * cos(angle);
		m_visualHull[i].ry() = m_totalRadius * sin(angle);
	}
	return true;
}

void Layouter::setNodes( const QList<SymbolNode::Ptr>& nodeList, const SymbolNode::Ptr& parent /*= SymbolNode::Ptr()*/ )
{
	if (nodeList.size() == 0)
		m_status |= ERROR_CHILDREN_INVALID;
	if (parent.isNull())
		m_status |= ERROR_PARENT_INVALID;
	CHECK_ERRORS_RETURN_VOID(m_status);

	clear();
	m_childList = nodeList;
	m_parent   = parent;
	LayoutSetting::getChildrenRadius(m_childList, m_nodeRadius);
}

void Layouter::writeResults()
{
	CHECK_ERRORS_RETURN_VOID(m_status);

	FuzzyDependAttr::Ptr fuzzyAttr = m_parent->getAttr<FuzzyDependAttr>();
	VectorXi* levelVec = NULL;
	if (fuzzyAttr)
		levelVec = &fuzzyAttr->levelVector();

	// node position and level
	m_childParam.resize(m_childList.size());
	for (int ithChild = 0; ithChild < m_childList.size(); ++ithChild)
	{
		const SymbolNode::Ptr& node = m_childList[ithChild];
		if (UIElementAttr::Ptr uiAttr = node->getOrAddAttr<UIElementAttr>())
		{
			uiAttr->position() = QPointF(m_nodePos(ithChild, 0), m_nodePos(ithChild, 1));
			uiAttr->radius()   = m_nodeRadius[ithChild];
			if (levelVec && levelVec->size() == m_childList.size())
			{
				uiAttr->level() = (*levelVec)[ithChild];
			}

			NodeParam& cp = m_childParam[ithChild];
			uiAttr->setEntryInfo(cp.m_inPnts, cp.m_outPnts, cp.m_inNors, cp.m_outNors);
		}
	}
	if (UIElementAttr::Ptr uiAttr = m_parent->getOrAddAttr<UIElementAttr>())
	{
		uiAttr->radius() = getTotalRadius(true);
	}

	// visual hull
	if (!(m_status & WARNING_NO_VISUAL_HULL))
	{
		UIElementAttr::Ptr uiAttr = m_parent->getOrAddAttr<UIElementAttr>();
		uiAttr->setVisualHull(m_visualHull);
	}

	// edges
	FuzzyDependAttr::DependPairList depPairList;
	if (!fuzzyAttr)
		m_status |= WARNING_NO_EDGE;
	else
	{
		depPairList = fuzzyAttr->dependPairList();
		if (m_edgeParam.size() != depPairList.size())
			m_status |= WARNING_INVALID_EDGE_DATA;
	}

	// write edge attr
	if ((m_status & (WARNING_INVALID_EDGE_DATA | WARNING_NO_EDGE)) == 0)
	{
		for (int ithEdge = 0; ithEdge < depPairList.size(); ++ithEdge)
		{
			FuzzyDependAttr::DependPair& edgeKey = depPairList[ithEdge];
			SymbolEdge::Ptr edge = SymbolTree::getEdge(edgeKey.m_defNode, edgeKey.m_refNode, SymbolEdge::EDGE_FUZZY_DEPEND);
			EdgeUIElementAttr::Ptr uiAttr = edge->getOrAddAttr<EdgeUIElementAttr>();
			MatrixXf& pnts = m_edgeParam[ithEdge].m_points;
			if (pnts.rows() <= 0 || pnts.cols() != 2)
				continue;
			uiAttr->pointList() = m_edgeParam[ithEdge].m_points;
			uiAttr->edgeWeight() = m_edgeParam[ithEdge].m_weight;
		}
	}

}

void Layouter::clear()
{
	m_childList.clear();
	m_parent = SymbolNode::Ptr();
	m_nodePos = MatrixXf();
	m_nodeRadius = VectorXf();
	m_visualHull.clear();
	m_totalRadius = 0;
}

float Layouter::getTotalRadius( bool withPadding /*= true*/ )const
{
	if (withPadding)
		return m_totalRadius * m_paddingFactor;
	return m_totalRadius;
}

void Layouter::trivalLayout()
{
	CHECK_ERRORS_RETURN_VOID(m_status);

	TrivalLayouter trivalLayouter;
	m_status |= WARNING_TRIVAL_LAYOUT;
	trivalLayouter.setNodes(m_childList, m_parent);
	trivalLayouter.compute();
	m_status |= trivalLayouter.getStatus();

	CHECK_ERRORS_RETURN_VOID(m_status);

	m_nodePos = trivalLayouter.getNodePos();
	m_nodeRadius = trivalLayouter.getNodeRadius();
	m_totalRadius = trivalLayouter.getTotalRadius();
	m_edgeParam = trivalLayouter.getEdgeParams();
	m_visualHull = trivalLayouter.getVisualHull();
}

QPainterPath Layouter::mat2Path( const MatrixXf& pntMat )
{
	QPainterPath path;
	if (pntMat.rows() <= 0 || pntMat.cols() != 2)
		return path;
	path.moveTo(pntMat(0,0), pntMat(0,1));
	for (int i = 1; i < pntMat.rows(); ++i)
		path.lineTo(pntMat(i,0), pntMat(i,1));
	return path;
}

QPolygonF Layouter::getRect( const QPointF& center, float radius )
{
	QPolygonF poly(8);
	poly[7] = QPointF(center.x() - radius, center.y() - radius);
	poly[0] = QPointF(center.x()         , center.y() - radius);
	poly[1] = QPointF(center.x() + radius, center.y() - radius);
	poly[2] = QPointF(center.x() + radius, center.y());
	poly[3] = QPointF(center.x() + radius, center.y() + radius);
	poly[4] = QPointF(center.x()         , center.y() + radius);
	poly[5] = QPointF(center.x() - radius, center.y() + radius);
	poly[6] = QPointF(center.x() - radius, center.y());
	return poly;
}

bool Layouter::computeEdgeRoute(DelaunayCore::DelaunayRouter router)
{
	CHECK_RETURN_BOOL(m_status, WARNING_NO_EDGE);

	FuzzyDependAttr::Ptr fuzzyAttr = m_parent->getAttr<FuzzyDependAttr>();
	if (fuzzyAttr.isNull())
	{
		m_status |= WARNING_NO_EDGE;
		return false;
	}
	SparseMatrix& veMat = fuzzyAttr->vtxEdgeMatrix();
	const QList<SymbolNode::Ptr>& childList = m_childList; 
	const MatrixXf& childPos = m_nodePos;
	QVector<EdgeParam>& edgeParam = m_edgeParam;

	if (childList.size() <= 0 || childPos.rows() != childList.size())
		m_status |= WARNING_NO_EDGE;
	if (veMat.rows() <= 0 || veMat.cols() <= 0)
		m_status |= WARNING_NO_EDGE;
	CHECK_RETURN_BOOL(m_status, WARNING_NO_EDGE);

	edgeParam.resize(veMat.cols());	
	vector<int> childIDMap;
	childIDMap.resize(childList.size(), -1);
	for (int ithChild = 0; ithChild < childList.size(); ++ithChild)
	{
		const SymbolNode::Ptr& node = childList[ithChild];
		if (UIElementAttr::Ptr uiAttr = node->getOrAddAttr<UIElementAttr>())
		{
			QSharedPointer<QPolygonF> visualHull = uiAttr->getVisualHull();
			bool res = false;
			if (visualHull && visualHull->size() >= 3)
			{
				res = router.addContour(visualHull->translated(childPos(ithChild,0),childPos(ithChild,1)));
			}
			else
			{
				QPolygonF poly = getRect(QPointF(childPos(ithChild,0),childPos(ithChild,1)), m_nodeRadius[ithChild]);
				res = router.addContour(poly);
			}
			if (res)
			{
				childIDMap[ithChild] = router.getNumContours()-1;
			}
		}
	}

	if (router.getNumContours() <= 0)
		return false;
	router.compute();

	QByteArray name = m_parent->getSymInfo().name().toLatin1();
#ifdef WRITE_DELAUNAY_MESH
 	char buf[200];
 	sprintf(buf, "H:\\Programs\\QtCreator\\qt-creator_master\\src\\plugins\\MyPlugin\\CodeAtlas\\triMesh_%s.obj", name.constData());
 	OpenMesh::IO::write_mesh(router.getRouteMesh(), buf);
#endif

	// get edge weight vector
	VectorXd	  edgeWeight	= fuzzyAttr->edgeWeightVector();
	if (edgeWeight.size() != veMat.cols())
	{
		m_status |= WARNING_USE_DEFAULT_EDGE_WEIGHT;
		edgeWeight.setOnes(veMat.cols());
	}

	// find actual route
	for (int ithEdge = 0; ithEdge < veMat.cols(); ++ithEdge)
	{
		int srcVtx, tarVtx;
		if(!GraphUtility::getVtxFromEdge(veMat, ithEdge, srcVtx, tarVtx))
			continue;

		int routerSrcID = childIDMap[srcVtx];
		int routerTarID = childIDMap[tarVtx];
		//qDebug("src %d tar %d routeSrc %d routeTar %d\n", srcVtx, tarVtx, routerSrcID, routerTarID);
		if (routerSrcID == -1 || routerTarID == -1)
		{
			qDebug("route not found.");
			continue;
		}

		vector<DelaunayCore::VtxPoint> route;
		if (!router.getRoute(routerSrcID, routerTarID, route, true))
		{
			qDebug("unable to find legal route for %s, try to find route with block constraint", name.constData());
			router.getRoute(routerSrcID, routerTarID, route, false);
		}
		if (route.size() <= 0)
			qDebug("unable to find route!");
		else
		{
			MatrixXf& pnts = edgeParam[ithEdge].m_points;
#ifdef EDGE_ROUTE_USE_BSPLINE
			BSpline bsplineMaker(3, BSpline::BSPLINE_OPEN_UNIFORM, true);			// use B-spline to smooth route curve
			for (int i = 0; i < route.size(); ++i)
			{
				bsplineMaker.addPoint(route[i].X, route[i].Y);
			}
			bsplineMaker.computeLine(route.size() * 2);
			QList<QPointF>curve = bsplineMaker.getCurvePnts();

			pnts.resize(curve.size(), 2);
			for (int i = 0; i < curve.size(); ++i)
			{
				pnts(i,0) = curve[i].x();
				pnts(i,1) = curve[i].y();
			}
#else
			pnts.resize(route.size(), 2);
			for (int i = 0; i < route.size(); ++i)
			{
				pnts(i,0) = route[i].X;
				pnts(i,1) = route[i].Y;
			}
#endif
			edgeParam[ithEdge].m_weight = edgeWeight[ithEdge];
		}
	}

	// collect entries
	m_childParam.resize(childList.size());
	for (int ithChild = 0; ithChild < childList.size(); ++ithChild)
	{
		int contourID = childIDMap[ithChild];
		if (contourID == -1)
			continue;

		QPointF childPos(m_nodePos(ithChild, 0), m_nodePos(ithChild, 1));
		NodeParam& cp = m_childParam[ithChild];
		router.getContourEntries(contourID, cp.m_inPnts, cp.m_inNors, cp.m_outPnts, cp.m_outNors);
		for (int i = 0; i < cp.m_inPnts.size(); ++i)
		{
			cp.m_inPnts[i] -= childPos;
		}
		for (int i = 0; i < cp.m_outPnts.size(); ++i)
		{
			cp.m_outPnts[i] -= childPos;
		}
	}
	return true;
}


bool CodeAtlas::Layouter::computeVisualHull(float padding)
{
	CHECK_ERRORS_RETURN_BOOL(m_status);

	std::vector<double> pointSetX, pointSetY;
	float visualHullBuffer = padding;

	for (int ithVtx = 0; ithVtx < m_nodePos.rows(); ++ithVtx)
	{
		float x = m_nodePos(ithVtx, 0);
		float y = m_nodePos(ithVtx, 1);
		float r = m_nodeRadius(ithVtx) * 2.f + visualHullBuffer;
		pointSetX.push_back(x-r);		pointSetY.push_back(y-r);
		pointSetX.push_back(x-r);		pointSetY.push_back(y+r);
		pointSetX.push_back(x+r);		pointSetY.push_back(y-r);
		pointSetX.push_back(x+r);		pointSetY.push_back(y+r);
	}
	for (int ithEdge = 0; ithEdge < m_edgeParam.size() ; ++ithEdge)
	{
		MatrixXf& edgePnts = m_edgeParam[ithEdge].m_points;
		for (int ithVtx = 0; ithVtx < edgePnts.rows(); ++ithVtx)
		{
			float x = edgePnts(ithVtx, 0);
			float y = edgePnts(ithVtx, 1);
			float r = visualHullBuffer;
			pointSetX.push_back(x-r);			pointSetY.push_back(y-r);
			pointSetX.push_back(x-r);			pointSetY.push_back(y+r);
			pointSetX.push_back(x+r);			pointSetY.push_back(y-r);
			pointSetX.push_back(x+r);			pointSetY.push_back(y+r);
		}
	}

	QVector<QPointF> hull;
	if(!GeometryUtility::computeConvexHull(&pointSetX[0], &pointSetY[0], pointSetX.size(), hull) )
	{
		m_status |= WARNING_NO_VISUAL_HULL;
		qDebug("compute convex hull failed!");
		return false;
	}

	QVector<QPointF> poly;
	if(!GeometryUtility::samplePolygon(hull, poly, 1.1, 12))
		poly = hull;
	BSpline bs(3, BSpline::BSPLINE_PERIODIC);
	for (int ithPnt = 0; ithPnt < poly.size(); ++ithPnt)
	{
		bs.addPoint(poly[ithPnt]);
	}
	bs.computeLine(poly.size() * 2);
	m_visualHull = bs.getCurvePnts().toVector();

	float bound[2] = {0,0};
	foreach (QPointF p, m_visualHull)
	{
		bound[0] = qMax(bound[0], (float)qAbs(p.x()));
		bound[1] = qMax(bound[1], (float)qAbs(p.y()));
	}
	m_totalRadius = sqrt(bound[0]*bound[0] + bound[1]*bound[1]);
	m_status &= ~WARNING_NO_VISUAL_HULL;
	return true;
}

bool CodeAtlas::Layouter::graphLayout( const SparseMatrix& veMat, const VectorXf& radiusVec, MatrixXf& finalPos2D, float& finalRadius, float sparseFactor /*= 1.f*/ )
{
	Graph G;
	GraphAttributes GA(G,
		GraphAttributes::nodeGraphics | GraphAttributes::edgeGraphics);

	const int nNodes = veMat.rows();
	const int nEdges = veMat.cols();
	if (nNodes <= 0 || nEdges < 1)
		return false;

	vector<node> nodeArray;
	vector<edge> edgeArray;
	NodeArray<float> nodeSize(G);
	EdgeArray<double> edgeLength(G);
	for (int i = 0; i < nNodes; ++i)
	{
		nodeArray.push_back(G.newNode());
		float r = radiusVec[i];
		GA.width(nodeArray.back()) = r*2;
		GA.height(nodeArray.back()) = r*2;
		nodeSize[nodeArray.back()] = r * 2;
	}

	for (int ithEdge = 0; ithEdge < nEdges; ++ithEdge)
	{
		int src, dst;
		GraphUtility::getVtxFromEdge(veMat, ithEdge, src, dst);
		edgeArray.push_back(G.newEdge(nodeArray[src], nodeArray[dst]));
		edgeLength[edgeArray.back()] = 1;
	}

	MatrixXd pos;
	pos.resize(nNodes, 2);

	try
	{
		FMMMLayout layouter;
		float avgRadius = radiusVec.sum();
		float minRadius = radiusVec.minCoeff();
		layouter.unitEdgeLength(avgRadius * 4);
		layouter.call(GA);
		for (int v = 0; v < nNodes; ++v)
		{
			double x = GA.x(nodeArray[v]);
			double y = GA.y(nodeArray[v]);
			pos(v,0) = x;
			pos(v,1) = y;
		}
		MDSPostProcesser m_postProcessor(5000, sparseFactor, 1.0, 0.08, minRadius * 2);
		m_postProcessor.set2DPos(pos, radiusVec.cast<double>());
		m_postProcessor.compute();
		m_postProcessor.getFinalPos(pos);
		finalPos2D = pos.cast<float>();
		finalRadius = m_postProcessor.getFinalRadius();
	}
	catch(...)//AlgorithmFailureException e
	{
		return false;
	}
	return true;
}

