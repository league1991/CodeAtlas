#ifndef BSPLINE_H
#define BSPLINE_H

#include <QMainWindow>
#include <QPainter>
#include <QPainterPath>
#include <QDebug>

namespace CodeAtlas
{
class BSpline
{
public:
	enum Type
	{
		BSPLINE_OPEN_UNIFORM = 0,
		BSPLINE_PERIODIC = 1,
	};
	BSpline(int d, Type type = BSPLINE_OPEN_UNIFORM, bool simplifyPnts = false);								//construction, input means degree+1
	~BSpline();

	void	addPoint(QPointF);					//add a QPoint
	void	addPoint(float,float);				//add a point with x magnitude and y magnitude

	QPointF computePoint(float t);				//given a t, return the correspond Point
	const QList<QPointF>& getCurvePnts(){return m_curvePt;}
	MatrixXf getCurveMatrix();
	QPainterPath getPainterPath(int nSeg );
	void	computeLine(int nSeg);				//determine the value of each t, line the points into a list
	void	setSimplifyMinAngles(float radian);
private:	
	float	computeB(int k,int d1,float u1);	//compute value of B for computePoint
	float   computePeriodicB(int k, int d1, float u1);
	void	buildOpenUniformKnotVector();		//allocate space and give value to knot vector
	void	buildPeriodicKnotVector();
	void	simplifyCurve(const QList<QPointF>& tmpCurve);
	Type			m_type;						//spline type
	int				m_d;						//degree+1
	int				m_nKnots;					//length of knot vector
	QList<QPointF>	m_orginPt;					//original points
	QList<QPointF>	m_curvePt;					//points on the BSpline line
	QPainterPath	m_BSplinePath;				//path of BSpline line
	float		*   m_u;						//restore value of knot vectors

	bool			m_simplifyPnts;				//simplify curve points
	float			m_cosSqThreshold;			//threshold used to simplify pnts
};
}

#endif