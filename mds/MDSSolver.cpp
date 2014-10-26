#include "StdAfx.h"
#include "MDSSolver.h"
#include "stressMDSlib.h"

using namespace CodeAtlas;
HitMDSSolver::HitMDSSolver(void)
{
	m_maxIter = 100;
	m_stepSize = 0.05f;
	m_useExternalInitPos = false;
	m_oriDistType = HitMDSSolver::MDS_EUCLIDEAN;
}

HitMDSSolver::~HitMDSSolver(void)
{
}

void HitMDSSolver::compute()
{
	preprocess();

//	srand(time(0));
	unsigned n = m_dataArray.size();
//	double oldStrssFunc = computeStressFunc();
	for (int ithIter = 0; ithIter < m_maxIter; ++ithIter)
	{
		double curStepSize = m_stepSize * (m_maxIter - ithIter) * .25f * (ithIter%2+1)/ m_maxIter;
		
		Eigen::MatrixXd pntGrad = compute2DPointApproxGrad();
		qreal maxStepLength = 0;
		for (int ithPoint = 0; ithPoint < m_dataArray.size(); ++ithPoint)
		{
			double gradX = pntGrad(0,ithPoint);
			double gradY = pntGrad(1,ithPoint);

			QVector2D& oriPos = m_position2D[ithPoint];
			//qDebug("%d th point, oriPos: %f, %f\n   grad: %f, %f", ithPoint, oriPos.x(), oriPos.y(), gradX, gradY);
			QVector2D  step   = curStepSize * QVector2D(gradX, gradY);
			maxStepLength     = qMax(qreal(maxStepLength), qreal(step.length()));
			oriPos = oriPos + step;
		}
		computeD2DMat();
		if (maxStepLength < 0.001 )
			break;
	}

	if (m_usePostProcess)
		postProcess();
}

double MDSSolver::getAlignVal( const QString& str )
{
	if (str.length() == 0)
		return 0.0;
	char c = str.at(0).toLower().toLatin1();
	return (c - 'a') % 10;
}

double MDSSolver::computeDissimilarity(int ithData, int jthData)
{
	double length = 0;
	MDSData& d1 = m_dataArray[ithData];
	MDSData& d2 = m_dataArray[jthData];
	if (ithData != jthData)
	{
		switch(m_oriDistType)
		{
		case MDS_EUCLIDEAN:
			length = (d1.m_dataVec - d2.m_dataVec).norm();
			break;
		case MDS_COS:
			length = d1.m_dataVec.dot(d2.m_dataVec) /  
				(d1.m_dataVec.norm() * d2.m_dataVec.norm() + 1e-10f);
			length = 1 - length;
			break;
		case MDS_PEARSON:
			int dim = d1.m_dataVec.size();
			double d1Mean = d1.m_dataVec.sum() / dim;
			double d2Mean = d2.m_dataVec.sum() / dim;
			VectorXd d1Vec = d1.m_dataVec - VectorXd::Ones(dim) * d1Mean;
			VectorXd d2Vec = d2.m_dataVec - VectorXd::Ones(dim) * d2Mean;			
			length = d1Vec.dot(d2Vec) / (d1Vec.norm() * d2Vec.norm() + 1e-10f);
			length = 1 - length;
			// length ^ 8
			length = length * length;
			length = length * length * length;
			break;
		}
	}
	return length;
	//double    totLength = length + d1.m_radius + d2.m_radius;
	//return   totLength;// * totLength;
}

void HitMDSSolver::computeDMat()
{
	m_meanD = 0;
	m_nNonZero = 0;
	for (int i = 0; i < m_dataArray.size(); ++i)
	{
		m_D(i,i)=0.0;
		for (int j = i+1; j < m_dataArray.size(); ++j)
		{
			double d = m_D(i,j);
			m_nNonZero += d < 1e-10 ? 0 : 2;
			m_meanD += d;
		}
	}
	int n = m_dataArray.size();
	m_meanD = m_meanD * 2.0 / (m_nNonZero + 1e-10f);

	m_deltaDSqSum = 0;
	m_deltaD.resizeLike(m_D);
	for (int i = 0; i < m_D.rows(); ++i)
	{
		m_deltaD(i,i) = 0; 
		for (int j = i+1; j < m_D.cols(); ++j)
		{
			double deltaD = m_D(i,j) < 1e-10 ? 0 : m_D(i,j) - m_meanD;
			m_deltaD(i,j) = m_deltaD(j,i) = deltaD;
			m_deltaDSqSum += deltaD * deltaD;
		}
	}
	m_deltaDSqSum = m_deltaDSqSum*2.0;
}


void HitMDSSolver::preprocess()
{
	if (m_dataArray.size() == 0)
		return;

	if (!m_useExternalDisimilarityMat)
		computeDissimilarityMat();

	computeDMat();
	init2DPoint();
	computeD2DMat();
}

void HitMDSSolver::update2DPoint( int ithPoint , const QVector2D& newPos)
{
	m_position2D[ithPoint] = newPos;
	double oriDSum = 0;
	double newDSum = 0;
	for (int i = 0; i < m_position2D.size(); ++i)
	{
		double newDist = (m_position2D[ithPoint] - m_position2D[i]).lengthSquared();
		oriDSum += m_D2D(ithPoint,i);
		newDSum += newDist;
		m_D2D(ithPoint,i) = m_D2D(i,ithPoint) = newDist;
	}

	double n = m_position2D.size();
	double totElements = (n*(n-1)*0.5f);
	m_meanD2D = (m_meanD2D * totElements - oriDSum + newDSum) / totElements;
	m_deltaDSqSum2D = 0;
	for (int i = 0; i < m_position2D.size(); ++i)
	{
		m_deltaD2D(i,i)=0.0;
		for (int j = i+1; j < m_position2D.size(); ++j)
		{
			double d = m_D2D(i,j) - m_meanD2D;
			m_deltaD2D(i,j) = m_deltaD2D(j,i) = d;
			m_deltaDSqSum2D += d * d;
		}
	}
	m_deltaDSqSum2D = m_deltaDSqSum2D;
}

void HitMDSSolver::init2DPoint()
{
	qsrand(time(0));
	m_position2D.resize(m_dataArray.size());
	if (m_useExternalInitPos)
	{	// use external initialization
		for (int ithP = 0; ithP < m_position2D.size(); ++ithP)
			m_position2D[ithP] = m_dataArray[ithP].m_init2DPos;
	}
	else
	{   // random initialization
		Eigen::MatrixXd projMat = Eigen::MatrixXd::Random(2, m_dataArray[0].m_dataVec.size());
		for (int ithP = 0; ithP < m_position2D.size(); ++ithP)
		{
	//		Eigen::Vector2f v = projMat * m_dataArray[ithP].m_dataVec;

	//		m_position2D[ithP].setX(v[0]);
	//		m_position2D[ithP].setY(v[1]);
 			m_position2D[ithP].setX(qrand() / double(RAND_MAX));
 			m_position2D[ithP].setY(qrand() / double(RAND_MAX));
		}
	}

	m_D2D.resize(m_dataArray.size(), m_dataArray.size());
	m_deltaD2D.resize(m_dataArray.size(), m_dataArray.size());
}

QVector2D HitMDSSolver::compute2DPointGrad( int ithPoint )
{
	double invL = 1.0 / m_deltaDSqSum;			// deltaDSqSum = mo_D
	double invDeltaDSqSum2D = 1.0 / (m_deltaDSqSum2D * m_deltaDSqSum2D);
	double n = m_dataArray.size();
	double totElements = (n*(n-1)*0.5f);

	double beta = m_deltaD.cwiseProduct(m_deltaD2D).sum() * 0.5f;
	double deltaSqrt= m_deltaDSqSum2D;
	double deltaDSum   = m_deltaD.sum() * 0.5f * (-1.0 / totElements);
	double deltaDSum2D = m_deltaD2D.sum() * 0.5f * (-1.0 / totElements) * 2.0;
	double r    = beta / (m_deltaDSqSum * m_deltaDSqSum2D);

	VectorXd DbetaDd, DdeltaDd, DdDx, DdDy;
	DbetaDd.resize(n);
	DdeltaDd.resize(n);
	DdDx.resize(n);
	DdDy.resize(n);
	for (int i=0; i < n; ++i)
	{
		DbetaDd[i]  = deltaDSum + m_D(ithPoint,i) - m_meanD;
		DdeltaDd[i] = 0.5f / m_deltaDSqSum2D * (deltaDSum2D + 2.0 * (m_D2D(ithPoint,i) - m_meanD2D));
		QVector2D dPos2D = m_position2D[ithPoint] - m_position2D[i];
		DdDx[i]     = 2 * dPos2D.x();
		DdDy[i]     = 2 * dPos2D.y();
	}
	DbetaDd[ithPoint] = 0;
	DdeltaDd[ithPoint] = 0;

	VectorXd DrDd = invL * invDeltaDSqSum2D * (DbetaDd * m_deltaDSqSum2D - beta * DdeltaDd);
	double    DrDx = 0, DrDy = 0;
	for (int i = 0; i < n; ++i)
	{
		DrDx += DrDd[i] * DdDx[i];
		DrDy += DrDd[i] * DdDx[i];
	}

	double DsDr = -2.0 / (r*r*r);
	return QVector2D(DrDx, DrDy) * DsDr;
}

Eigen::MatrixXd HitMDSSolver::compute2DPointApproxGrad()
{
	int   n    = m_dataArray.size();
	double beta = m_deltaD.cwiseProduct(m_deltaD2D).sum();
	double delta= m_deltaDSqSum2D;

	//qDebug("stress = %f", beta / sqrt(m_deltaDSqSum * delta + 1e-10));

	double absBetaDelta = qAbs(beta) + qAbs(delta) + 1e-10;
	double mi_T = 2.0 * beta / absBetaDelta;
	double mo_T = 2.0 * delta/ absBetaDelta;

	Eigen::MatrixXd tmpT = m_deltaD2D * mi_T - m_deltaD * mo_T;
	for (int i = 0; i < tmpT.rows(); ++i)
	{
		for (int j = i+1; j < tmpT.cols(); ++j)
		{
			double t = tmpT(i,j) / (m_deltaD2D(i,j) + (0.1 + m_meanD2D));
			tmpT(i,j) = tmpT(j,i) = t;
		}
		tmpT(i,i) = 0;
	}

	Eigen::MatrixXd stepMat(2, n);
	for (int ithCol = 0; ithCol < n; ++ithCol)
	{
		QVector2D colSum;
		for (int ithRow = 0; ithRow < n; ++ithRow)
		{
			QVector2D d = m_position2D[ithRow] - m_position2D[ithCol];
			colSum += d * tmpT(ithRow, ithCol);
		}
		stepMat(0,ithCol) = colSum.x();
		stepMat(1,ithCol) = colSum.y();
	}

	return stepMat;
}

// double MDSSolver::computeStressFunc()
// {
// 	double beta = m_deltaD.cwiseProduct(m_deltaD2D).sum() * 0.5f;
// 	double r    = beta / (m_deltaDSqSum * m_deltaDSqSum2D);
// 
// 	// s = r ^ -2
// 	return 1.0 / (r*r);
// }

void MDSSolver::print2DPosition()
{
	for (int i=0; i < m_position2D.size(); ++i)
	{
		qDebug("%d th Pnt, \t%f \t%f", i, m_position2D[i].x(), m_position2D[i].y());
	}
}

void HitMDSSolver::computeD2DMat()
{
	m_meanD2D = 0;
	for (int i = 0; i < m_position2D.size(); ++i)
	{
		m_D2D(i,i)=0.0;
		for (int j = i+1; j < m_position2D.size(); ++j)
		{
			double d = (m_position2D[i] - m_position2D[j]).length();
			m_D2D(i,j) = m_D2D(j,i) = d;
			m_meanD2D += m_D2D(i,j);
		}
	}
	int n = m_dataArray.size();
	m_meanD2D = m_nNonZero ? m_meanD2D * 2.0 / (m_nNonZero) : m_meanD2D;

	m_deltaDSqSum2D = 0;
	for (int i = 0; i < m_position2D.size(); ++i)
	{
		m_deltaD2D(i,i)=0.0;
		double rowSum = 0;
		for (int j = i+1; j < m_position2D.size(); ++j)
		{
			double d = m_D(i,j) < 1e-10f ? 0 : m_D2D(i,j) - m_meanD2D;
			m_deltaD2D(i,j) = m_deltaD2D(j,i) = d;
			double v = m_D2D(i,j);
			rowSum += d * d;
		}
		m_deltaDSqSum2D += rowSum;
	}
	m_deltaDSqSum2D = m_deltaDSqSum2D * 2.0;
}

void MDSSolver::save2DPosToFile( const QString& fileName )
{
	QFile data(fileName);
	if (!data.open(QFile::WriteOnly | QFile::Truncate)) 
		return;

	QTextStream out(&data);
	//out << "pntMat =[";
	for (int ithP = 0; ithP < m_position2D.size(); ++ithP)
	{
		out << m_position2D[ithP].x() << " " << m_position2D[ithP].y() << ";\n";
	}
	//out << "]";
}

void MDSSolver::computeDissimilarityMat()
{
	m_D.resize(m_dataArray.size(), m_dataArray.size());
	for (int i = 0; i < m_dataArray.size(); ++i)
	{
		m_D(i,i)=0.0;
		for (int j = i+1; j < m_dataArray.size(); ++j)
		{
			double d = computeDissimilarity(i,j);
			m_D(i,j) = m_D(j,i) = d;
		}
	}
}

void MDSSolver::postProcess()
{
	if (m_dataArray.size() == 0)
		return;
	Eigen::VectorXd r(m_dataArray.size());
	//Eigen::VectorXd a(m_dataArray.size());
	for (int i = 0; i < m_dataArray.size(); ++i)
	{
		r(i) = m_dataArray[i].m_radius;
		//a(i) = m_dataArray[i].m_alignVal;
	}

	MDSPostProcesser postProcesser;
	postProcesser.setSparseFactor(m_sparseFactor);
	postProcesser.set2DPos(m_position2D, r);
	postProcesser.compute();
	postProcesser.getFinalPos(m_position2D);
}

bool MDSSolver::setExternalDissimilarityMat( const Eigen::MatrixXd& distMat )
{
	int r = distMat.rows();
	int c = distMat.cols();
	if (r != m_dataArray.size() || r != c)
		return false;
	m_useExternalDisimilarityMat = true;
	m_D = distMat;
	return true;
}



void StressMDSSolver::compute()
{
	if (m_dataArray.size() == 0)
		return;

	if (!m_useExternalDisimilarityMat)
		computeDissimilarityMat();

	Eigen::MatrixXd pos2D;
	if (!m_useExternalInitPos)
		computeInitPos(m_D, pos2D);

	// initialize data
	int nData = m_D.rows();
	StressMDSLib::Matrix<double> matD(nData, nData);
	StressMDSLib::Matrix<double> matX(nData, 2);
	double* weight = new double[nData];
	for (int r = 0; r < nData; ++r)
	{
		weight[r] = m_dataArray[r].m_radius * m_dataArray[r].m_radius;
		for (int c = 0; c < nData; ++c)
		{
			matD.set(r, c, double(m_D(r,c)));
		}
		if (m_useExternalInitPos)
		{
			QVector2D& initPos = m_dataArray[r].m_init2DPos;
			matX.set(r, 0, initPos.x());
			matX.set(r, 1, initPos.y());
		}
		else
		{
			matX.set(r, 0, pos2D(r,0));
			matX.set(r, 1, pos2D(r,1));
			double x = pos2D(r,0);
			double y = pos2D(r,1);
		}
	}
	
	// stress mds
	StressMDSLib::Matrix<double>* res = StressMDSLib::MDS_UCF(&matD, &matX, 2, 30, weight, -1.0);

	m_position2D.resize(nData);
	for (int r = 0; r < nData; ++r)
		m_position2D[r] = QVector2D(res->get(r,0), res->get(r,1));

	delete   res;
	delete[] weight;
	if (m_usePostProcess)
		postProcess();

}

void StressMDSSolver::computeInitPos( const Eigen::MatrixXd& D, Eigen::MatrixXd& res )
{
	MatrixXd r;
	ClassicalMDSSolver::compute(D, r, 2);
	res = r;
}



void ClassicalMDSSolver::compute()
{
	if (m_dataArray.size() == 0)
		return;

	if (!m_useExternalDisimilarityMat)
	{
		computeDissimilarityMat();
	}

	int nData = m_D.rows();
	MatrixXd resultMat(nData,2);
	compute(m_D, resultMat, 2);

	m_position2D.resize(nData);
	for (int ithData = 0; ithData < nData; ++ithData)
		m_position2D[ithData] = QVector2D(resultMat(ithData,0), resultMat(ithData,1));

	if (m_usePostProcess)
		postProcess();
}

void ClassicalMDSSolver::compute( const Eigen::MatrixXd& D, Eigen::MatrixXd& resultMat, int nDim /*= 2*/ )
{
	int nData = D.rows();
	double invNData = 1.0 / double(nData);
	MatrixXd J = MatrixXd::Identity(nData, nData) - MatrixXd::Constant(nData, nData, 1.0/double(nData));
	MatrixXd D2= D.cwiseProduct(D);
	//MatrixXd B = -0.5 * J * D2 * J;

	MatrixXd B = D2;
	VectorXd D2AvgCol = B.colwise().sum() * invNData;
	for (int ithCol = 0; ithCol < nData; ++ithCol)
	{
		double avgColValue = D2AvgCol[ithCol];
		for (int ithRow = 0; ithRow < nData; ++ithRow)
			B(ithRow, ithCol) -= avgColValue;
	}

	VectorXd D2AvgRow = B.rowwise().sum() * invNData;
	for (int ithRow = 0; ithRow < nData; ++ithRow)
	{
		double avgRowValue = D2AvgRow[ithRow];
		for (int ithCol = 0; ithCol < nData; ++ithCol)
			B(ithRow, ithCol) -= avgRowValue;
	}
	B *= -0.5;


	Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es;
	es.compute(B);
	const Eigen::VectorXd& val = es.eigenvalues();
	MatrixXd eigVec = es.eigenvectors();

	nDim = qMin(nDim, nData);
	resultMat.resize(nData, nDim);
	for (int i = 0; i < nDim; ++i)
	{
		int ithCol = eigVec.cols()-1-i;
		VectorXd col = eigVec.col(ithCol);
		double sum = 0;
		for (int ithVal = 0; ithVal < nData; ++ithVal)
		{
			double v = col[ithVal];
			sum += v > 0.0 ? v*v : -v*v;
		}
		double sign= sum > 0.0 ? 1.0 : -1.0;			// reverse eigen vector when necessary
		resultMat.col(i) = sign * val(ithCol) * col;
	}
}

void MDSPostProcesser::set2DPos(const Eigen::MatrixXd& pos2D, 
								const Eigen::VectorXd& radius,
								const Eigen::VectorXi* hashVec)
{
	m_oriPos = pos2D;
	m_radius = radius;
	m_nData  = pos2D.rows();
	if (hashVec)
		m_hashID = *hashVec;
	else
		m_hashID.setZero(pos2D.rows());
}

void MDSPostProcesser::set2DPos(const QVector<QVector2D>& pos2D, 
								const Eigen::VectorXd& radius,
								const Eigen::VectorXi* hashVec)
{
	int nData = pos2D.size();
	m_nData = nData;
	m_oriPos.resize(nData,2);
	for (int ithPos = 0; ithPos < nData; ++ithPos)
	{
		m_oriPos(ithPos,0) = pos2D[ithPos].x();
		m_oriPos(ithPos,1) = pos2D[ithPos].y();
	}

	m_radius = radius;
	if (hashVec)
		m_hashID = *hashVec;
	else
		m_hashID.setZero(nData);
}

void MDSPostProcesser::init2DPnt()
{
	// rotate data
	PCASolver pcaSolver;
	Eigen::VectorXd w = m_radius.cwiseProduct(m_radius); // use area as weight
	pcaSolver.setDataMatrix(m_oriPos, &w);
	pcaSolver.compute();
	pcaSolver.getResult(m_pcaPos);
 
// 	for (int i = 0; i < 2; ++i)
// 	{
// 		Eigen::VectorXd col = m_pcaPos.col(i);
// 		col = col.cwiseProduct(col);
// 		if (col.sum() < 0)
// 			m_pcaPos.col(i) *= -1.0;
// 	}

	//m_pcaPos = m_oriPos;
	// scale data points according to avgDist and avgSize
	m_radiusWithPadding = m_radius * (m_radiusPaddingRatio + 1) + Eigen::VectorXd::Constant(m_radius.size(), m_minPadding);

// 	double avgDist = computeAvgDist(m_pcaPos);
// 	double avgSize = m_radius.sum() / m_radius.size();
// 	double scaleRate = double(2 * avgSize * m_sparseFactor / avgDist);
// 
// 	if (avgDist > 1e-10 && avgSize > 1e-10)
// 	{
// 		m_pcaPos *= scaleRate;
// 	}

	double scaleRate = computeScaleRate(m_pcaPos, m_radiusWithPadding);
	m_pcaPos *= scaleRate;
}

double MDSPostProcesser::computeScaleRate( const Eigen::MatrixXd& datPnt , const Eigen::VectorXd& radiusVec)
{
	if (datPnt.rows() <= 1 || datPnt.cols() != 2)
		return 1.0;

	double dSum = 0, rSum = 0;
	const int nPnt = datPnt.rows();
	for (int ithPnt = 0; ithPnt < nPnt; ++ithPnt)
	{
		double minD = FLT_MAX, minDRatio = FLT_MAX, minR = FLT_MAX;
		for (int jthPnt = 0; jthPnt < nPnt; ++jthPnt)
		{
			if (ithPnt == jthPnt)
				continue;
			double dx = datPnt(ithPnt, 0) - datPnt(jthPnt, 0);
			double dy = datPnt(ithPnt, 1) - datPnt(jthPnt, 1);
			double d = sqrt(dx*dx + dy*dy);
			double sizeSum = radiusVec[ithPnt] + radiusVec[jthPnt];
			double dRatio = d / sizeSum;
			if (dRatio < minDRatio)
			{
				minDRatio = dRatio;
				minD = d;
				minR = sizeSum;
			}
		}
		dSum += minD;
		rSum += minR;
	}
	double avgD = dSum / nPnt, avgR = rSum / nPnt;
	if (avgD < FLT_EPSILON)
		return 1.f;
	return avgR *  m_sparseFactor / avgD;
}

double MDSPostProcesser::computeAvgDist( const Eigen::MatrixXd& datPnt )
{
	if (datPnt.rows() <= 1 || datPnt.cols() != 2)
		return 0.0;

	double dSum = 0;
	const int nPnt = datPnt.rows();
	for (int ithPnt = 0; ithPnt < nPnt; ++ithPnt)
	{
		double minD = FLT_MAX;
		for (int jthPnt = 0; jthPnt < nPnt; ++jthPnt)
		{
			if (ithPnt == jthPnt)
				continue;
			double dx = datPnt(ithPnt, 0) - datPnt(jthPnt, 0);
			double dy = datPnt(ithPnt, 1) - datPnt(jthPnt, 1);
			double d = sqrt(dx*dx + dy*dy);
			if (d < minD)
				minD = d;
		}
		dSum += minD;
	}
	return dSum / nPnt;
}


void MDSPostProcesser::forceDirectedLayout()
{
	m_resultPos = m_pcaPos;
	int nDat    = m_nData;
	//m_maxIter = 1000;
	int ithIter = 0;
	double totalOverlap = 0;
	double avgSize = m_radius.sum() / nDat;
	double stepFactor = 0.9; 
	double deltaStepFactor = 0.1;

	double*dPosBuf   = new double[nDat*2];
	double*resultBuf = m_resultPos.data();
	double*radiusBuf = m_radiusWithPadding.data();

	for (; ithIter < m_maxIter; ++ithIter)
	{
		for (double*pDPos = dPosBuf; pDPos < dPosBuf+nDat*2; ++pDPos)
			*pDPos = 0.0;

		totalOverlap = 0.0;
		for (int ithP = 0; ithP < nDat; ++ithP)
		{
			for (int jthP = ithP+1; jthP < nDat; ++jthP)
			{
				double* pResI = resultBuf + ithP;
				double* pResJ = resultBuf + jthP;
				double v[2] = {pResI[0] - pResJ[0], pResI[nDat] - pResJ[nDat]};
				double dist = sqrt(v[0]*v[0] + v[1]*v[1]);
				double sizeI = radiusBuf[ithP];
				double sizeJ = radiusBuf[jthP];
				double sizeSum = sizeI + sizeJ;
				double deltaSum = sizeSum - dist;
				if (deltaSum < avgSize * 1e-5)
					continue;

				if (dist < avgSize * 1e-5)
				{
					double sign = 1;
					if(m_hashID[ithP] != m_hashID[jthP])
						sign = m_hashID[ithP] > m_hashID[jthP] ? 1.0 : -1.0;
 					else if (abs(sizeI - sizeJ) > avgSize * 1e-3)
  						sign = sizeI > sizeJ ? 1.0 : -1.0;
					else 
					{
 						//qDebug("equal hash!!!!!!!!!!!! ri,rj %lf, %lf  hi,hj %d, %d i,j %d %d", sizeI, sizeJ,  m_hashID[ithP],  m_hashID[jthP], ithP, jthP);
						continue;
					}
 					v[0] = sign; v[1] = 0;
				}
				else
				{
					v[0] /= dist;
					v[1] /= dist;
				}
				totalOverlap += deltaSum;

				double totOffset = deltaSum * stepFactor;
				double massSum   = sizeI + sizeJ;
				double offsetI   = totOffset * sizeJ / massSum;
				double offsetJ   = totOffset * sizeI / massSum;

				dPosBuf[ithP] += v[0] * offsetI;
				dPosBuf[jthP] -= v[0] * offsetJ;
				dPosBuf[ithP+nDat] += v[1] * offsetI;
				dPosBuf[jthP+nDat] -= v[1] * offsetJ;
			} 
		}

		for (int i = 0; i < nDat*2; ++i)
			resultBuf[i] += dPosBuf[i];

		stepFactor  = qMin(stepFactor + deltaStepFactor, 1.5);
		deltaStepFactor *= 2.0;

		double avgOverlap = totalOverlap / nDat;
		if (avgOverlap < avgSize * 1e-4 && avgOverlap > m_minPadding)
			break;
	}

	delete[] dPosBuf;
	//qDebug("iteration times: %d, total overlap: %f, avgSize: %f, nData: %d", ithIter, totalOverlap, avgSize, nDat);
}

void MDSPostProcesser::compute()
{
	if (m_oriPos.rows() <= 0)
		return;
	else if (m_oriPos.rows() == 1)
	{
		m_resultPos = Eigen::MatrixXd::Zero(1,2);
		return;
	}

	init2DPnt();
	//m_resultPos = m_pcaPos;
	forceDirectedLayout();
	normalizeResult();
}

void MDSPostProcesser::getFinalPos( QVector<QVector2D>& pos2D )
{
	int nDat = m_resultPos.rows();
	if (nDat <= 0)
		return;

	pos2D.resize(nDat);
	for (int ithP = 0; ithP < nDat; ++ithP)
		pos2D[ithP] = QVector2D(m_resultPos(ithP,0), m_resultPos(ithP,1));
}

void MDSPostProcesser::getFinalPos( Eigen::MatrixXd& pos2D )
{
	pos2D = m_resultPos;
}

double MDSPostProcesser::getFinalRadius()
{
	double bound[2] = {	qMax(qAbs(m_minRect[0]), qAbs(m_maxRect[0])),
						qMax(qAbs(m_minRect[1]), qAbs(m_maxRect[1]))};
	return sqrt(bound[0]*bound[0] + bound[1]*bound[1]);
}


void CodeAtlas::MDSPostProcesser::getFinalBoundingRect( Eigen::VectorXd& minV, Eigen::VectorXd& maxV )
{
	if (m_resultPos.rows() <= 0 || m_resultPos.cols() != 2)
	{
		minV = maxV = Vector2d(0.0, 0.0);
		return;
	}
	minV = m_minRect;
	maxV = m_maxRect;
}

void CodeAtlas::MDSPostProcesser::normalizeResult()
{
	m_resultPos = m_resultPos * m_sizeFactor;
	int   nPnt = m_resultPos.rows();
	if (nPnt <= 0 || m_resultPos.cols()<= 0)
		return;

	GeometryUtility::moveToOrigin(m_resultPos, m_radius, m_maxRect);
	m_minRect = -m_maxRect;
}
