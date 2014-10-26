#pragma once

namespace CodeAtlas{
class PCASolver
{
public:
	PCASolver(void);
	~PCASolver(void);

	// set high dimemsional data, each row a data point, column a dimension
	// data is weighted by weight vector
	void setDataMatrix(const Eigen::MatrixXd& dataMat, const Eigen::VectorXd* pWeight = NULL);
	const Eigen::MatrixXd& getDataMatrix(){return m_dataMat;}

	// compute eigen decomposition
	void compute();

	// get result, return the eigen vectors of nDim largest eigen Values
	void getResult(Eigen::MatrixXd& resultMat, int nDim = -1);

	// compute ¦² di * wi
	// dataMat  each row a data point, column a dimension
	// weight   the weight of each data point
	// center   weighted center
	static void computeWeightedCenter(	const Eigen::MatrixXd& dataMat, 
										const Eigen::VectorXd& weight,
										Eigen::VectorXd& center)
	{
		center = weight * dataMat;
	}
private:
	void centralizeData();
	void computeEigen();

	Eigen::MatrixXd   m_dataMat;
	Eigen::VectorXd   m_weight;
	double             m_totalWeight;

	Eigen::MatrixXd   m_centralizedDataMat;
	Eigen::MatrixXd   m_eigVecMat;
	Eigen::MatrixXd   m_resultMat;
};
}
