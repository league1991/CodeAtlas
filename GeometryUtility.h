#pragma once
namespace CodeAtlas
{
	class GeometryUtility
	{
	public:
		// compute convex hull
		// convexHull     output convex hull, points are arranged in counter-clockwise order
		static bool computeConvexHull( const MatrixXd& pos2D, QVector<QPointF>& convexHull );
		static bool computeConvexHull( const double* pDataX, const double* pDataY, int nPnt, QVector<QPointF>& convexHull );
		
		static bool findBoundingBox(   const MatrixXd& data, const VectorXd& radius, 
									   Vector2d& minPoint, Vector2d& maxPoint);
		static bool moveToOrigin(   MatrixXd& data, const VectorXd& radius, VectorXd& halfSize, VectorXd* pOffset = NULL);

		static double computeCircumference(const QVector<QPointF>& polygon);
		static bool samplePolygon( const QVector<QPointF>& src, QVector<QPointF>& tar, double sampleRate, int minPnt = 3);
		static bool computeNormal(	const Vector2d& prevPnt, const Vector2d& thisPnt, const Vector2d& nextPnt,
									Vector2d& normal);
	private:
		static inline double pointDist(const QPointF& thisPnt, const QPointF& nextPnt)
		{
			double dx = nextPnt.x() - thisPnt.x();
			double dy = nextPnt.y() - thisPnt.y();
			return sqrt(dx*dx + dy*dy);
		}
		struct Tuple
		{
			int    m_id;
			double m_value;
			bool operator<(const Tuple& other)const{return m_value < other.m_value;}
		};
	};
}