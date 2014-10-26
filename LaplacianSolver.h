#pragma once

using namespace Eigen;
class LaplacianSolver
{
public:
	typedef Eigen::GeneralizedSelfAdjointEigenSolver<Eigen::MatrixXd> GSEigenSolver;
	// compute laplacian eigen map
	static bool compute( const Eigen::MatrixXd& L, Eigen::MatrixXd& resultMat, int nDim = 2 );
};