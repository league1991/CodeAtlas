#include "StdAfx.h"
#include "PCASolver.h"

using namespace CodeAtlas;
PCASolver::PCASolver(void)
{
}

PCASolver::~PCASolver(void)
{
}

void PCASolver::compute()
{
	centralizeData();
	computeEigen();
}

void PCASolver::centralizeData()
{
	// compute weighted centroid
	int nData = m_dataMat.rows();
	int nDim  = m_dataMat.cols();
	Eigen::VectorXd avg = Eigen::VectorXd::Zero(nDim);
	for (int i = 0; i < nData; ++i)
		avg += m_dataMat.row(i) * m_weight(i);
	avg /= m_totalWeight;

	// centralized original data
	m_centralizedDataMat.resize(nData, nDim);
	for (int i = 0; i < nData; ++i)
	{
		for (int j = 0; j < nDim; ++j)
		{
			m_centralizedDataMat(i,j) = m_dataMat(i,j) - avg(j);
		}
	}
}

void PCASolver::computeEigen()
{
	// weighted covariance matrix
	// cov(X,Y) = ¦² (Xi-avgX)(Yi-avgY) * wi / ¦² w
	Eigen::MatrixXd weightedData = m_centralizedDataMat.transpose();
	for (int ithData = 0; ithData < weightedData.cols(); ++ithData)
		weightedData.col(ithData) *= m_weight[ithData] / m_totalWeight;
	Eigen::MatrixXd covMat = weightedData * m_centralizedDataMat; 

	Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es;
	es.compute(covMat);
	const Eigen::VectorXd& val = es.eigenvalues();
	m_eigVecMat = es.eigenvectors();
	for (int i = 0; i < m_eigVecMat.cols(); ++i)
	{
		double sign = m_eigVecMat.col(i).sum() > 0.0 ? 1.0 : -1.0;
		m_eigVecMat.col(i) *= sign;
	}
}

void PCASolver::getResult(Eigen::MatrixXd& resultMat, int nDim)
{
	if (m_eigVecMat.rows() <= 0)
		return;

	if (nDim <= 0 || nDim > m_eigVecMat.cols())
		nDim = m_eigVecMat.cols();

	resultMat.resize(m_dataMat.rows(), nDim);
	for (int i = 0; i < nDim; ++i)
	{
		int ithCol = m_eigVecMat.cols()-1-i;
		Eigen::VectorXd c = m_centralizedDataMat * m_eigVecMat.col(ithCol);

// 		Eigen::VectorXd absC = c.cwiseAbs();
// 		double sign = (c.dot(absC) > 0) ? 1.f : -1.f;
		double sign = 1.0;
		resultMat.col(i) = sign * c;		
	}

}

void PCASolver::setDataMatrix( const Eigen::MatrixXd& dataMat, const Eigen::VectorXd* pWeight /*= NULL*/ )
{
	m_dataMat = dataMat;
	if (pWeight && pWeight->size() == dataMat.rows())
	{
		m_weight = Eigen::VectorXd(dataMat.rows());
		for (int i = 0; i < dataMat.rows(); ++i)
			m_weight(i) = (*pWeight)[i];
	}
	else
		m_weight = Eigen::VectorXd::Ones(dataMat.rows());

	m_totalWeight = m_weight.sum();
}
