
#include "stdafx.h"
#include "GeometryUtility.h"
bool CodeAtlas::GeometryUtility::computeConvexHull( const MatrixXd& pos2D, QVector<QPointF>& convexHull )
{
	int nPnt = pos2D.rows();
	const double* pDataX = pos2D.data();
	const double* pDataY = pDataX + nPnt;
	return computeConvexHull( pDataX, pDataY, nPnt, convexHull );
}
bool CodeAtlas::GeometryUtility::computeConvexHull( const double* pDataX, const double* pDataY, int nPnt, QVector<QPointF>& convexHull )
{
	if (!pDataX  || !pDataY || nPnt == 0)
		return false;

	convexHull.clear();
	if (nPnt == 1)
	{
		convexHull.push_back(QPointF(pDataX[0], pDataY[0]));
		return true;
	}
	else if (nPnt == 2)
	{
		convexHull.push_back(QPointF(pDataX[0], pDataY[0]));
		convexHull.push_back(QPointF(pDataX[1], pDataY[1]));
		return true;
	}

	int     refPntIdx= 0;
	double  refPnt[2] = {*pDataX, *pDataY};
	for (int ithPnt = 1; ithPnt < nPnt; ++ithPnt)
	{
		float x = pDataX[ithPnt];
		float y = pDataY[ithPnt];
		if (y < refPnt[1] || (y == refPnt[1] && x < refPnt[0]))
		{
			refPnt[0] = x;
			refPnt[1] = y;
			refPntIdx = ithPnt;
		}
	}

	QList<Tuple> angleList;
	for (int ithPnt = 0; ithPnt < nPnt; ++ithPnt)
	{
		if (ithPnt == refPntIdx)
			continue;

		double dy = pDataY[ithPnt] - pDataY[refPntIdx];
		double dx = pDataX[ithPnt] - pDataX[refPntIdx];

		if (dy == 0 || dx == 0)
		{
			int i = 0;
		}
		Tuple t;
		t.m_value = atan2(dy, dx);
		t.m_id    = ithPnt;
		angleList.push_back(t);
	}

	qSort(angleList);
	// 	Tuple t = {refPntIdx, 0};
	// 	angleList.push_back(t);

	int minPntIdx = angleList[0].m_id;
	convexHull.resize(nPnt+1);
	convexHull[0] = QPointF(pDataX[refPntIdx], pDataY[refPntIdx]);
	convexHull[1] = QPointF(pDataX[minPntIdx], pDataY[minPntIdx]);
	int stackTop  = 1;

	for (int ithPnt = 1; ithPnt < nPnt-1; ++ithPnt)
	{
		int idx = angleList[ithPnt].m_id;

		// throw away useless points
		while (stackTop > 0)
		{
			QPointF& pnt0 = convexHull[stackTop-1];
			QPointF& pnt1 = convexHull[stackTop];

			double vec01[2] = {pnt1.x()-pnt0.x()   , pnt1.y()-pnt0.y()};
			double vec12[2] = {pDataX[idx]-pnt1.x(), pDataY[idx]-pnt1.y()};

			double det = vec01[0]*vec12[1] - vec01[1]*vec12[0];
			if (det > 0)
				break;
			stackTop--;
		}

		// insert new points
		QPointF& p = convexHull[++stackTop];
		p.setX(pDataX[idx]);
		p.setY(pDataY[idx]);
	}

	int nConvexPnts = stackTop+1;
	convexHull.resize(nConvexPnts);
	return true;
}

bool CodeAtlas::GeometryUtility::findBoundingBox( const MatrixXd& data, const VectorXd& radius, Vector2d& minPoint, Vector2d& maxPoint )
{
	int nPnt = data.rows();
	if (nPnt <= 0 || data.cols() <= 0)
		return false;
	const double* pRes = data.data();
	double minPnt[2] = {pRes[0], pRes[nPnt]};
	double maxPnt[2] = {pRes[0], pRes[nPnt]};
	for (int i = 0; i < nPnt; ++i)
	{
		double pnt[2] = {pRes[i], pRes[i+nPnt]};
		double r      = radius[i];
		minPnt[0] = minPnt[0] < (pnt[0]-r) ? minPnt[0] : (pnt[0]-r);
		minPnt[1] = minPnt[1] < (pnt[1]-r) ? minPnt[1] : (pnt[1]-r);
		maxPnt[0] = maxPnt[0] > (pnt[0]+r) ? maxPnt[0] : (pnt[0]+r);
		maxPnt[1] = maxPnt[1] > (pnt[1]+r) ? maxPnt[1] : (pnt[1]+r);
	}

	minPoint = Vector2d(minPnt[0], minPnt[1]);
	maxPoint = Vector2d(maxPnt[0], maxPnt[1]);
	return true;
}

bool CodeAtlas::GeometryUtility::moveToOrigin( MatrixXd& data, const VectorXd& radius, VectorXd& halfSize, VectorXd* pOffset )
{
	int   nPnt = data.rows();
	if (nPnt <= 0 || data.cols()<= 0)
		return false;

	Vector2d minPnt, maxPnt;
	GeometryUtility::findBoundingBox(data, radius, minPnt, maxPnt);
	Vector2d midPnt = 0.5 * (minPnt + maxPnt);

	if (pOffset)
	{
		(*pOffset) = -1.0 * midPnt;
	}
	for (int i = 0; i < data.rows(); ++i)
	{
		data(i,0) = data(i, 0) - midPnt[0];
		data(i,1) = data(i, 1) - midPnt[1];
	}
	halfSize = maxPnt - midPnt;
	return true;
}

double CodeAtlas::GeometryUtility::computeCircumference( const QVector<QPointF>& polygon )
{
	double c = 0;
	int n = polygon.size();
	for (int i = 0; i < n; ++i)
	{
		const QPointF& thisPnt = polygon[i];
		const QPointF& nextPnt = polygon[(i+1)%n];
		double dx = nextPnt.x() - thisPnt.x();
		double dy = nextPnt.y() - thisPnt.y();
		c += sqrt(dx*dx + dy*dy);
	}
	return c;
}

bool CodeAtlas::GeometryUtility::samplePolygon( const QVector<QPointF>& src, QVector<QPointF>& tar, double sampleRate, int minPnt)
{
	if (src.size() <= 1)
		return false;
	int nSrcPnts = src.size();
	int nTarPnts = qMax(minPnt, int(nSrcPnts * sampleRate + 1));
	tar.resize(nTarPnts);

	double c = computeCircumference(src);
	double tarSegLength = c / nTarPnts;
	double srcUsedLength = 0;
	double srcNextLength = pointDist(src[1], src[0]);
	double tarUsedLength = 0;
	for (int ithTar = 0, ithSrc = 0; ithTar < nTarPnts; ++ithTar)
	{
		tarUsedLength = tarSegLength * (ithTar);
		const double curLength = tarUsedLength;

		// find src segments
		while ((curLength >= srcNextLength || srcNextLength == srcUsedLength) &&
			ithSrc < nSrcPnts)
		{
			ithSrc++;
			const QPointF& thisPnt = src[ithSrc];
			const QPointF& nextPnt = src[(ithSrc+1)%nSrcPnts];
			double dx = nextPnt.x() - thisPnt.x();
			double dy = nextPnt.y() - thisPnt.y();
			double dLength = sqrt(dx*dx + dy*dy);
			srcUsedLength = srcNextLength;
			srcNextLength += dLength;
		}

		double srcRatio = 0;
		if (abs(srcNextLength - srcUsedLength) > 1e-8)
			srcRatio = (curLength - srcUsedLength) / (srcNextLength - srcUsedLength);
		tar[ithTar] = src[(ithSrc+1)%nSrcPnts] * srcRatio + src[ithSrc] * (1.0-srcRatio);
	}
	return true;
}


