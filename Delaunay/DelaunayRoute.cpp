#include "StdAfx.h"
#include "DelaunayRoute.h"

using namespace DelaunayCore;

DelaunayRouter::DelaunayRouter(void):
m_isDoubleEdgeContour(false), m_doubleEdgeWidth(30.0), 
m_addContourCenter(true),
m_routeStepSize(1),
m_nRouteEntries(2), m_addSafeFrame(true),m_laneWidth(0),
m_smoothFactor(0.5), m_smoothIter(8),
m_endPointNormalRatio(0.1),
m_laneOffsetRatio(0.4),
m_avgContourRadius(0.f),
m_minContourRadius(0.f),
m_maxContourRadius(0.f),
m_boundaryLengthRatio(0.7),
m_laneWidthRatio(1.0)
{ 
}

DelaunayRouter::~DelaunayRouter(void)
{
}

bool DelaunayRouter::addContour( const Contour& contour )
{
	if (contour.m_vtxList.size() > 2)
	{
		m_contourList.push_back(contour);
		return true;
	}
	return false;
}
 
bool DelaunayRouter::addContour( const std::vector<VtxPoint>& contourPnt )
{
	return addContour(Contour(contourPnt));
}

void DelaunayRouter::buildVertexSetFromContour()
{
	m_vertexSet.clear();
	m_avgContourRadius = 0.f;
	m_minContourRadius = FLT_MAX;
	m_maxContourRadius = 0;
	for (int i = 0; i < m_contourList.size(); ++i)
	{
		Contour&c = m_contourList[i];
		REAL avgPnt[2] = {0,0};
		REAL minPnt[2] = {FLT_MAX, FLT_MAX};
		REAL maxPnt[2] = {-FLT_MAX, -FLT_MAX};
		int nVtx = c.m_vtxList.size();
		for (int ithVtx = 0; ithVtx < nVtx; ++ithVtx)
		{
			DelaunayCore::VtxPoint& p = c.m_vtxList[ithVtx];
			insertNonRepeated(p);
			avgPnt[0] += p.X;
			avgPnt[1] += p.Y;
			minPnt[0] = p.X < minPnt[0] ? p.X : minPnt[0];
			minPnt[1] = p.Y < minPnt[1] ? p.Y : minPnt[1];
			maxPnt[0] = p.X > maxPnt[0] ? p.X : maxPnt[0];
			maxPnt[1] = p.Y > maxPnt[1] ? p.Y : maxPnt[1];
		}
		avgPnt[0] /= (REAL)nVtx;
		avgPnt[1] /= (REAL)nVtx;
		printf("%lf %lf\n", minPnt[0], minPnt[1]);

		REAL size[2] = {maxPnt[0] - minPnt[0], maxPnt[1]-minPnt[1]};
		c.m_radius = 0.5 * sqrt(size[0] * size[0] + size[1] * size[1]);
		m_avgContourRadius += c.m_radius;
		m_minContourRadius = min(m_minContourRadius, c.m_radius);
		m_maxContourRadius = max(m_maxContourRadius, c.m_radius);
		if (m_addContourCenter)
		{
			VtxPoint v(avgPnt[0], avgPnt[1]);
			insertNonRepeated(v);
		}
		if (m_isDoubleEdgeContour)
		{ 
			int nPnts = c.m_vtxList.size();
			for (int ithVtx = 0; ithVtx < nPnts; ++ithVtx)
			{
				VtxPoint& thisPnt = c.m_vtxList[ithVtx];
				VtxPoint& nextPnt = c.m_vtxList[(ithVtx+1)%nPnts];
				VtxPoint& prevPnt = c.m_vtxList[(ithVtx+nPnts-1)%nPnts];
				
				VtxPoint  dPrev   = thisPnt - prevPnt;
				VtxPoint  dThis   = nextPnt - thisPnt;

				REAL midX = dPrev.Y + dThis.Y;
				REAL midY = -dPrev.X - dThis.X;
				REAL l    = sqrt(midX*midX + midY * midY);
				if (l > REAL_EPSILON)
				{
					REAL factor = m_doubleEdgeWidth / l;
					VtxPoint v(thisPnt.X + midX*factor, thisPnt.Y + midY*factor);
					insertNonRepeated(v);
				}
			}
		}
	}

	m_avgContourRadius /= m_contourList.size();
}

void DelaunayRouter::compute()
{
	if (m_contourList.size() <= 0)
		return;
	clock_t t0 = clock();
	buildVertexSetFromContour();
	if (m_addSafeFrame)
		buildSafeCircle();
		//buildSafeFrame();

#ifdef PRO_DELAUNAY
	proDelaunay();
#else
	Delaunay triangulater;
	triangulater.Triangulate(m_vertexSet, m_triangleSet);
#endif
	clock_t t1 = clock();
	buildOpenMeshData();
	clock_t t2 = clock();
	markContourHalfedge();
	clock_t t3 = clock();
	markInternalFaces();
	clock_t t4 = clock();
	printf("\n\ntime:\n triangulate:%dms\n build openmesh Data:%dms\n mark contour:%dms\n mark internal:%dms\n", t1-t0, t2-t1, t3-t2, t4-t3);
}



void DelaunayRouter::buildOpenMeshData()
{
	if (m_triangleSet.size() == 0)
		return;

	// add vertex and set handle
	for (vertexSet::iterator pVtx = m_vertexSet.begin(); pVtx != m_vertexSet.end(); ++pVtx)
	{
		const Vertex& v = *pVtx;
		VertexHandle vh = m_mesh.add_vertex(Point(v.GetX(),v.GetY(), 0.0));

		// some vertices may be repeated
		if (!m_mesh.is_valid_handle(vh))
			printf("invalid vertex");
		v.setHandle(vh);				// set index in open mesh, this id is used to add openmesh face
	}
	m_faceData.reserve(m_triangleSet.size());
	vector<const triangle*> singularTris;
	for (tIterator pTri = m_triangleSet.begin(); pTri != m_triangleSet.end(); ++pTri)
	{
		const triangle& tri = *pTri;
		const Vertex* pVtx[3];
		tri.GetVertex(pVtx[0], pVtx[1], pVtx[2]);

		// ignore invalid vertex
		bool isAllValid = true;
		for (int i = 0; i < 3; ++i)
			isAllValid &= m_mesh.is_valid_handle(pVtx[i]->handle());
		if (!isAllValid)
		{
			qDebug("some handles are invalid.");
			continue;
		}
#ifndef PRO_DELAUNAY
		// guarantee correct vertex order
		double z = tri.ComputeZVector();
		FaceHandle fh;
		if (z > 0)
		{
			fh = m_mesh.add_face(pVtx[0]->handle(), pVtx[1]->handle(), pVtx[2]->handle());
		}
		else if (z < 0)
		{
			fh = m_mesh.add_face(pVtx[0]->handle(), pVtx[2]->handle(), pVtx[1]->handle());
		}
#else
		FaceHandle fh = m_mesh.add_face(pVtx[0]->handle(), pVtx[1]->handle(), pVtx[2]->handle());
#endif
		if(!m_mesh.is_valid_handle(fh))
		{
			printf("add face failed !!!!\n");
			singularTris.push_back(&tri);
		}
	}
	int finalInvalidFaces = 0;
	for (int ithTri = 0; ithTri < singularTris.size(); ++ithTri)
	{
		const triangle& tri = *singularTris[ithTri];
		const Vertex* pVtx[3];
		tri.GetVertex(pVtx[0], pVtx[1], pVtx[2]);

		// try to add face to mesh
		FaceHandle fh = m_mesh.add_face(pVtx[0]->handle(), pVtx[1]->handle(), pVtx[2]->handle());
		if (!m_mesh.is_valid_handle(fh))
			fh = m_mesh.add_face(pVtx[0]->handle(), pVtx[2]->handle(), pVtx[1]->handle());

		if (!m_mesh.is_valid_handle(fh))
			finalInvalidFaces++;
	}

	printf("%d vtxs, %d faces, %d vtx to add, %d tris to add, %d invalid faces\n", 
		m_mesh.n_vertices(), m_mesh.n_faces(), m_vertexSet.size(), m_triangleSet.size(), finalInvalidFaces);

	int nVtx = m_mesh.n_vertices();
	m_vertexData.resize(nVtx);
	// set openmesh handle	
	for (int ithContour = 0; ithContour < m_contourList.size(); ++ithContour)
	{
		Contour& c = m_contourList[ithContour];
		int n = c.m_vtxList.size();
		c.m_handleList.resize(n);
		for (int ithVtx = 0; ithVtx < n; ++ithVtx)
		{
			VtxPoint& thisPnt = c.m_vtxList[ithVtx];
			vertexSet::iterator pThisVtx = m_vertexSet.find(thisPnt);

			if (pThisVtx == m_vertexSet.end())
			{
				qDebug("some points not found!");
				continue;
			}

			VertexHandle vtxHandle = pThisVtx->handle();
			c.m_handleList[ithVtx] = vtxHandle;
			int handleIdx = vtxHandle.idx();
			m_vertexData[handleIdx].m_vertexType = CustomVertexData::VTX_CONTOUR;
		}

		// sample handle list
		// first, find all valid handles
		struct HandleStruct{VertexHandle m_h; int m_idx;};
		vector<HandleStruct> validHandles;
		validHandles.reserve(c.m_handleList.size());
		for (int ithVtx = 0; ithVtx < c.m_handleList.size(); ++ithVtx)
		{
			VertexHandle& vh = c.m_handleList[ithVtx];
			if (!m_mesh.is_valid_handle(vh) || m_mesh.is_boundary(vh) ||
				 m_mesh.is_isolated(vh) || !m_mesh.is_manifold(vh))
				continue;
			HandleStruct hs;
			hs.m_h = vh;
			hs.m_idx = ithVtx;
			validHandles.push_back(hs);
		}

		// sample valid handles
		int step = max(int(validHandles.size() / (m_nRouteEntries * 2)), 1);
		vector<EntryPnt>* seedHandles[2] = {&c.m_srcEntryList, &c.m_tarEntryList};
		int ithSeed = 0;
		for (int ithVtx = 0; ithVtx < validHandles.size(); ithVtx += step,ithSeed++)
		{
			int idx = validHandles[ithVtx].m_idx;
			VtxPoint normal;
			VtxPoint& thisPnt = c.m_vtxList[idx];
			VtxPoint& nextPnt = c.m_vtxList[(idx+1) % n];
			VtxPoint& prevPnt = c.m_vtxList[(idx+n-1) % n];
			VtxPoint::computeNormal(prevPnt, thisPnt, nextPnt, normal);
			seedHandles[ithSeed%2]->push_back(EntryPnt(validHandles[ithVtx].m_h, normal));
		}
		if (c.m_srcEntryList.size() == 0 || c.m_tarEntryList.size() == 0)
		{
			if (validHandles.size() != 0)
			{
				for (int i = 0; i < validHandles.size(); i ++)
				{

					int idx = validHandles[i].m_idx;
					VtxPoint normal;
					VtxPoint& thisPnt = c.m_vtxList[idx];
					VtxPoint& nextPnt = c.m_vtxList[(idx+1) % n];
					VtxPoint& prevPnt = c.m_vtxList[(idx+n-1) % n];
					VtxPoint::computeNormal(prevPnt, thisPnt, nextPnt, normal);

					c.m_srcEntryList.push_back(EntryPnt(validHandles[i].m_h, normal));
					c.m_tarEntryList.push_back(EntryPnt(validHandles[i].m_h, normal));
				}
			}
			else
				qDebug("no valid handles!!!");
		}
	}

	REAL totalEdgeLength = 0;
	for (DelaunayMesh::EdgeIter pEdge = m_mesh.edges_begin();
		 pEdge != m_mesh.edges_end(); ++pEdge)
	{
		HalfedgeHandle h = m_mesh.halfedge_handle(pEdge.handle(), 0);
		VertexHandle v0 = m_mesh.from_vertex_handle(h);
		VertexHandle v1 = m_mesh.to_vertex_handle(h);
		Point& p0 = m_mesh.point(v0);
		Point& p1 = m_mesh.point(v1);
		totalEdgeLength += (p0 - p1).norm();
	}

	m_laneWidth = totalEdgeLength / m_mesh.n_edges() * m_laneWidthRatio;

	int nEdge= m_mesh.n_edges();
	int nHEdge= m_mesh.n_halfedges();
	int nFace= m_mesh.n_faces();
	m_edgeData.resize(nEdge);
	m_halfEdgeData.resize(nHEdge);
	m_faceData.resize(nFace);

}


void DelaunayRouter::clearAllTags( int vtxValue /*= 0*/, int edgeValue /*= 0*/, int halfEdgeValue, int faceValue /*= 0*/ )
{
	for (int i = 0; i < m_vertexData.size(); ++i)
	{
		m_vertexData[i].m_tempTag = vtxValue;
	}
	for (int i = 0; i < m_edgeData.size(); ++i)
	{
		m_edgeData[i].m_tempTag = edgeValue;
	}
	for (int i = 0; i < m_faceData.size(); ++i)
	{
		m_faceData[i].m_tempTag = faceValue;
		m_faceData[i].m_tempID0 = faceValue;
		m_faceData[i].m_tempID0 = faceValue;
	}
	for (int i = 0; i < m_halfEdgeData.size(); ++i)
	{
		m_halfEdgeData[i].m_tempTag = halfEdgeValue;
	}
}

bool DelaunayRouter::findEdgeRouteBFS( const VertexHandle src, const VertexHandle tar, vector<HalfedgeHandle>& route )
{
	if (!m_mesh.is_valid_handle(src) || !m_mesh.is_valid_handle(tar))
		return false;

	route.clear();
	// try simple look up first, which can be used in most cases
	HalfedgeHandle routeEdge = m_mesh.find_halfedge(src, tar);
	if (m_mesh.is_valid_handle(routeEdge))
	{
		route.push_back(routeEdge);
		return true;
	}

	printf("noDirectEdge ");

	// do BFS search
	clearAllTags();
	int nVtx = m_mesh.n_vertices();

	set<VtxInfo> vtxSet0, vtxSet1;
	set<VtxInfo>* srcHandles = &vtxSet0;
	set<VtxInfo>* tarHandles = &vtxSet1;
	srcHandles->insert(tar);
	m_vertexData[tar.idx()].m_tempTag = 1;

	Point srcPnt = m_mesh.point(src);
	Point tarPnt = m_mesh.point(tar);

	// breath first traversal from tar vertex
	bool isSrcFound = false;
	for (int ithNei = 0; srcHandles->size() != 0;++ithNei)
	{
		for (set<VtxInfo>::iterator pVtx = srcHandles->begin(); 
			 pVtx != srcHandles->end(); ++pVtx)
		{
			VertexHandle vh = pVtx->m_vtxHandle;
			if (!m_mesh.is_valid_handle(vh) || m_mesh.is_isolated(vh))
				continue;

			Point thisPnt = m_mesh.point(vh);
			for(VertexVertexIter pVtx = m_mesh.vv_begin(vh); 
				pVtx != m_mesh.vv_end(vh); ++pVtx)
			{
				VertexHandle neighVH = pVtx.handle();
				if (!m_mesh.is_valid_handle(neighVH))
					continue;
				int idx = neighVH.idx();

				// ignore visited vertex
				if (m_vertexData[idx].m_tempTag != 0)
					continue;
				m_vertexData[idx].m_tempTag = 1;									// mark visited

				// ignore vertices already passed
				if (m_vertexData[idx].m_tempNInContourEdge && neighVH != src)
					continue;

				// ignore invalid half edge
				HalfedgeHandle he = m_mesh.find_halfedge( neighVH, vh );
				if (!m_mesh.is_valid_handle(he))
					continue;
				m_vertexData[idx].m_tempEdge = he;									// store half edge, src -> tar

				Point neiPnt = m_mesh.point(neighVH);
				OpenMesh::Vec3d srcDp = (srcPnt - neiPnt).normalize();							
				OpenMesh::Vec3d tarDp = (tarPnt - neiPnt).normalize();
				double length = srcDp.data()[0]*tarDp.data()[0] + srcDp.data()[1]*tarDp.data()[1]; // inner product
				tarHandles->insert(VtxInfo(neighVH, length));						// add to tar vtx array

				if (neighVH == src)
				{
					isSrcFound = true;
					goto PROCESS_ROUTE;
				}
			}
		}
		set<VtxInfo>* tmp = srcHandles;
		srcHandles = tarHandles;
		tarHandles = tmp;
		tarHandles->clear();
	}
PROCESS_ROUTE:

	if (!isSrcFound)
		return false;
	// route 
	for (VertexHandle v = src; v != tar;)
	{
		HalfedgeHandle he = m_vertexData[v.idx()].m_tempEdge;
		route.push_back(he);
		v = m_mesh.to_vertex_handle(he);
	}
	return true;
}

void DelaunayRouter::markContourHalfedge()
{
	for (int ithContour = 0; ithContour < m_contourList.size(); ++ithContour)
	{
		Contour& c = m_contourList[ithContour];
		vector<HalfedgeHandle> contour;

		// a contour must be simple (not itersect with itself)
		int resultTag = 0;
		const int notSimpleContourTag = 0x1;
		const int routeNotFoundTag    = 0x2;
		const int complexEdgeTag      = 0x4;
		for (int ithVtx = 0; ithVtx < c.m_vtxList.size(); ++ithVtx)
		{
			int nextID = (ithVtx+1) % c.m_vtxList.size();
			VertexHandle srcHandle = c.m_handleList[ithVtx];
			VertexHandle tarHandle = c.m_handleList[nextID];
			if (srcHandle == tarHandle)
				continue;

			vector<HalfedgeHandle> route;
			bool res = findEdgeRouteBFS(srcHandle, tarHandle, route);
			if (!res)
			{
				resultTag |= routeNotFoundTag;
				printf("route of %dth pnt in %dth contour not found\n", ithVtx, ithContour);
				goto RECORD_CONTOUR;
			}
			for (int ithEdge = 0; ithEdge < route.size(); ++ithEdge)
			{
				VertexHandle vtx = m_mesh.from_vertex_handle(route[ithEdge]);
				HalfedgeHandle ohe = m_mesh.opposite_halfedge_handle(route[ithEdge]);

				// ignore edges without any contribution to the internal area
				if (ithEdge < route.size()-1 && ohe == route[ithEdge+1])
				{
					++ithEdge;
					resultTag |= complexEdgeTag;
					continue;
				}

				// encounter visited vertex, so this isn't a simple contour
				if (m_vertexData[vtx.idx()].m_tempNInContourEdge != 0)
				{
					resultTag |= notSimpleContourTag;
					printf("%dth contour is not simple\n", ithContour);
					goto RECORD_CONTOUR;
				}
				m_vertexData[vtx.idx()].m_tempNInContourEdge++;
				contour.push_back(route[ithEdge]);
			}
		}

RECORD_CONTOUR:
		// clear tag, in order to prepare for next contour
		for (int ithEdge = 0; ithEdge < contour.size(); ++ithEdge)
		{
			VertexHandle vtx = m_mesh.to_vertex_handle(contour[ithEdge]);
			m_vertexData[vtx.idx()].m_tempNInContourEdge = 0;
		}
		if ((resultTag & (routeNotFoundTag | notSimpleContourTag)) == 0)
		{
			for (int ithEdge = 0; ithEdge < contour.size(); ++ithEdge)
			{
				m_halfEdgeData[contour[ithEdge].idx()].m_isContourEdge = true;
			}
		}
	}
}



bool DelaunayRouter::markInternalFaces()
{
	clearAllTags();
	vector<FaceHandle> faceBuf0;
	for (HalfedgeIter pHE = m_mesh.halfedges_begin(); pHE != m_mesh.halfedges_end(); ++pHE)
	{
		HalfedgeHandle thisHe = pHE.handle();
		HalfedgeHandle oppoHe = m_mesh.opposite_halfedge_handle(thisHe);
		int thisIdx = thisHe.idx();
		int oppoIdx = oppoHe.idx();

		if (m_halfEdgeData[thisIdx].m_isContourEdge == true &&
			m_halfEdgeData[oppoIdx].m_isContourEdge == false &&
			m_mesh.is_boundary(thisHe) == false &&
			m_mesh.is_boundary(oppoHe) == false)
		{
			FaceHandle face = m_mesh.face_handle(thisHe);
			faceBuf0.push_back(face);
		}
	}

	vector<FaceHandle> faceBuf1;
	vector<FaceHandle>* pSeedBuf = &faceBuf0, *pNextBuf = &faceBuf1;

	while (pSeedBuf->size())
	//for (int i = 0; i < 3; ++i)
	{
		for (int ithFace = 0; ithFace < pSeedBuf->size(); ++ithFace)
		{
			FaceHandle face = (*pSeedBuf)[ithFace];
			m_faceData[face.idx()].m_tempTag = 1;
			m_faceData[face.idx()].m_isInternalFace = true;
			for (FaceHalfedgeIter pHE = m_mesh.fh_begin(face); pHE != m_mesh.fh_end(face); ++pHE)
			{
				HalfedgeHandle he = pHE.handle();
				HalfedgeHandle ohe= m_mesh.opposite_halfedge_handle(he);
				int heIdx = he.idx();
				int oheIdx = ohe.idx();

				// stop at marked edge border
				if (m_mesh.is_boundary(he) || 
					m_mesh.is_boundary(ohe) ||
					m_halfEdgeData[heIdx].m_isContourEdge ||
					m_halfEdgeData[oheIdx].m_isContourEdge)
					continue;		

				// add adjacent unvisited face
				FaceHandle adjFace = m_mesh.face_handle(ohe);
				if (m_faceData[adjFace.idx()].m_tempTag == 1)
					continue;
				m_faceData[adjFace.idx()].m_tempTag = 1;
				(*pNextBuf).push_back(adjFace);
			}
		}

		// swap face buffer
		vector<FaceHandle>* tmp = pSeedBuf;
		pSeedBuf = pNextBuf;
		pNextBuf = tmp;
		(*pNextBuf).clear();
	}
	return true;
}

bool DelaunayRouter::findFaceRouteBFS(const vector<VertexHandle> srcSet, 
									 const vector<VertexHandle> tarSet, 
									 vector<FaceHandle>& route,
									 bool considerFaceBlock,
									 int* pSrcVtxID,
									 int* pTarVtxID)
{
	clearAllTags();
	const int srcVtxTag  = 1;
	const int tarVtxTag  = 1 << 1;
	const int tarFaceTag = 1;
	const int srcFaceTag = 1<<1;
	const int visitedFaceTag = 1 << 2;

	const int nFaces = m_mesh.n_faces();
	// mark legal faces around src and tar point set
	vector<FaceHandle> faceBuf;
	faceBuf.resize(nFaces*2);
	FaceHandle *pSrcFace = &faceBuf[0], *pTarFace = &faceBuf[nFaces];
	int nSrcFace = 0, nTarFace = 0;

	// mark src faces
	for (int ithSrc = 0; ithSrc < srcSet.size(); ++ithSrc)
	{
		VertexHandle src = srcSet[ithSrc];
		m_vertexData[src.idx()].m_tempTag |= srcVtxTag;

		if (m_mesh.is_isolated(src) || !m_mesh.is_manifold(src))
			continue;

		for (VertexOHalfedgeIter pEdge = m_mesh.voh_begin(src); pEdge != m_mesh.voh_end(src); ++pEdge)
		{
			HalfedgeHandle eh = pEdge.handle();
			if (!m_mesh.is_valid_handle(eh))
				continue;
			FaceHandle fh = m_mesh.face_handle(eh);
			if (!m_mesh.is_valid_handle(fh))
				continue;
			if (considerFaceBlock && m_faceData[fh.idx()].m_isInternalFace == true)
				continue;
			m_faceData[fh.idx()].m_tempTag |= srcFaceTag;
			m_faceData[fh.idx()].m_tempID0 = ithSrc;
		}
	}

	// mark tar faces
	for (int ithTar = 0; ithTar < tarSet.size(); ++ithTar)
	{
		VertexHandle tar = tarSet[ithTar];
		m_vertexData[tar.idx()].m_tempTag |= tarVtxTag;
		if (m_mesh.is_isolated(tar) || !m_mesh.is_manifold(tar))
			continue;

		for (VertexOHalfedgeIter pEdge = m_mesh.voh_begin(tar); pEdge != m_mesh.voh_end(tar); ++pEdge)
		{
			HalfedgeHandle eh = pEdge.handle();
			if (!m_mesh.is_valid_handle(eh))
				continue;
			FaceHandle fh = m_mesh.face_handle(eh);
			if (!m_mesh.is_valid_handle(fh))
				continue;
			if (considerFaceBlock && m_faceData[fh.idx()].m_isInternalFace == true)
				continue;
			m_faceData[fh.idx()].m_tempTag |= tarFaceTag;
			m_faceData[fh.idx()].m_tempID1 = ithTar;
			pSrcFace[nSrcFace++] = fh;
		}
	}

	// breath first search
	FaceHandle lastFace;
	for (int ithIter = 0; nSrcFace != 0; ++ithIter)
	{
		for (int ithFace = 0; ithFace < nSrcFace; ++ithFace)
		{
			FaceHandle& fh = pSrcFace[ithFace];

			// add neighbour faces
			for (FaceHalfedgeIter pEdge = m_mesh.fh_begin(fh); pEdge != m_mesh.fh_end(fh); ++pEdge)
			{
				HalfedgeHandle heH = pEdge.handle();
				if (!m_mesh.is_valid_handle(heH) || m_mesh.is_boundary(heH))
					continue;
				HalfedgeHandle oheH = m_mesh.opposite_halfedge_handle(heH);
				if (!m_mesh.is_valid_handle(oheH) || m_mesh.is_boundary(oheH))
					continue;
				FaceHandle nearFh = m_mesh.face_handle(oheH);
				if (!m_mesh.is_valid_handle(nearFh))
					continue;

				int nearFhIdx = nearFh.idx();
				int nearTag = m_faceData[nearFhIdx].m_tempTag;

				if (nearTag & visitedFaceTag)
					continue;
				if (considerFaceBlock && m_faceData[nearFhIdx].m_isInternalFace == true)
					continue;

				// encounter faces around src, break
				m_faceData[nearFhIdx].m_tempFace = fh;
				if (nearTag & srcFaceTag)
				{
					lastFace = nearFh;
					printf("%d iterations\n", ithIter);
					goto FOUND_ROUTE;
				}
				m_faceData[nearFh.idx()].m_tempTag |= visitedFaceTag;
				pTarFace[nTarFace++] = nearFh;
			}
		}
		// swap buffer
		FaceHandle* tmp = pSrcFace;
		pSrcFace = pTarFace;
		pTarFace = tmp;
		nSrcFace = nTarFace;
		nTarFace = 0;
	}
	// no route found
	if (!m_mesh.is_valid_handle(lastFace))
		return false;
FOUND_ROUTE:

	// record route
	// at least one face added
	for (FaceHandle fh = lastFace;;fh = m_faceData[fh.idx()].m_tempFace)
	{
		route.push_back(fh);
		if (m_faceData[fh.idx()].m_tempTag & tarFaceTag)
			break;
	}
	// set src and tar vtx
	if (pSrcVtxID)
	{
		*pSrcVtxID = m_faceData[route[0].idx()].m_tempID0;
	}
	if (pTarVtxID)
	{
		*pTarVtxID = m_faceData[route.back().idx()].m_tempID1;
	}
	return true;
}

bool DelaunayRouter::getRoute( int src, int tar, vector<VtxPoint>& route , bool considerBlockFace )
{
	if (src < 0 || tar < 0 ||
		src >= m_contourList.size() || tar >= m_contourList.size())
		return false;

	clock_t t0 = clock();
	Contour& srcCtr = m_contourList[src];
	Contour& tarCtr = m_contourList[tar];
	vector<FaceHandle> faceRoute;
	int srcVtxID = -1, tarVtxID = -1;
	vector<VertexHandle> srcSeedHandles, tarSeedHandles;

	srcSeedHandles.resize(srcCtr.m_srcEntryList.size());
	tarSeedHandles.resize(tarCtr.m_tarEntryList.size());
	for (int i = 0; i < srcCtr.m_srcEntryList.size(); ++i)
		srcSeedHandles[i] = srcCtr.m_srcEntryList[i].m_vtxHandle;
	for (int i = 0; i < tarCtr.m_tarEntryList.size(); ++i)
		tarSeedHandles[i] = tarCtr.m_tarEntryList[i].m_vtxHandle;

	bool res = findFaceRouteBFS(srcSeedHandles,
								tarSeedHandles,
								faceRoute,
								considerBlockFace,
								&srcVtxID, &tarVtxID);
	clock_t t1 = clock();

	if (!res || srcVtxID == -1 || tarVtxID == -1)
		return false;

	EntryPnt& srcEntry = srcCtr.m_srcEntryList[srcVtxID];
	EntryPnt& tarEntry = tarCtr.m_tarEntryList[tarVtxID];

	srcEntry.m_count++;
	tarEntry.m_count++;
	Point srcPnt = m_mesh.point(srcEntry.m_vtxHandle);
	Point tarPnt = m_mesh.point(tarEntry.m_vtxHandle);

	VtxPoint srcVtx(srcPnt.data()[0], srcPnt.data()[1]);
	VtxPoint tarVtx(tarPnt.data()[0], tarPnt.data()[1]);
	VtxPoint&srcNor = srcEntry.m_normal;
	VtxPoint&tarNor = tarEntry.m_normal;

	vector<EdgeRoute> edgeList;
	edgeList.reserve(faceRoute.size());
	for (int ithFace = 0; ithFace < faceRoute.size()-1; ithFace += m_routeStepSize)
	{
		FaceHandle thisFaceHanle = faceRoute[ithFace];
		for (FaceHalfedgeIter pNearH = m_mesh.fh_begin(thisFaceHanle); pNearH != m_mesh.fh_end(thisFaceHanle); ++pNearH)
		{
			HalfedgeHandle edgeHandle = pNearH.handle();
			if (!m_mesh.is_valid_handle(edgeHandle) || m_mesh.is_boundary(edgeHandle) )
				continue;
			HalfedgeHandle oppoEdgeHandle = m_mesh.opposite_halfedge_handle(edgeHandle);
			if (!m_mesh.is_valid_handle(oppoEdgeHandle) || m_mesh.is_boundary(oppoEdgeHandle) )
				continue;
			if (m_mesh.face_handle(oppoEdgeHandle) == faceRoute[ithFace+1])
			{
				VertexHandle vh0 = m_mesh.from_vertex_handle(edgeHandle);
				VertexHandle vh1 = m_mesh.to_vertex_handle(edgeHandle);
				
				EdgeRoute e={m_mesh.point(vh0), m_mesh.point(vh1), vh0.idx(), vh1.idx()};
				edgeList.push_back(e);
			}
		}
	}

	route.clear();
	route.reserve(faceRoute.size()+2);
	route.push_back(srcVtx);
	route.push_back(srcVtx);
	int nEdge = edgeList.size();


#ifndef BIDIRECTIONAL_ROUTE
	Point routeEndPnt(srcVtx.X, srcVtx.Y, 0);
	for (int ithEdge = 0; ithEdge < nEdge-1; ++ithEdge)
	{
		EdgeRoute& edge = edgeList[ithEdge];
		EdgeRoute& nextEdge = edgeList[ithEdge+1];

		Point endPoint[2] = {(edge.p1 + nextEdge.p1) * 0.5, (edge.p2 + nextEdge.p2) * 0.5};
		Point dP = (endPoint[1] - endPoint[0]);
		REAL dPLength = dP.norm();
		REAL dPLengthRatio = dPLength / m_laneWidth;
		if (dPLengthRatio > 2.0)
		{
			if (m_vertexData[edge.idx1].m_vertexType != m_vertexData[edge.idx2].m_vertexType || 1)
			{
#ifndef MULTI_SEGMENT
				int otherIdx = (m_vertexData[vh0.idx()].m_vertexType == CustomVertexData::VTX_CONTOUR);
				int contourIdx = 1 - otherIdx;
				REAL factor = contourIdx ? -1.f : 1.f;
				endPoint[otherIdx] = endPoint[contourIdx] + factor * dP * (m_laneWidth / dPLength);
#else
				// project current pnt to next edge			
				routeEndPnt = Point(route.back().X, route.back().Y, 0);	
				dP /= dPLength;
				REAL t = ((routeEndPnt - endPoint[0]) | dP) / m_laneWidth;
				t = max(min((int)t, int(dPLengthRatio-1)), 0);
				//t = max(min(t, int(dPLengthRatio)-REAL_EPSILON), 0.0);

				//routeEndPnt = endPoint[0] + dP * (m_laneWidth * t);
				//t = (int)t;

				Point newEndPnt0 = endPoint[0] + dP * (m_laneWidth * t);
				Point newEndPnt1 = endPoint[0] + dP * (m_laneWidth * (t+1));
				endPoint[0] = newEndPnt0;
				endPoint[1] = newEndPnt1;
#endif
			}
			else
			{
				dP /= dPLength;
				VtxPoint& routeEndVtx = route.back();
				VtxPoint  d = tarVtx - routeEndVtx;
				VtxPoint dEndVtx(routeEndVtx.X - endPoint[0][0], routeEndVtx.Y - endPoint[0][1]);
				REAL denominator = dP[1] * d.X - dP[0] * d.Y;
				REAL numerator0   = d.X * dEndVtx.Y - d.Y * dEndVtx.X;
				REAL numerator1   = dP[0] * dEndVtx.Y - dP[1] * dEndVtx.X;
				REAL t;
				t = numerator0 / denominator / m_laneWidth;
				//t = max(min((int)t, (int)dPLengthRatio-1), 0);
				if (t < 0 || t > dPLengthRatio || numerator1 < 0)
				{
// 							REAL dot = d.X * dP[0] + d.Y * dP[1];
// 							t = dot > 0 ? (int)dPLengthRatio-1 : 0;
					Point routeEndPnt(routeEndVtx.X, routeEndVtx.Y,0);
					t = ((routeEndPnt - endPoint[0]) | dP) / m_laneWidth;
				}
				t = max(min((int)t, (int)dPLengthRatio-1), 0);
				Point newEndPnt0 = endPoint[0] + dP * (m_laneWidth * t);
				Point newEndPnt1 = endPoint[0] + dP * (m_laneWidth * (t+1));
				endPoint[0] = newEndPnt0;
				endPoint[1] = newEndPnt1;
			}
		}
		Point edgePnt = endPoint[0] * (1.0 - m_laneOffsetRatio)  + endPoint[1] * m_laneOffsetRatio;
		route.push_back(VtxPoint(edgePnt.data()[0], edgePnt.data()[1]));
	}
#else
	vector<float> tList[2];
	VtxPoint routeEndVtx = srcVtx;
	for (int ithEdge = 0; ithEdge < nEdge-1; ++ithEdge)
	{
		EdgeRoute& edge = edgeList[ithEdge];
		EdgeRoute& nextEdge = edgeList[ithEdge+1];

		Point endPoint[2] = {(edge.p1 + nextEdge.p1) * 0.5, (edge.p2 + nextEdge.p2) * 0.5};
		Point dP = (endPoint[1] - endPoint[0]);
		REAL dPLength = dP.norm();
		REAL dPLengthRatio = dPLength / m_laneWidth;

		// project current pnt to next edge
		Point routeEndPnt(routeEndVtx.X, routeEndVtx.Y,0);
		dP /= dPLength;
		REAL t = ((routeEndPnt - endPoint[0]) | dP) / m_laneWidth;
		t = max(min(t, dPLengthRatio - REAL_EPSILON), 0.0);
		//t = max(min((int)t, int(dPLengthRatio-1)), 0);
		Point edgePnt = endPoint[0] + dP * (m_laneWidth * t);
		routeEndVtx.X = edgePnt[0];
		routeEndVtx.Y = edgePnt[1];
		tList[0].push_back(t);
	}
	routeEndVtx = tarVtx;
	for (int ithEdge = nEdge-1; ithEdge > 0; --ithEdge)
	{
		EdgeRoute& edge = edgeList[ithEdge];
		EdgeRoute& nextEdge = edgeList[ithEdge-1];

		Point endPoint[2] = {(edge.p1 + nextEdge.p1) * 0.5, (edge.p2 + nextEdge.p2) * 0.5};
		Point dP = (endPoint[1] - endPoint[0]);
		REAL dPLength = dP.norm();
		REAL dPLengthRatio = dPLength / m_laneWidth;

		// project current pnt to next edge
		Point routeEndPnt(routeEndVtx.X, routeEndVtx.Y,0);
		dP /= dPLength;
		REAL l = dP.norm();
		REAL t = ((routeEndPnt - endPoint[0]) | dP) / m_laneWidth;
		t = max(min(t, dPLengthRatio - REAL_EPSILON), 0.0);
		//t = max(min((int)t, int(dPLengthRatio-1)), 0);
		Point edgePnt = endPoint[0] + dP * (m_laneWidth * t);
		routeEndVtx.X = edgePnt[0];
		routeEndVtx.Y = edgePnt[1];
		tList[1].push_back(t);
	}

	for (int ithEdge = 0; ithEdge < nEdge-1; ++ithEdge)
	{
		EdgeRoute& edge = edgeList[ithEdge];
		EdgeRoute& nextEdge = edgeList[ithEdge+1];

		Point endPoint[2] = {(edge.p1 + nextEdge.p1) * 0.5, (edge.p2 + nextEdge.p2) * 0.5};
		Point dP = (endPoint[1] - endPoint[0]);
		REAL dPLength = dP.norm();

		dP /= dPLength;
		float weight = nEdge-2 <= 0 ? 0.5 : (float)ithEdge / (nEdge-2);		
		weight = 0.5;
		float t = tList[0][ithEdge] * (1-weight) + tList[1][nEdge-2-ithEdge] * weight;
		t = (int)t;
		Point newEndPnt0 = endPoint[0] + dP * (m_laneWidth * t);
		Point newEndPnt1 = endPoint[0] + dP * (m_laneWidth * (t+1));
		Point edgePnt = newEndPnt0 * (1.0 - m_laneOffsetRatio)  + newEndPnt1 * m_laneOffsetRatio;
		route.push_back(VtxPoint(edgePnt[0], edgePnt[1]));
	}
#endif
	route[1] = route[1] + srcNor * (srcCtr.m_radius * m_endPointNormalRatio);
	route.push_back(tarVtx + tarNor * (tarCtr.m_radius * m_endPointNormalRatio));
	route.push_back(tarVtx);

	smooth(route, m_smoothFactor, m_smoothIter);
	clock_t t2 = clock();
	printf("find route BFS: %dms\n",t1-t0);
	printf("assemble route: %dms\n",t2-t1);
	return true;
}

void DelaunayRouter::processEdgeRoute(const vector<EdgeRoute>& edges,
									  vector<Point>& p1List,
									  vector<Point>& p2List)
{
	p1List.resize(edges.size());
	p2List.resize(edges.size());
	for (int i = 0; i < edges.size(); ++i)
	{
		const EdgeRoute& e = edges[i];
		Point edgeDir = e.p2 - e.p1;
		float edgeLength = edgeDir.norm();
		edgeDir /= edgeLength;

		Point p1 = e.p1;
		Point p2 = e.p2;
		float lanePadding = m_laneWidth * 0.5f;
		float padding = 0;
		if (edgeLength <= m_laneWidth)
			lanePadding = edgeLength / 3;
		else if (edgeLength - 20 * 2 >= m_laneWidth)
			padding = 20;
		float totPadding = lanePadding + padding;

		p1List[i] = e.p1 + edgeDir * totPadding;
		p2List[i] = e.p2 - edgeDir * totPadding;		
	}
}
void DelaunayRouter::routeByEdge(const Point& srcEntry, const Point& tarEntry, 
								 const Point& src, const Point& tar,
								 vector<Point>& p1List,
								 vector<Point>& p2List,
								 vector<VtxPoint>& route)
{
	route.clear();
	route.push_back(VtxPoint(srcEntry[0], srcEntry[1]));
	route.push_back(VtxPoint(src[0], src[1]));

	Point begPnt = src;

	// find farest way point
	int startEdge = 0;
	float angleMin = -M_PI, angleMax = M_PI;
	Point* pntMin = NULL, *pntMax = NULL;
	int wayPntIdx = -1;
	for (int ithEdge = startEdge; ithEdge < p1List.size(); ++ithEdge)
	{
		Point dP1 = p1List[ithEdge] - begPnt;
		Point dP2 = p2List[ithEdge] - begPnt;
		float angP[2] = {atan2(dP1[1], dP1[0]), atan2(dP2[1], dP2[0])};
		int minMaxIdx[2] = {0,1};
		if (angP[0] > angP[1])
		{
			minMaxIdx[0] = 1;
			minMaxIdx[1] = 0;
		}


	}
}
void DelaunayRouter::reset()
{
	m_contourList.clear();
	m_vertexSet.clear();
	m_triangleSet.clear();
	m_vertexData.clear();
	m_edgeData.clear();
	m_halfEdgeData.clear();
	m_faceData.clear();
	m_mesh.clear();
}

#ifdef GDI_DRAWFUNCTION
void DelaunayRouter::drawTriangles( CDC& dc, REAL hScale, REAL vScale ) 
{
	CBrush triBrush0, triBrush1, triBrush2;
	triBrush0.CreateSolidBrush(RGB(255,0,0));
	triBrush1.CreateSolidBrush(RGB(0,0,255));
	triBrush2.CreateSolidBrush(RGB(0,255,0));
	for(triangleSet::const_iterator pTri = m_triangleSet.begin();
		pTri != m_triangleSet.end(); ++pTri)
	{
		const triangle& tri = *pTri;
		const Vertex * v0 = tri.GetVertex(0);
		const Vertex * v1 = tri.GetVertex(1);
		const Vertex * v2 = tri.GetVertex(2);
		double z = tri.ComputeZVector();
		if (z > 0)
			dc.SelectObject(triBrush0);
		else if(z < 0)
			dc.SelectObject(triBrush1);
		else
			dc.SelectObject(triBrush2);

		POINT p[3];
		p[0].x = Int(hScale * v0->GetX());
		p[0].y = Int(vScale * v0->GetY());
		p[1].x = Int(hScale * v1->GetX());
		p[1].y = Int(vScale * v1->GetY());
		p[2].x = Int(hScale * v2->GetX());
		p[2].y = Int(vScale * v2->GetY());
		//dc.Polygon(p,3);
	}

	for (FaceIter pFace = m_mesh.faces_begin(); pFace != m_mesh.faces_end(); ++pFace)
	{
		FaceHandle f = pFace.handle();
		POINT p[3];
		int ithPnt = 0;
		for (FaceVertexIter pVtx = m_mesh.fv_begin(f); pVtx != m_mesh.fv_end(f); ++pVtx, ++ithPnt)
		{
			Point pnt = m_mesh.point(pVtx.handle());
			p[ithPnt].x = Int(hScale * pnt.data()[0]);
			p[ithPnt].y = Int(vScale * pnt.data()[1]);
		}
		if (m_faceData[f.idx()].m_isInternalFace)
		{
			dc.SelectObject(triBrush0);
		}
		else
			dc.SelectObject(triBrush1);
		dc.Polygon(p,3);
	}
	CPen borderPen;
	borderPen.CreatePen(PS_SOLID,2,RGB(255,255,0));
	dc.SelectObject(borderPen);
	for (EdgeIter pEdge = m_mesh.edges_begin(); pEdge != m_mesh.edges_end(); ++pEdge)
	{
		EdgeHandle eh = pEdge.handle();
		HalfedgeHandle h0 = m_mesh.halfedge_handle(eh, 0);
		HalfedgeHandle h1 = m_mesh.halfedge_handle(eh, 1);
		if (m_halfEdgeData[h0.idx()].m_isContourEdge ||
			m_halfEdgeData[h1.idx()].m_isContourEdge)
		{
			VertexHandle v0 = m_mesh.from_vertex_handle(h0);
			VertexHandle v1 = m_mesh.to_vertex_handle(h0);
			Point p0 = m_mesh.point(v0);
			Point p1 = m_mesh.point(v1);
			dc.MoveTo(Int(hScale * p0.data()[0]), Int(vScale * p0.data()[1]));
			dc.LineTo(Int(hScale * p1.data()[0]), Int(vScale * p1.data()[1]));
		}
	}

	int nContour = m_contourList.size();
	int srcContour = rand() % nContour;
	int tarContour = rand() % nContour;
	vector<VtxPoint> routePnts;
	getRoute(srcContour, tarContour, routePnts, true);
	if (!routePnts.size())
	{
		getRoute(srcContour, tarContour, routePnts, false);
	}
	if (routePnts.size())
	{ 
		CDC* pDC = &dc;
		CPen routePen;
		routePen.CreatePen(PS_SOLID,2,RGB(0,255,0));
		dc.SelectObject(routePen);
		//µ÷ÓÃDC»æÍ¼º¯Êý»æÍ¼
		for (int i = 1; i < routePnts.size(); ++i)
		{
			dc.MoveTo(Int(hScale * routePnts[i-1].X), Int(vScale * routePnts[i-1].Y));
			dc.LineTo(Int(hScale * routePnts[i].X), Int(vScale * routePnts[i].Y));
		}

		CPen pointPen;
		pointPen.CreatePen(PS_NULL,2,RGB(0,255,0));
		dc.SelectObject(pointPen);
		int dColor = 255 / routePnts.size();
		for (int i = 0; i < routePnts.size(); ++i)
		{
			int x = Int(hScale * routePnts[i].X);
			int y = Int(vScale * routePnts[i].Y);
			const int r = 3;

			CBrush pntBrush;
			pntBrush.CreateSolidBrush(RGB(dColor * i,dColor * i,dColor * i));
			dc.SelectObject(pntBrush);
			dc.Ellipse(x-r,y-r,x+r,y+r);
		}
	}

	CPen pointPen;
	pointPen.CreatePen(PS_SOLID,2,RGB(0,255,0));
	dc.SelectObject(pointPen);
	dc.Rectangle(10,10,Int(10 + hScale * m_laneWidth),Int(10 + vScale * m_laneWidth));
}

#endif
Contour::Contour( const vector<DelaunayCore::VtxPoint>& pnt )
{
	// avoid repeated end points
	if (pnt.size() > 2)
	{
		VtxPoint delta = pnt.back() - pnt[0];
		double   deltaLength = sqrt(delta.X*delta.X + delta.Y*delta.Y);
		int nPnt = deltaLength < REAL_EPSILON ? pnt.size()-1 : pnt.size();

		m_vtxList = pnt;
		m_vtxList.resize(nPnt);
	}
	m_handleList.resize(pnt.size());
}

#ifdef USE_QT
bool DelaunayCore::DelaunayRouter::addContour( const QPolygonF& contourPoly )
{
	if (contourPoly.size() <= 2)
		return false;

	m_contourList.push_back(Contour());
	Contour& con = m_contourList.back();
	
	// ignore last point when it is the same as the first one
	QPointF delta = contourPoly[0] - contourPoly.back();
	double   deltaLength = sqrt(delta.x()*delta.x() + delta.y()*delta.y());
	int nPnt = deltaLength < REAL_EPSILON ? contourPoly.size()-1 : contourPoly.size();
	
	for (int i = 0; i < nPnt; ++i)
	{
		const QPointF& pnt = contourPoly[i];
		con.m_vtxList.push_back(VtxPoint(pnt.x(), pnt.y()));
	}
	con.m_handleList.resize(contourPoly.size());
	return true;
}

bool DelaunayCore::DelaunayRouter::getContourEntries( int ithContour, 
													 QVector<QPointF>& inPoints, QVector<QPointF>& inNormals, 
													 QVector<QPointF>& outPoints, QVector<QPointF>& outNormals,
													 bool onlyRoutedEntries)
{
	if (ithContour < 0 || ithContour >= m_contourList.size())
		return false;
	Contour& c = m_contourList[ithContour];

	int nOut = c.m_srcEntryList.size();
	int nIn = c.m_tarEntryList.size();
	outPoints.reserve(nOut);
	outNormals.reserve(nOut);
	inPoints.reserve(nIn);
	inNormals.reserve(nIn);
	for (int i = 0; i < nOut; ++i)
	{
		EntryPnt& entry = c.m_srcEntryList[i];
		if (onlyRoutedEntries && entry.m_count == 0)
			continue;

		DelaunayMesh::Point& p = m_mesh.point(entry.m_vtxHandle);
		VtxPoint& n = entry.m_normal;
		outPoints.push_back(QPointF(p.data()[0], p.data()[1]));
		outNormals.push_back(QPointF(n.X, n.Y));
	}


	for (int i = 0; i < nIn; ++i)
	{
		EntryPnt& entry = c.m_tarEntryList[i];
		if (onlyRoutedEntries && entry.m_count == 0)
			continue;

		DelaunayMesh::Point& p = m_mesh.point(entry.m_vtxHandle);
		VtxPoint& n = entry.m_normal;
		inPoints.push_back(QPointF(p.data()[0], p.data()[1]));
		inNormals.push_back(QPointF(n.X, n.Y));
	}
	return true;
}
#endif

void DelaunayCore::DelaunayRouter::smooth( vector<VtxPoint>& route, double smoothFactor /*= 0.5*/, int nIter /*= 1*/ , bool preserveEndTangent)
{
	const int nCurvePnt = route.size();
	const int nCurvePnt_2 = nCurvePnt * 2;
	REAL* curvePnt = new REAL[nCurvePnt * 4];
	REAL* p = curvePnt;
	for (vector<VtxPoint>::iterator pRoute = route.begin(); pRoute != route.end(); ++pRoute, p += 2)
	{
		*p = pRoute->X;
		*(p+1) = pRoute->Y;
		*(p+nCurvePnt_2) = pRoute->X;
		*(p+nCurvePnt_2+1) = pRoute->Y;
	}

	// smooth
	int begIdx = 1, endIdx = route.size() -2;
	if (preserveEndTangent)
	{
		begIdx++;
		endIdx--;
	}

	REAL* srcP = curvePnt, *tarP = curvePnt + nCurvePnt*2;
	const int begOffset = begIdx * 2;
	const int endOffset = endIdx * 2;
	for (int ithIter = 0; ithIter < nIter; ++ithIter)
	{
		REAL* srcPnt, *tarPnt;
		for (srcPnt = srcP + begOffset, tarPnt = tarP + begOffset;
			 srcPnt <= srcP + endOffset; srcPnt += 2, tarPnt += 2)
		{
			REAL* py = srcPnt + 1;
			REAL deltaX = (*(srcPnt-2) + *(srcPnt+2)) * 0.5 - *srcPnt;
			REAL deltaY = (*(py-2) + *(py+2)) * 0.5 - *py;
			*tarPnt = *srcPnt + deltaX * smoothFactor;
			*(tarPnt+1) = *(srcPnt+1) + deltaY * smoothFactor;
		}
		REAL* tmp = srcP;
		srcP = tarP;
		tarP = tmp;
	}

	p = tarP;
	for (vector<VtxPoint>::iterator pRoute = route.begin(); pRoute != route.end(); ++pRoute, p += 2)
	{
		pRoute->X = *p;
		pRoute->Y = *(p+1);
	}
	delete[] curvePnt;
// 	for (int ithIter = 0; ithIter < nIter; ++ithIter)
// 	{
// 		for (int i = begIdx; i <= endIdx; ++i)
// 		{
// 			VtxPoint& thisPnt = route[i];
// 			VtxPoint& prevPnt = route[i-1];
// 			VtxPoint& nextPnt = route[i+1];
// 			VtxPoint  deltaDir= (prevPnt + nextPnt) * 0.5 - thisPnt;
// 			thisPnt = thisPnt + deltaDir * smoothFactor;
// 		}
// 	}
}
void DelaunayCore::DelaunayRouter::buildSafeCircle()
{
	REAL minPnt[2] = {FLT_MAX,FLT_MAX}, maxPnt[2] = {-FLT_MAX,-FLT_MAX};
	for (vertexSet::iterator pVtx = m_vertexSet.begin(); pVtx != m_vertexSet.end(); ++pVtx)
	{
		const Vertex& vtx = *pVtx;
		minPnt[0] = min(minPnt[0], vtx.GetX());
		minPnt[1] = min(minPnt[1], vtx.GetY());
		maxPnt[0] = max(maxPnt[0], vtx.GetX());
		maxPnt[1] = max(maxPnt[1], vtx.GetY());
	}
	REAL padding = m_laneWidth;

	REAL centerPnt[2] = {(minPnt[0]+maxPnt[0])*0.5, (minPnt[1]+maxPnt[1])*0.5};
	REAL dim[2] = {maxPnt[0] - minPnt[0], maxPnt[1] - minPnt[1]};
	REAL radius = sqrt(dim[0] *dim[0] + dim[1]*dim[1]) * 0.5 + padding;

	int nPnt = max((int)sqrt((REAL)m_vertexSet.size()) * 4, 12);
	REAL dAngle = M_PI * 2 / nPnt;
	REAL angle = 0;
	for (int i = 0; i < nPnt; ++i, angle += dAngle)
	{
		VtxPoint p(cos(angle) * radius + centerPnt[0], sin(angle) * radius + centerPnt[1]);
		insertNonRepeated(p);
	}
}
void DelaunayCore::DelaunayRouter::buildSafeFrame()
{
	REAL minPnt[2] = {FLT_MAX,FLT_MAX}, maxPnt[2] = {-FLT_MAX,-FLT_MAX};
	for (vertexSet::iterator pVtx = m_vertexSet.begin(); pVtx != m_vertexSet.end(); ++pVtx)
	{
		const Vertex& vtx = *pVtx;
		minPnt[0] = min(minPnt[0], vtx.GetX());
		minPnt[1] = min(minPnt[1], vtx.GetY());
		maxPnt[0] = max(maxPnt[0], vtx.GetX());
		maxPnt[1] = max(maxPnt[1], vtx.GetY());
	}

	REAL padding = m_laneWidth;// max(maxPnt[0]-minPnt[0], maxPnt[1]-minPnt[1]) * m_paddingRatio;

	int nPnt = (int)sqrt((REAL)m_vertexSet.size()) + 2;
	REAL dx = (maxPnt[0]-minPnt[0]) / (nPnt-1);
	REAL dy = (maxPnt[1]-minPnt[1]) / (nPnt-1);

	REAL x  = minPnt[0], y = minPnt[1];
	for (int ithPnt = 0; ithPnt < nPnt; ++ithPnt, x+=dx, y+=dy)
	{
		VtxPoint b(x, minPnt[1]-padding);
		VtxPoint t(x, maxPnt[1]+padding);
		VtxPoint l(minPnt[0]-padding, y);
		VtxPoint r(maxPnt[0]+padding, y);
		insertNonRepeated(b);
		insertNonRepeated(t);
		insertNonRepeated(l);
		insertNonRepeated(r);
	}
	VtxPoint x0y0(minPnt[0]-padding, minPnt[1]-padding);
	VtxPoint x1y0(maxPnt[0]+padding, minPnt[1]-padding);
	VtxPoint x0y1(minPnt[0]-padding, maxPnt[1]+padding);
	VtxPoint x1y1(maxPnt[0]+padding, maxPnt[1]+padding);
	insertNonRepeated(x0y0);
	insertNonRepeated(x1y0);
	insertNonRepeated(x0y1);
	insertNonRepeated(x1y1);
}

void DelaunayRouter::initTriangulateio(triangulateio& io)
{
	io.pointlist = NULL;
	io.pointattributelist = NULL;
	io.pointmarkerlist = NULL;
	io.numberofpoints = io.numberofpointattributes = 0;

	io.trianglelist = NULL;
	io.triangleattributelist = io.trianglearealist = NULL;
	io.neighborlist = NULL;
	io.numberoftriangles = io.numberofcorners = io.numberoftriangleattributes = 0;

	io.segmentlist = io.segmentmarkerlist = NULL;
	io.numberofsegments = 0;

	io.holelist = NULL;
	io.numberofholes = 0;

	io.regionlist = NULL;
	io.numberofregions = 0;

	io.edgelist = NULL;
	io.edgemarkerlist = NULL;
	io.normlist = NULL;
	io.numberofedges = NULL;
}
void DelaunayCore::DelaunayRouter::proDelaunay()
{
	triangulateio in, mid, vorout;
	initTriangulateio(in);
	initTriangulateio(mid);
	initTriangulateio(vorout);

	in.numberofpoints = m_vertexSet.size();
	in.numberofpointattributes = 0;
	in.pointlist = (REAL *) malloc(in.numberofpoints * 2 * sizeof(REAL));
	int ithVtx = 0;

	// insert vertices
	for (vertexSet::iterator pVtx = m_vertexSet.begin(); pVtx != m_vertexSet.end(); ++pVtx, ithVtx ++)
	{
		const Vertex& v = *pVtx;
		in.pointlist[ithVtx*2]   = v.GetX();
		in.pointlist[ithVtx*2+1] = v.GetY();
		v.m_tempFlag = ithVtx;
	}

	// mark boundary
	vector<int>segmentList, segmentMarkerList;
	for (int ithContour = 0; ithContour < m_contourList.size(); ++ithContour)
	{
		Contour& c = m_contourList[ithContour];
		for (ithVtx = 0; ithVtx < c.m_vtxList.size(); ++ithVtx)
		{
			VtxPoint& p0 = c.m_vtxList[ithVtx];
			VtxPoint& p1 = c.m_vtxList[(ithVtx + 1) % c.m_vtxList.size()];
			vertexSet::iterator it0 = m_vertexSet.find(Vertex(p0.X, p0.Y));
			vertexSet::iterator it1 = m_vertexSet.find(Vertex(p1.X, p1.Y));
			if (it0 == m_vertexSet.end() || it1 == m_vertexSet.end())
			{
				printf("mark boundary error: vtx not found\n");
				continue;
			}
			const Vertex& v0 = *it0;
			const Vertex& v1 = *it1;
			segmentList.push_back(v0.m_tempFlag);
			segmentList.push_back(v1.m_tempFlag);
			segmentMarkerList.push_back(ithContour);
		}
	}

	in.numberofsegments = segmentMarkerList.size();
	in.segmentlist = (int*) malloc(segmentList.size() * sizeof(int));
	memcpy(in.segmentlist, &segmentList[0], segmentList.size() * sizeof(int));
	in.segmentmarkerlist = (int*) malloc(segmentMarkerList.size() * sizeof(int));
	memcpy(in.segmentmarkerlist, &segmentMarkerList[0], segmentMarkerList.size() * sizeof(int));

	/* Triangulate the points.  Switches are chosen to read and write a  */
	/*   PSLG (p), preserve the convex hull (c), number everything from  */
	/*   zero (z), assign a regional attribute to each element (A), and  */
	/*   produce an edge list (e), a Voronoi diagram (v), and a triangle */
	/*   neighbor list (n).                                              */
	triangulate("pczAevn", &in, &mid, &vorout);
	//reportresult(&mid, 1, 1, 0, 1, 0, 0);
	//reportresult(&vorout, 0, 0, 0, 0, 1, 1);

	// collect results
	for (int ithTri = 0; ithTri < mid.numberoftriangles; ++ithTri)
	{
		int triIdx = ithTri * mid.numberofcorners;
		int vtxID[3];
		const Vertex* pVtx[3] = {NULL, NULL, NULL};

		bool allVtxFound = true;
		for (int i = 0; i < 3; ++i)
		{
			vtxID[i] = mid.trianglelist[triIdx+i];
			Vertex v(mid.pointlist[vtxID[i]*2], mid.pointlist[vtxID[i]*2+1]);
			vertexSet::iterator it = m_vertexSet.find(v);

			// some extra points will be added
			if (it == m_vertexSet.end())
			{
				it = m_vertexSet.insert(v).first;
				if (it == m_vertexSet.end())
				{
					printf("error in prodelaunay: vertex not found\n");
					allVtxFound = false;
					break;
				}
			}
			pVtx[i] = &(*it);
		}

		if (!allVtxFound)
			continue;

		m_triangleSet.insert(triangle(pVtx[0], pVtx[1], pVtx[2]));
	}
	
	// release memory
	freeTriangulateio(in);
	freeTriangulateio(mid);
	freeTriangulateio(vorout);
}

void DelaunayCore::DelaunayRouter::insertNonRepeated( VtxPoint& p )
{
	Vertex v(p);
	REAL stepX = abs(p.X) * 1e-5 + REAL_EPSILON;
	REAL stepY = abs(p.Y) * 1e-5 + REAL_EPSILON;
	while (m_vertexSet.find(v) != m_vertexSet.end())
	{
		// move the point slightly in order to make it unique
		p.X += stepX;
		p.Y += stepY;
		v.SetX(p.X);
		v.SetY(p.Y);
	}
	m_vertexSet.insert(v);
}

void DelaunayCore::DelaunayRouter::freeTriangulateio( triangulateio& io )
{
	free(io.pointlist);                                               /* In / out */
	free(io.pointattributelist);                                      /* In / out */
	free(io.pointmarkerlist);                                          /* In / out */

	free(io.trianglelist);                                             /* In / out */
	free(io.triangleattributelist);                                   /* In / out */
	free(io.trianglearealist);                                         /* In only */
	free(io.neighborlist);                                             /* Out only */

	free(io.segmentlist);                                              /* In / out */
	free(io.segmentmarkerlist);                                        /* In / out */

	free(io.holelist);                        /* In / pointer to array copied out */

	free(io.regionlist);                      /* In / pointer to array copied out */

	free(io.edgelist);                                                 /* Out only */
	free(io.edgemarkerlist);            /* Not used with Voronoi diagram); out only */
	free(io.normlist);                /* Used only with Voronoi diagram); out only */
	initTriangulateio(io);
}

void DelaunayCore::DelaunayRouter::setSmoothParam( float smoothFactor, int smoothIter )
{
	m_smoothFactor = smoothFactor;
	m_smoothIter = smoothIter;
}


