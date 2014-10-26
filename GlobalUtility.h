#pragma once
namespace CodeAtlas
{

class GlobalUtility
{
public:
	// get matlab matrix format: name = [...; ...];
	static QString toMatlabString(const Eigen::MatrixXf& mat, const QString& name);
	// get matlab row cell format:   name = {...  ...};
	static QString toMatlabString(const QList<QString>&  strVec, const QString& name);

	// save string to file
	static bool    saveString(const QString& fileName, const QString& content);
};

}