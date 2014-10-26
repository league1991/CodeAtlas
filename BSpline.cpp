#include "stdafx.h"
#include "BSpline.h"

using namespace CodeAtlas;

BSpline::BSpline(int d1, Type type, bool simplifyPnts):m_u(NULL), m_type(type), m_nKnots(0), m_simplifyPnts(simplifyPnts)
{
	//degree+1
	m_d=d1;
	setSimplifyMinAngles(M_PI_4 / 9.f * 0.5f);		// 5 degree
}

void BSpline::addPoint( QPointF pt)
{
	//add a QPoint
	m_orginPt.push_back(pt);
}

void BSpline::addPoint( float a,float b)
{
	//add a point with x magnitude and y magnitude
	QPointF temp;
	temp.setX(a);
	temp.setY(b);
	m_orginPt.push_back(temp);
}

QPointF BSpline::computePoint( float t)
{
	//given a t, return the correspond Point
	QPointF temp;
	float weightSum = 0;
	for (int i=0;i<m_orginPt.size();i++)
	{
		float B = 0;
		switch(m_type)
		{
		case BSPLINE_OPEN_UNIFORM:
			B = computeB(i,m_d,t); break;
		case BSPLINE_PERIODIC:
			B = computePeriodicB(i,m_d,t); break;
		}
		temp+=( m_orginPt[i] * B );
		weightSum += B;
	}
	if (weightSum > 0)
		temp /= weightSum;
	return temp;
}

float BSpline::computeB( int k,int d,float t )
{
	if (d<=1)
	{
		if (t>=m_u[k] && t<=m_u[k+1]) return 1;
		else return 0;
	}
	else
	{
		float deltaU1 = m_u[k+d-1]-m_u[k];
		float deltaU2 = m_u[k+d]-m_u[k+1];

		float numerator1 = t-m_u[k];
		float numerator2 = m_u[k+d]-t;

		//compute value of B for computePoint
		float temp = 0;
		float factor1 = numerator1 / deltaU1;
		float factor2 = numerator2 / deltaU2;
		if (deltaU1 >= 0 && factor1 > 0 && factor1 <= 1)
			temp += factor1 * computeB(k,d-1,t);
		if (deltaU2 >= 0 && factor2 > 0 && factor2 <= 1)
			temp += factor2 * computeB(k+1,d-1,t);

		//qDebug("tempb:%f",temp);
		return temp;
	}
}

void BSpline::buildOpenUniformKnotVector()
{
	//allocate space and give value to knot vector
	int nPnt=m_orginPt.size();
	m_d = qMax(1,qMin(m_d, nPnt));
	m_nKnots = nPnt + m_d;
	if (m_u)
		delete[] m_u;
	m_u=new float[m_nKnots];
	for (int i = 0; i < m_d; ++i)
	{
		m_u[i] = 0;
		m_u[m_nKnots-1-i] = 1;
	}
	int nNormalKnots = m_nKnots - m_d * 2;
	float deltaKnot = 1.f / (nNormalKnots+1);
	float knotValue = deltaKnot;
	for (int i= 0; i < nNormalKnots; i++, knotValue += deltaKnot)
	{
		m_u[m_d+i] = knotValue;
	}
}

void BSpline::computeLine( int nSegs)
{
	//determine the value of each t, line the points into a list
	int ptnum=m_orginPt.size();
	int nCurvePnts = 0;
	float t = 0;
	if (m_type == BSPLINE_OPEN_UNIFORM)
	{
		buildOpenUniformKnotVector();
		nSegs = max(nSegs, 2);
		t=float(1)/float(nSegs);
		nCurvePnts = nSegs + 1;
	}
	else if (m_type == BSPLINE_PERIODIC)
	{
		buildPeriodicKnotVector();
		nSegs = max(nSegs, 1);
		t = float(ptnum) / nSegs;
		nCurvePnts = nSegs;
	}

	float maxU = m_u[ptnum + m_d -1];	
	QList<QPointF> tmpCurve;
	for (int j=0;j<=nCurvePnts;j++)
	{
		float param = min(t*j, maxU);
		QPointF temp=computePoint(min(t*j, maxU));
		tmpCurve.push_back(temp);
	}

	if (m_simplifyPnts)
	{
		simplifyCurve(tmpCurve);
	}
	else
		m_curvePt = tmpCurve;
}

QPainterPath BSpline::getPainterPath(int n )
{
	//line up the points
	computeLine(n);
	QPainterPath path;
	m_BSplinePath = path;
	m_BSplinePath.moveTo(m_curvePt[0]);
	for (int i=1;i<m_curvePt.size();i++)
	{
		m_BSplinePath.lineTo(m_curvePt[i]);
	}
	return m_BSplinePath;
}




CodeAtlas::BSpline::~BSpline()
{
	if (m_u)
		delete[] m_u;
}

Eigen::MatrixXf CodeAtlas::BSpline::getCurveMatrix()
{
	MatrixXf curve;
	if (m_curvePt.size() == 0)
		return curve;
	curve.resize(m_curvePt.size(), 2);
	for (int i = 0; i < m_curvePt.size(); ++i)
	{
		curve(i, 0) = m_curvePt[i].x();
		curve(i, 1) = m_curvePt[i].y();
	}
	return curve;
}

void CodeAtlas::BSpline::buildPeriodicKnotVector()
{
	//allocate space and give value to knot vector
	int nPnt=m_orginPt.size();
	m_d = qMax(1,qMin(m_d, nPnt));
	m_nKnots = nPnt + m_d;
	if (m_u)
		delete[] m_u;
	m_u=new float[m_nKnots];

	for (int i= 0; i < m_nKnots; i++)
	{
		m_u[i] = i;
	}
}

float CodeAtlas::BSpline::computePeriodicB( int k, int d1, float t )
{
	int nPnts = m_orginPt.size();
	int offset = 0;
	if (t >= m_u[0] && t <= m_u[m_d-1])
	{
		offset = (0 + d1) % nPnts - (k + d1) % nPnts;
	}
	else //if (t >= m_u[nPnts] && t <= m_u[nPnts+d1-1])
	{
		offset = -k;
	}
	return computeB(0, d1, t + (float)offset);
}

void CodeAtlas::BSpline::setSimplifyMinAngles( float radian )
{
	float cosVal = cos(radian);
	m_cosSqThreshold = cosVal * cosVal;
}

void CodeAtlas::BSpline::simplifyCurve( const QList<QPointF>& tmpCurve )
{
	m_curvePt.push_back(tmpCurve[0]);
	for (int i = 1; i < tmpCurve.size()-1; ++i)
	{
		const QPointF& lastPnt = m_curvePt.back();
		const QPointF& thisPnt = tmpCurve[i];
		const QPointF& nextPnt = tmpCurve[i+1];

		QPointF  lastDir = thisPnt - lastPnt;
		QPointF  nextDir = nextPnt - thisPnt;
		float lastSqLength = lastDir.x() * lastDir.x() + lastDir.y() * lastDir.y();
		float nextSqLength = nextDir.x() * nextDir.x() + nextDir.y() * nextDir.y();
		float lengthProd   = lastSqLength * nextSqLength;
		if (lastSqLength == 0)		
			continue;							// two points are coincident
		else
		{
			float dotProduct = (lastDir.x() * nextDir.x() + lastDir.y() * nextDir.y());
			float cosSq = dotProduct * dotProduct / lengthProd;
			if (cosSq < m_cosSqThreshold)
			{
				m_curvePt.push_back(thisPnt);
			}
		}
	}
	m_curvePt.push_back(tmpCurve.back());
}
