#include "StdAfx.h"
#include "svdlib.h"
#include "SVDSolver.h"

using namespace CodeAtlas;
SVDSolver::SVDSolver(void):m_mat(NULL), m_svdResult(NULL)
{
}

SVDSolver::~SVDSolver(void)
{
	clear();
}

void SVDSolver::clear()
{
	svdFreeSVDRec(m_svdResult);
	svdFreeSMat(m_mat);
	m_mat = NULL;
	m_svdResult = NULL;
}

void SVDSolver::setMat(int nRows, int nCols, int nNonZeros, 
					   const long* const colRangeArray, 
					   const long* const rowIdxArray, 
					   const double* const valueArray )
{
	clear();
	m_mat = svdNewSMat(nRows, nCols, nNonZeros);
	memcpy(m_mat->pointr, colRangeArray, sizeof(long)*(nCols+1));
	memcpy(m_mat->rowind, rowIdxArray, sizeof(long)*nNonZeros);
	memcpy(m_mat->value,valueArray, sizeof(double)*nNonZeros);
}

void SVDSolver::setMat( const SMat mat )
{
	if (!mat || !mat->pointr || !mat->rowind || !mat->value)
		return;

	clear();
	m_mat = svdNewSMat(mat->rows, mat->cols, mat->vals);
	memcpy(m_mat->pointr, mat->pointr, sizeof(long)*(mat->cols+1));
	memcpy(m_mat->rowind, mat->rowind, sizeof(long)*mat->vals);
	memcpy(m_mat->value,  mat->value,  sizeof(double)*mat->vals);
}

void SVDSolver::setMat( const SparseMatrix& mat )
{
	const double* valuePtr  = mat.valuePtr();
	const long* innerIdxPtr = (const long*)mat.innerIndexPtr();
	const long* outerIdxPtr = (const long*)mat.outerIndexPtr();
	int nRows = mat.rows();
	int nCols = mat.cols();
	int nNZeros = mat.nonZeros();
	setMat(nRows, nCols, nNZeros, outerIdxPtr, innerIdxPtr, valuePtr);
}

void SVDSolver::compute( int nDim/*=-1*/ )
{
	m_svdResult = svdLAS2A(m_mat, nDim);
}

void SVDSolver::print()
{
	if (!m_mat)
		return;

	SMat Mt = svdTransposeS(m_mat);
	printf("\nmatrix M:\n");
	for (int ithCol = 0; ithCol < Mt->cols; ++ithCol)
	{
		long beginIdx = Mt->pointr[ithCol];
		long endIdx   = Mt->pointr[ithCol+1];
		for (int idx = beginIdx; idx < endIdx; ++idx)
		{
			printf("%d %d %lf\n", ithCol, Mt->rowind[idx], Mt->value[idx]);
		}
	}
	svdFreeSMat(Mt);

	if (!m_svdResult || !m_svdResult->S || !m_svdResult->Ut || !m_svdResult->Vt)
		return;

	printf("\nU=[\n");
	DMat m = m_svdResult->Ut;
	for (int c = 0; c < m->cols; ++c)
	{
		for (int r = 0; r < m->rows; ++r)
		{
			printf("%.3lf\t", m->value[r][c]);
		}
		printf(";\n");
	}
	printf("];\n");

	printf("\nS=[\n");
	for (int ithVal = 0; ithVal < m_svdResult->d; ++ithVal)
	{
		printf("%.3lf\t", m_svdResult->S[ithVal]);
	}
	printf("]\n");

	printf("\nV=[\n");
	m = m_svdResult->Vt;
	for (int c = 0; c < m->cols; ++c)
	{
		for (int r = 0; r < m->rows; ++r)
		{
			printf("%.3lf\t", m->value[r][c]);
		}
		printf(";\n");
	}
	printf("]\n");
}

void SVDSolver::getApproxMat( double* const pValue )
{
	if (!m_svdResult || !m_svdResult->S || !m_svdResult->Ut || !m_svdResult->Vt)
		return;
	int  m  = m_mat->rows;
	int  d  = m_svdResult->d;
	int  n  = m_mat->cols;
	DMat us = svdNewDMat(m,d);

	for (int ithRow = 0; ithRow < m; ++ithRow)
	{
		for (int ithCol = 0; ithCol < d; ++ithCol)
		{
			us->value[ithRow][ithCol] = m_svdResult->Ut->value[ithCol][ithRow] * m_svdResult->S[ithCol];
		}
	}

	for (int ithRow = 0; ithRow < m; ++ithRow)
	{
		for (int ithCol = 0; ithCol < n; ++ithCol)
		{
			double& v = pValue[ithCol*m + ithRow];
			v = 0;
			for (int ithE = 0; ithE < d; ++ithE)
			{
				v += us->value[ithRow][ithE] * m_svdResult->Vt->value[ithE][ithCol];
			}
		}
	}

	svdFreeDMat(us);
}

void SVDSolver::getApproxMat( float* const pValue )
{
	if (!m_svdResult || !m_svdResult->S || !m_svdResult->Ut || !m_svdResult->Vt)
		return;
	int  m  = m_mat->rows;
	int  d  = m_svdResult->d;
	int  n  = m_mat->cols;
	DMat us = svdNewDMat(m,d);

	for (int ithRow = 0; ithRow < m; ++ithRow)
	{
		for (int ithCol = 0; ithCol < d; ++ithCol)
		{
			us->value[ithRow][ithCol] = m_svdResult->Ut->value[ithCol][ithRow] * m_svdResult->S[ithCol];
		}
	}

	for (int ithRow = 0; ithRow < m; ++ithRow)
	{
		for (int ithCol = 0; ithCol < n; ++ithCol)
		{
			double v = 0;
			for (int ithE = 0; ithE < d; ++ithE)
			{
				v += us->value[ithRow][ithE] * m_svdResult->Vt->value[ithE][ithCol];
			}
			pValue[ithCol*m + ithRow] = (float)v;
		}
	}

	svdFreeDMat(us);
}

void SVDSolver::getApproxMat( Eigen::MatrixXd& mat )
{
	mat.resize(m_mat->rows, m_mat->cols);
	double* pV = mat.data();
	return getApproxMat(pV);
}

void SVDSolver::printApproxMat()
{
	if (!m_svdResult || !m_svdResult->S || !m_svdResult->Ut || !m_svdResult->Vt)
		return;
	int  m  = m_mat->rows;
	int  n  = m_mat->cols;

	float* pMat = new float[m*n];
	getApproxMat(pMat);


	printf("\nM' =[\n");
	for (int r = 0; r < m; ++r)
	{
		for (int c = 0; c < n; ++c)
		{
			printf("%.3f\t", pMat[r + c*m]);
		}
		printf(";\n");
	}
	printf("]\n");
	delete[] pMat;
}

bool SVDSolver::getApproxMatWithoutFirst( float* const pValue )

{
	if (!m_svdResult || !m_svdResult->S || !m_svdResult->Ut || !m_svdResult->Vt || !m_svdResult->d)
		return false;
	int  m  = m_mat->rows;
	int  d  = m_svdResult->d;
	int  n  = m_mat->cols;
	DMat us = svdNewDMat(m,d);

	for (int ithRow = 0; ithRow < m; ++ithRow)
	{
		us->value[ithRow][0] = 0;
		for (int ithCol = 1; ithCol < d; ++ithCol)
		{
			us->value[ithRow][ithCol] = m_svdResult->Ut->value[ithCol][ithRow] * m_svdResult->S[ithCol];
		}
	}

	for (int ithRow = 0; ithRow < m; ++ithRow)
	{
		for (int ithCol = 0; ithCol < n; ++ithCol)
		{
			double v = 0;
			for (int ithE = 0; ithE < d; ++ithE)
			{
				v += us->value[ithRow][ithE] * m_svdResult->Vt->value[ithE][ithCol];
			}
			pValue[ithCol*m + ithRow] = (float)v;
		}
	}

	svdFreeDMat(us);
	return true;
}

bool SVDSolver::getApproxMatWithoutFirst( Eigen::MatrixXf& mat )
{
	mat.resize(m_mat->rows, m_mat->cols);
	float* pV = mat.data();
	return getApproxMatWithoutFirst(pV);
}

void SVDSolver::getU( Eigen::MatrixXf& U )
{
	if (!m_mat || !m_svdResult)
		return;
	int r = m_svdResult->Ut->cols;
	int c = m_svdResult->Ut->rows;
	U.resize(r,c);
	for (int i = 0; i < r; ++i)
	{
		for (int j = 0; j < c; ++j)
		{
			U(i,j) = m_svdResult->Ut->value[j][i];
		}
	}
}

void SVDSolver::getV( Eigen::MatrixXf& V )
{
	if (!m_mat || !m_svdResult)
		return;
	int r = m_svdResult->Ut->cols;
	int c = m_svdResult->Ut->rows;
	V.resize(r,c);
	for (int i = 0; i < r; ++i)
	{
		for (int j = 0; j < c; ++j)
		{
			V(i,j) = m_svdResult->Vt->value[j][i];
		}
	}
}

void SVDSolver::getS( Eigen::VectorXf& S )
{
	if (!m_mat || !m_svdResult)
		return;
	int c = m_svdResult->Ut->rows;
	S.resize(c);
	for (int i = 0; i < c; ++i)
	{
		S[i] = m_svdResult->S[i];
	}
}

void SVDSolver::getUSqrtS( Eigen::MatrixXd& USqrtS )
{
	if (!m_mat || !m_svdResult)
		return;
	int r = m_svdResult->Ut->cols;
	int c = m_svdResult->Ut->rows;
	USqrtS.resize(r,c);
	for (int i = 0; i < r; ++i)
	{
		for (int j = 0; j < c; ++j)
		{
			USqrtS(i,j) = m_svdResult->Ut->value[j][i] * m_svdResult->S[j];
		}
	}
}
