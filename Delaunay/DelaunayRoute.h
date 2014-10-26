#pragma once
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>
#include "Delaunay.h"
#include "..\Triangle\triangle.h"

#define COMMON_EDGE_ROUTE
//#define COMMON_CENTROID_ROUTE
#define PRO_DELAUNAY
#define MULTI_SEGMENT
//#define BIDIRECTIONAL_ROUTE
#ifndef USE_QT
#define qDebug printf
#endif

namespace DelaunayCore
{
#ifndef DOXY_IGNORE_THIS
	// Define my personal traits
	struct MyTraits : OpenMesh::DefaultTraits
	{
		// Let Point and Normal be a vector of doubles
		typedef OpenMesh::Vec3d Point;
		typedef OpenMesh::Vec3d Normal;
		// Already defined in OpenMesh::DefaultTraits
		// HalfedgeAttributes( OpenMesh::Attributes::PrevHalfedge );

		// Uncomment next line to disable attribute PrevHalfedge
		// HalfedgeAttributes( OpenMesh::Attributes::None );
		//
		// or
		//
		// HalfedgeAttributes( 0 );
	};
#endif
	// Define my mesh with the new traits!
	//typedef OpenMesh::PolyMesh_ArrayKernelT<>  MyMesh;
	typedef OpenMesh::PolyMesh_ArrayKernelT<MyTraits> DelaunayMesh;
	typedef DelaunayMesh::VertexHandle VertexHandle;
	typedef DelaunayMesh::HalfedgeHandle HalfedgeHandle;
	typedef DelaunayMesh::EdgeHandle EdgeHandle;
	typedef DelaunayMesh::FaceHandle FaceHandle;
	typedef DelaunayMesh::Point Point;

	typedef DelaunayMesh::VertexVertexIter VertexVertexIter;
	typedef DelaunayMesh::HalfedgeIter HalfedgeIter;
	typedef DelaunayMesh::FaceHalfedgeIter FaceHalfedgeIter;
	typedef DelaunayMesh::FaceVertexIter FaceVertexIter;
	typedef DelaunayMesh::VertexOHalfedgeIter VertexOHalfedgeIter;
	typedef DelaunayMesh::FaceIter FaceIter;
	typedef DelaunayMesh::EdgeIter EdgeIter;

	struct EntryPnt
	{
		EntryPnt(VertexHandle vtxHandle, VtxPoint& normal):m_vtxHandle(vtxHandle), m_normal(normal), m_count(0){}
		VertexHandle m_vtxHandle;
		VtxPoint	 m_normal;
		int			 m_count;
	};
	struct Contour
	{
		Contour():m_radius(1){}
		Contour(const vector<DelaunayCore::VtxPoint>& pnt);
		// must be counter-clockwise simple curve!
		vector<DelaunayCore::VtxPoint> m_vtxList;
		vector<VertexHandle> m_handleList;
		// seed handle used to be end points of any route
		vector<EntryPnt>	 m_srcEntryList;
		vector<EntryPnt>	 m_tarEntryList;
		double				 m_radius;
	};
	struct CustomData
	{
		int m_tempTag;											// tag used to support stackless traversal, or record something temporarily
	};
	struct CustomVertexData:public CustomData
	{
		enum VtxType{VTX_CONTOUR, VTX_OTHERS};
		CustomVertexData():CustomData(),m_tempNInContourEdge(0),m_vertexType(VTX_OTHERS){}
		HalfedgeHandle m_tempEdge;						// temp edge used to record edge route
		int m_tempNInContourEdge;						// temp tag used to identify complex contour
		int m_vertexType;
	};
	struct CustomFaceData:public CustomData
	{
		CustomFaceData():m_isInternalFace(false){}
		int		   m_tempID0;							// temp ID used to find corresponding vtx
		int		   m_tempID1;							// temp ID used to find corresponding vtx
		FaceHandle m_tempFace;							// temp face used to record edge route
		bool m_isInternalFace;
	};
	struct CustomEdgeData:public CustomData
	{
		CustomEdgeData():m_isContourEdge(false){}
		bool m_isContourEdge;
	};
	struct CustomHalfEdgeData:public CustomData
	{
		CustomHalfEdgeData():m_isContourEdge(false){}
		bool m_isContourEdge;
	};

	class DelaunayRouter
	{
	public:
		DelaunayRouter(void);
		~DelaunayRouter(void);

		bool addContour(const vector<VtxPoint>& contourPnt);
#ifdef USE_QT
		bool addContour(const QPolygonF& contourPoly);
		bool getContourEntries( int ithContour, 
								QVector<QPointF>& inPoints, QVector<QPointF>& inNormals, 
								QVector<QPointF>& outPoints, QVector<QPointF>& outNormals,
								bool onlyRoutedEntries = true);

#endif
		int  getNumContours(){return m_contourList.size();}
		void reset();
		void compute();
		bool getRoute(int src, int tar, vector<VtxPoint>& route, bool avoidInternalFace = true);

#ifdef GDI_DRAWFUNCTION
		void drawTriangles(CDC& dc, REAL hScale, REAL vScale);
#endif

		void setSmoothParam(float smoothFactor, int smoothIter);
		DelaunayMesh& getRouteMesh(){return m_mesh;}

		void setEndPointNormalRatio(float normalRatio){m_endPointNormalRatio = normalRatio;}
		
		// laneWidthRatio = lane width / avg edge length
		void setLaneWidthRatio(float laneWidthRatio){m_laneWidthRatio = laneWidthRatio;}
		float getLaneWidth()const{return m_laneWidth;}
	private:
		struct VtxInfo
		{
			VtxInfo(VertexHandle handle, double value=0.):m_vtxHandle(handle), m_value(value){}
			bool operator<(const VtxInfo& other)const
			{
				return m_value < other.m_value;
			}
			bool operator==(const VtxInfo& other)const
			{
				return m_vtxHandle == other.m_vtxHandle;
			}
			VertexHandle m_vtxHandle;
			double        m_value;
		};
		struct EdgeRoute
		{
			Point p1,p2;
			int idx1,idx2;
		};

		bool addContour(const Contour& contour);

		void buildVertexSetFromContour();				// add contour vertices to vtxset
		void buildSafeFrame();							// unsafe!!! may cause triangle with 0 area
		void buildSafeCircle();
		void buildOpenMeshData();

		void insertNonRepeated(VtxPoint& p);			// safe insert function

		// professional delaunay triangulation
		void initTriangulateio(triangulateio& io);
		void freeTriangulateio(triangulateio& io);
		void proDelaunay();

		// mark blocked portion
		void markContourHalfedge();// mark all contour edges in m_edgeData	
		bool findEdgeRouteBFS(const VertexHandle src, const VertexHandle tar, vector<HalfedgeHandle>& route);// find a route from source vertex to target vertex using breath first search
		bool markInternalFaces();

		// find route
		bool findFaceRouteBFS(	const vector<VertexHandle> srcSet, 
			const vector<VertexHandle> tarSet, 
			vector<FaceHandle>& route,
			bool considerFaceBlock = true ,
			int* pSrcVtx = NULL,
			int* pTarVtx = NULL);					 // find a face route connecting the two point sets

		void routeByEdge(const Point& srcEntry, const Point& tarEntry, const Point& src, const Point& tar, vector<Point>& p1List, vector<Point>& p2List, vector<VtxPoint>& route);
		void processEdgeRoute(const vector<EdgeRoute>& edges,
			vector<Point>& p1List,
			vector<Point>& p2List);

		// laplacian smooth
		void smooth(vector<VtxPoint>& route, double smoothFactor = 0.5, int nIter = 1, bool preserveEndTangent = true);

		// utilities
		Point computeOuterCenter(Point& p0, Point& p1, Point& p2);
		void clearAllTags(int vtxValue = 0, int edgeValue = 0, int halfEdgeValue = 0, int faceValue = 0);
		inline int Int(REAL r)const	{ return (int) floor(r + 0.5f); }

		template <typename T>
		inline bool isHandleValid(T& handle)const
		{
			return m_mesh.is_valid_handle(handle);
		}
		vector<Contour> m_contourList;					// input contour

		vertexSet       m_vertexSet;					// vertex used to build triangulation
		triangleSet     m_triangleSet;					// triangulation result

		// custom data
		vector<CustomVertexData> m_vertexData;
		vector<CustomEdgeData>   m_edgeData;
		vector<CustomHalfEdgeData> m_halfEdgeData;
		vector<CustomFaceData>   m_faceData;

		DelaunayMesh			m_mesh;							// result mesh

		// contoure center
		bool			m_addContourCenter;				// add average point of each contour to vtxset

		// double edge
		bool			m_isDoubleEdgeContour;			// add another contour outside each contour
		REAL			m_doubleEdgeWidth;				// double edge width

		// route curve
		int				m_routeStepSize;
		int				m_nRouteEntries;				// for every contour, the maximum number of entries

		// bounding rectangle
		bool			m_addSafeFrame;
		REAL			m_paddingRatio;					// determine the padding width of bounding rect

		// smooth
		int				m_smoothIter;
		REAL			m_smoothFactor;

		// end point control
		REAL			m_endPointNormalRatio;			// control the normal length of end point

		
		REAL			m_laneOffsetRatio;				// make "roads" along opposite directions not overlap
		REAL			m_laneWidth;
		REAL			m_laneWidthRatio;				// adjust lane width

		// contour
		REAL			m_avgContourRadius;
		REAL			m_minContourRadius,m_maxContourRadius;
		REAL			m_boundaryLengthRatio;
	};


}