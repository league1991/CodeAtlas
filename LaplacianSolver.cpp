#include "stdafx.h"
#include "LaplacianSolver.h"

bool LaplacianSolver::compute( const Eigen::MatrixXd& laplacian, Eigen::MatrixXd& resultMat, int nDim /*= 2 */ )
{
	int nData = laplacian.rows();
	if (nData <= 0 || laplacian.cols() != nData || nDim <= 0 || nDim > nData)
		return false;

	if (nData == 1)
	{
		resultMat = MatrixXd::Zero(1, nDim);
		return true;
	}

	// fill D
	Eigen::MatrixXd L = laplacian;
	Eigen::MatrixXd D = Eigen::MatrixXd::Zero(nData, nData);
	Eigen::VectorXd sqrtD(nData);
	for (int i = 0; i < nData; ++i)
	{
		D(i,i) = L(i,i);
		sqrtD(i) = sqrt(L(i,i));
	}

	// compute sqrtD^-1 * L * sqrtD^-1
	for (int ithRow = 0; ithRow < nData; ++ithRow)
		for (int ithCol = 0; ithCol < nData; ++ithCol)
			L(ithRow, ithCol) *= 1.0 / (sqrtD(ithRow) * sqrtD(ithCol));

	// compute eigen problem:  L * x = lambda * D * x
	// let    sqrtD = D^0.5; y = sqrtD * x;
	// so     sqrtD^-1 * L                     * x  = lambda *  sqrtD * x
	// =>     sqrtD^-1 * L * sqrtD^-1 * (sqrtD * x) = lambda * (sqrtD * x)
	// =>    (sqrtD^-1 * L * sqrtD^-1)*          y  = lambda *         y
	Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es;
	es.compute(L);
	const Eigen::VectorXd& eigValVec = es.eigenvalues();

	resultMat.resize(nData, nDim);
	Eigen::MatrixXd eigVecMat = es.eigenvectors();
//	bool isIll = qAbs(eigValVec[0]) > 1e-6;
// 	if (isIll)
// 	{
// 		QString s = "Laplacian=[\n";
// 		for (int i = 0; i < nData; ++i)
// 		{
// 			for (int j = 0; j < nData; ++j)
// 			{
// 				s += QString("%1 ").arg(laplacian(i,j));
// 			}
// 			s += "\n";
// 		}
// 		s += "]";
// 		qDebug() << s;
// 	}
	for (int i = 0; i < nDim; ++i)
//	for (int i = 0; i < nData; ++i)
	{
		//int ithCol = i;
		int ithCol = (nDim == nData) ? i : (i+1);

		Eigen::VectorXd eigVec = eigVecMat.col(ithCol);		// y
		double          eigVal = eigValVec[ithCol];			// lambda

		// x = sqrtD^-1 * y
		double sum = 0;
		for (int ithVal = 0; ithVal < nData; ++ithVal)
		{
			eigVec[ithVal] *= 1.0 / sqrtD(ithVal);			// y -> x
			double v = eigVec[ithVal];
			sum += v > 0.0 ? v*v : -v*v;
		}
		double sign= sum > 0.0 ? 1.0 : -1.0;				// reverse eigen vector when necessary

//		if (i < nDim)
		resultMat.col(i) = sign * eigVec;					// sign * x

// 		if (isIll)
// 		{
// 			qDebug("%d th Eigen, eigVal = %lf", i, eigVal);
// 			for (int j = 0; j < nData; ++j)
// 			{
// 				qDebug("vec[%d] = %lf", j, eigVec[j]);
// 			}
// 		}
		Eigen::VectorXd error = (laplacian - eigVal * D) * eigVec;
		if (error.norm() > 1e-8)
		{
			qDebug("Error too large!!!! nData = %d, error = %lf", nData, error.norm());
		}
	}
	return true;
}
