#pragma once
#include "svdlib.h"

namespace CodeAtlas{
class SVDSolver
{
public:
	typedef Eigen::SparseMatrix<double> SparseMatrix;

	SVDSolver(void);
	~SVDSolver(void);

	// set sparse matrix by array
	// nNonZeros is the number of non-zero elements
	// colRangeArray records each column's idx range
	// rowIdxArray   records each element's row index
	void setMat(int nRows, int nCols, int nNonZeros, 
				const long* const colRangeArray, 
				const long* const rowIdxArray, 
				const double* const valueArray );

	void setMat(const SparseMatrix& mat);

	void setMat( const SMat mat );
	// compute svd decomposition
	// nDim is the number of vectors to compute
	void compute(int nDim=-1);

	// compute U * ¦² * V' , where ¦² is a matrix with first nDim sigular values
	// pValue is arranged in column major order
	void getApproxMat( double* const pValue );
	void getApproxMat( float*  const pValue );
	void getApproxMat(Eigen::MatrixXd& mat );
	bool getApproxMatWithoutFirst(float* const pValue);	// skip first singular value
	bool getApproxMatWithoutFirst(Eigen::MatrixXf& mat);

	// M = U * ¦² * V'
	void getU(Eigen::MatrixXf& U);						// U
	void getV(Eigen::MatrixXf& V);						// V
	void getS(Eigen::VectorXf& S);						// ¦²
	void getUSqrtS(Eigen::MatrixXd& USqrtS);			// U * sqrt(¦²)

	void print();
	void printApproxMat();
private:
	void clear();
	SMat   m_mat;
	SVDRec m_svdResult;
};
}
