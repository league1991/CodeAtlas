#include "stdafx.h"
#include "GlobalUtility.h"

using namespace CodeAtlas;

QString GlobalUtility::toMatlabString( const Eigen::MatrixXf& mat, const QString& name )
{
	QString str = name + "=[\n";
	for (int i = 0; i < mat.rows(); ++i)
	{
		for (int j = 0; j < mat.cols(); ++j)
		{
			str += "\t" + QString::number(mat(i,j));
		}
		str += ";\n";
	}
	str += "];\n\n";
	return str;
}

bool GlobalUtility::saveString( const QString& fileName, const QString& content )
{
	QFile data(fileName);
	if (!data.open(QFile::WriteOnly | QFile::Truncate)) 
		return false;

	QTextStream out(&data);
	out << content;
	return true;
}

QString GlobalUtility::toMatlabString( const QList<QString>& strVec, const QString& name )
{
	QString str = name + "={\n";
	for (int i = 0; i < strVec.size(); ++i)
	{
		str += "\t'" + strVec[i] + "'\n";
	}
	str += "};\n\n";
	return str;
}
