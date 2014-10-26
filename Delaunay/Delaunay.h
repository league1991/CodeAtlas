/********************************************************************************
	Copyright (C) 2004-2005 Sjaak Priester	

	This is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this application; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
********************************************************************************/

// Delaunay
// Class to perform Delaunay triangulation on a set of vertices
//
// Version 1.2 (C) 2005, Sjaak Priester, Amsterdam.
// - Removed stupid bug in SetY; function wasn't used, so no consequences. Thanks to squat.
//
// Version 1.1 (C) 2005, Sjaak Priester, Amsterdam.
// - Removed bug which gave incorrect results for co-circular vertices.
//
// Version 1.0 (C) 2004, Sjaak Priester, Amsterdam.
// mailto:sjaak@sjaakpriester.nl

#pragma once

#include <set>
#include <algorithm>
#include <math.h>

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>

using namespace std;


namespace DelaunayCore
{
// I designed this with GDI+ in mind. However, this particular code doesn't
// use GDI+ at all, only some of it's variable types.
// These definitions are substitutes for those of GDI+. 
typedef double REAL;
struct VtxPoint
{
	VtxPoint() : X(0), Y(0)	{}
	VtxPoint(const VtxPoint& p) : X(p.X), Y(p.Y)	{}
	VtxPoint(REAL x, REAL y) : X(x), Y(y)	{}
	VtxPoint operator+(const VtxPoint& p) const	{ return VtxPoint(X + p.X, Y + p.Y); }
	VtxPoint operator-(const VtxPoint& p) const	{ return VtxPoint(X - p.X, Y - p.Y); }
	VtxPoint operator*(const REAL& scale) const { return VtxPoint(X * scale, Y * scale); }
	bool operator<(const VtxPoint& v) const
	{
		if (X == v.X) return Y < v.Y;
		return X < v.X;
	}
	void	 normalize()
	{
		double norm = sqrt(X*X+Y*Y);
		X /= (norm + 1e-10);
		Y /= (norm + 1e-10);
	}
	static bool computeNormal( const VtxPoint& prevPnt, const VtxPoint& thisPnt, const VtxPoint& nextPnt, VtxPoint& normal )
	{
		VtxPoint dPrev = thisPnt - prevPnt;
		VtxPoint dThis = nextPnt - thisPnt;
		dPrev.normalize();
		dThis.normalize();
		normal.X = dPrev.Y + dThis.Y;
		normal.Y = -dPrev.X - dThis.X;
		normal.normalize();
		return true;
	}
	REAL X;
	REAL Y;	
};

const REAL REAL_EPSILON = FLT_EPSILON;	// = 2^-23; I've no idea why this is a good value, but GDI+ has it.


///////////////////
// vertex

class Vertex
{
public:
	Vertex()					: m_Pnt(0.0F, 0.0F)			{}
	Vertex(const Vertex& v)		: m_Pnt(v.m_Pnt)			{}
	Vertex(const VtxPoint& pnt)	: m_Pnt(pnt)				{}
	Vertex(REAL x, REAL y)		: m_Pnt(x, y)				{}
	Vertex(int x, int y)		: m_Pnt((REAL) x, (REAL) y)	{}

	bool operator<(const Vertex& v) const
	{
		if (m_Pnt.X == v.m_Pnt.X) return m_Pnt.Y < v.m_Pnt.Y;
		return m_Pnt.X < v.m_Pnt.X;
	}

	bool operator==(const Vertex& v) const
	{
		return m_Pnt.X == v.m_Pnt.X && m_Pnt.Y == v.m_Pnt.Y;
	}
	
	REAL GetX()	const	{ return m_Pnt.X; }
	REAL GetY() const	{ return m_Pnt.Y; }

	void SetX(REAL x)		{ m_Pnt.X = x; }
	void SetY(REAL y)		{ m_Pnt.Y = y; }

	inline const VtxPoint& GetPoint() const		{ return m_Pnt; }

#ifdef USE_OPENMESH
	const OpenMesh::VertexHandle& handle()const{return m_vtxHandle;}
	OpenMesh::VertexHandle& handle(){return m_vtxHandle;}
	void setHandle(const OpenMesh::VertexHandle& handle)const{m_vtxHandle = handle;}
#endif
	mutable int m_tempFlag;
protected:
#ifdef USE_OPENMESH
	mutable OpenMesh::VertexHandle m_vtxHandle;
#endif
	VtxPoint	m_Pnt;
};

typedef set<Vertex> vertexSet;
typedef set<Vertex>::iterator vIterator;
typedef set<Vertex>::const_iterator cvIterator;

///////////////////
// triangle

class triangle
{
public:
	triangle(const triangle& tri)
		: m_Center(tri.m_Center)
		, m_R(tri.m_R)
		, m_R2(tri.m_R2)
	{
		for (int i = 0; i < 3; i++) m_Vertices[i] = tri.m_Vertices[i];
	}
	triangle(const Vertex * p0, const Vertex * p1, const Vertex * p2)
	{
		m_Vertices[0] = p0;
		m_Vertices[1] = p1;
		m_Vertices[2] = p2;
		SetCircumCircle();
	}
	triangle(const Vertex * pV)
	{
		for (int i = 0; i < 3; i++) m_Vertices[i] = pV++;
		SetCircumCircle();
	}

	bool operator<(const triangle& tri) const
	{
		if (m_Center.X == tri.m_Center.X) return m_Center.Y < tri.m_Center.Y;
		return m_Center.X < tri.m_Center.X;
	}

	const Vertex * GetVertex(int i) const
	{
		return m_Vertices[i];
	}
	void GetVertex(const Vertex*& v0, const Vertex*& v1, const Vertex*& v2)const
	{
		v0 = m_Vertices[0];
		v1 = m_Vertices[1];
		v2 = m_Vertices[2];
	}
	inline const double ComputeZVector()const
	{
		VtxPoint dP1 = m_Vertices[1]->GetPoint() - m_Vertices[0]->GetPoint();
		VtxPoint dP2 = m_Vertices[2]->GetPoint() - m_Vertices[0]->GetPoint();
		return dP1.X * dP2.Y - dP1.Y * dP2.X;
	}

	bool IsLeftOf(cvIterator itVertex) const
	{
		// returns true if * itVertex is to the right of the triangle's circumcircle
		return itVertex->GetPoint().X > (m_Center.X + m_R);
	}

	triangle& operator=(const triangle& tri);
	VtxPoint ComputeOuterCenter()
	{
		VtxPoint h1 = (m_Vertices[1]->GetPoint() + m_Vertices[0]->GetPoint()) * 0.5;
		VtxPoint h2 = (m_Vertices[2]->GetPoint() + m_Vertices[0]->GetPoint()) * 0.5;
		VtxPoint e1 = m_Vertices[1]->GetPoint() - m_Vertices[0]->GetPoint();
		VtxPoint e2 = m_Vertices[2]->GetPoint() - m_Vertices[0]->GetPoint();

		REAL b1 = h1.X * e1.X + h1.Y * e1.Y;
		REAL b2 = h2.X * e2.X + h2.Y * e2.Y;

		REAL delta = e1.X * e2.Y - e1.Y * e2.X;
		if (delta > REAL_EPSILON || delta < -REAL_EPSILON)
		{
			REAL resX, resY;
			resX = (b1 * e2.Y - b2 * e1.Y) / delta;
			resY = (b2 * e1.X - b1 * e2.X) / delta;
			return VtxPoint(resX, resY);
		}
		return (h1 + h2) * 0.5;
	}

	bool CCEncompasses(cvIterator itVertex) const
	{
		// Returns true if * itVertex is in the triangle's circumcircle.
		// A vertex exactly on the circle is also considered to be in the circle.

		// These next few lines look like optimisation, however in practice they are not.
		// They even seem to slow down the algorithm somewhat.
		// Therefore, I've commented them out.

		// First check boundary box.
//		REAL x = itVertex->GetPoint().X;
//				
//		if (x > (m_Center.X + m_R)) return false;
//		if (x < (m_Center.X - m_R)) return false;
//
//		REAL y = itVertex->GetPoint().Y;
//				
//		if (y > (m_Center.Y + m_R)) return false;
//		if (y < (m_Center.Y - m_R)) return false;

		VtxPoint dist = itVertex->GetPoint() - m_Center;	// the distance between v and the circle center
		REAL dist2 = dist.X * dist.X + dist.Y * dist.Y;		// squared
		return dist2 <= m_R2;								// compare with squared radius
	}
protected:
	const Vertex * m_Vertices[3];	// the three triangle vertices
	VtxPoint m_Center;				// center of circumcircle
	REAL m_R;			// radius of circumcircle
	REAL m_R2;			// radius of circumcircle, squared

	void SetCircumCircle()
	{
		REAL x0 = m_Vertices[0]->GetX();
		REAL y0 = m_Vertices[0]->GetY();

		REAL x1 = m_Vertices[1]->GetX();
		REAL y1 = m_Vertices[1]->GetY();

		REAL x2 = m_Vertices[2]->GetX();
		REAL y2 = m_Vertices[2]->GetY();

		REAL y10 = y1 - y0;
		REAL y21 = y2 - y1;

		bool b21zero = y21 > -REAL_EPSILON && y21 < REAL_EPSILON;

		if (y10 > -REAL_EPSILON && y10 < REAL_EPSILON)
		{
			if (b21zero)	// All three vertices are on one horizontal line.
			{
				if (x1 > x0)
				{
					if (x2 > x1) x1 = x2;
				}
				else
				{
					if (x2 < x0) x0 = x2;
				}
				m_Center.X = (x0 + x1) * .5;
				m_Center.Y = y0;
			}
			else	// m_Vertices[0] and m_Vertices[1] are on one horizontal line.
			{
				REAL m1 = - (x2 - x1) / y21;

				REAL mx1 = (x1 + x2) * .5;
				REAL my1 = (y1 + y2) * .5;

				m_Center.X = (x0 + x1) * .5;
				m_Center.Y = m1 * (m_Center.X - mx1) + my1;
			}
		}
		else if (b21zero)	// m_Vertices[1] and m_Vertices[2] are on one horizontal line.
		{
			REAL m0 = - (x1 - x0) / y10;

			REAL mx0 = (x0 + x1) * .5;
			REAL my0 = (y0 + y1) * .5;

			m_Center.X = (x1 + x2) * .5;
			m_Center.Y = m0 * (m_Center.X - mx0) + my0;
		}
		else	// 'Common' cases, no multiple vertices are on one horizontal line.
		{
			REAL m0 = - (x1 - x0) / y10;
			REAL m1 = - (x2 - x1) / y21;

			REAL mx0 = (x0 + x1) * .5;
			REAL my0 = (y0 + y1) * .5;

			REAL mx1 = (x1 + x2) * .5;
			REAL my1 = (y1 + y2) * .5;

			m_Center.X = (m0 * mx0 - m1 * mx1 + my1 - my0) / (m0 - m1);
			m_Center.Y = m0 * (m_Center.X - mx0) + my0;
		}

		REAL dx = x0 - m_Center.X;
		REAL dy = y0 - m_Center.Y;

		m_R2 = dx * dx + dy * dy;	// the radius of the circumcircle, squared
		m_R = (REAL) sqrt(m_R2);	// the proper radius

		// Version 1.1: make m_R2 slightly higher to ensure that all edges
		// of co-circular vertices will be caught.
		// Note that this is a compromise. In fact, the algorithm isn't really
		// suited for very many co-circular vertices.
		m_R2 *= 1.000001;
	}
};

// Changed in verion 1.1: collect triangles in a multiset.
// In version 1.0, I used a set, preventing the creation of multiple
// triangles with identical center points. Therefore, more than three
// co-circular vertices yielded incorrect results. Thanks to Roger Labbe.
typedef multiset<triangle> triangleSet;
typedef multiset<triangle>::iterator tIterator;
typedef multiset<triangle>::const_iterator ctIterator;

///////////////////
// edge

class edge
{
public:
	edge(const edge& e)	: m_pV0(e.m_pV0), m_pV1(e.m_pV1)	{}
	edge(const Vertex * pV0, const Vertex * pV1)
		: m_pV0(pV0), m_pV1(pV1)
	{
	}

	bool operator<(const edge& e) const
	{
		if (m_pV0 == e.m_pV0) return * m_pV1 < * e.m_pV1;
		return * m_pV0 < * e.m_pV0;
	}

	const Vertex * m_pV0;
	const Vertex * m_pV1;
};

typedef set<edge> edgeSet;
typedef set<edge>::iterator edgeIterator;
typedef set<edge>::const_iterator cedgeIterator;

///////////////////
// Delaunay

class Delaunay
{
public:
	// Calculate the Delaunay triangulation for the given set of vertices.
	void Triangulate(const vertexSet& vertices, triangleSet& output);

	// Put the edges of the triangles in an edgeSet, eliminating double edges.
	// This comes in useful for drawing the triangulation.
	void TrianglesToEdges(const triangleSet& triangles, edgeSet& edges);
protected:
	void HandleEdge(const Vertex * p0, const Vertex * p1, edgeSet& edges);
};
}
