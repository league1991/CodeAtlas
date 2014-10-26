#include "StdAfx.h"
#include "WordLocator.h"

using namespace CodeAtlas;

WordLocator::WordLocator(void):m_topicCount(20), m_useTfIdfMeasure(true)
{
}

WordLocator::~WordLocator(void)
{
}

void WordLocator::setDocTermMat( const SparseMatrix& docTermMat, const Eigen::VectorXd& docRadius )
{
	m_docTermCountMat = docTermMat;
	m_docRadius  = docRadius;
}

bool WordLocator::compute(float sparseFactor)
{
	if (m_docTermCountMat.rows() <= 0 || m_docTermCountMat.cols() <= 0)
	{
		m_position2D = QVector<QVector2D>(1, QVector2D(0,0));
		return false;
	}

	if (m_useTfIdfMeasure)
		computeTfIdf(); 
	else
		m_docTermMat = m_docTermCountMat;
	computeLSI();
	compute2DPos(sparseFactor);
	compute2DStatistics(m_position2D, m_coordRange, m_radius);
	return true;
}

void WordLocator::computeLSI()
{
	SVDSolver solver;
	solver.setMat(m_docTermMat);
	solver.compute(m_topicCount);
	solver.getApproxMat(m_docTermMatLSI);
	solver.getUSqrtS(m_docTopicMatLSI);
}

void WordLocator::compute2DPos(float sparseFactor)
{
	if (m_docTermCountMat.rows() == 1)
	{
		m_position2D.clear();
		m_position2D.push_back(QVector2D(0,0));
		return;
	}

	StressMDSSolver solver;
	solver.setSparseFactor(sparseFactor);
	solver.setDissimilarityMeasure(MDSSolver::MDS_COS);
	solver.setUseExternalInitPos(false);
	solver.setUsePostProcess(true);
	for (int ithDoc = 0; ithDoc < m_docTermMatLSI.rows(); ++ithDoc)
	{
		MDSSolver::MDSData data;
		//data.m_dataVec = m_docTermMatLSI.row(ithDoc);
		data.m_dataVec   = m_docTopicMatLSI.row(ithDoc);
		data.m_init2DPos = QVector2D(m_docTopicMatLSI(ithDoc, 0), m_docTopicMatLSI(ithDoc, 1));
		data.m_radius    = m_docRadius[ithDoc];
		//data.m_alignVal  = 1.f;
		solver.addData(data);
	}

	solver.compute();
	//solver.print2DPosition();

	m_position2D = solver.get2DPosition();

}

void WordLocator::computeTfIdf()
{
	int nDoc = m_docTermCountMat.rows();
	int nTerm = m_docTermCountMat.cols();

	Eigen::VectorXd docCountPerWord = Eigen::VectorXd::Zero(nTerm);
	Eigen::VectorXd wordCountPerDoc = Eigen::VectorXd::Zero(nDoc);
	for (int jthTerm = 0; jthTerm < nTerm; ++jthTerm)
	{
		for (int ithDoc = 0; ithDoc < nDoc; ++ithDoc)
		{
			double count = m_docTermCountMat.coeff(ithDoc, jthTerm);
			if (count != 0)
				docCountPerWord[jthTerm]++;
			wordCountPerDoc[ithDoc]+= count;
		}
	}

	m_docTermMat = m_docTermCountMat;
	for (int jthTerm = 0; jthTerm < nTerm; ++jthTerm)
	{
		for (int ithDoc = 0; ithDoc < nDoc; ++ithDoc)
		{
			double freq = m_docTermCountMat.coeff(ithDoc, jthTerm);
			if (freq != 0)
			{
				float tfIdf = freq / wordCountPerDoc[ithDoc] * log(double(nDoc) / (docCountPerWord[jthTerm]));
				m_docTermMat.coeffRef(ithDoc, jthTerm) = tfIdf;					
			}
		}
	}
	m_docTermMat.makeCompressed();
}

void WordLocator::saveMatsToFile( const QString& fileName )
{
	QFile data(fileName);
	if (!data.open(QFile::WriteOnly | QFile::Truncate)) 
		return;

	QTextStream out(&data);

	out << "docTermCountMat =[";
	for (int ithR = 0; ithR < m_docTermMat.rows(); ++ithR)
	{
		for (int ithC = 0; ithC < m_docTermMat.cols(); ++ithC)
		{
			out << m_docTermCountMat.coeff(ithR, ithC) << "\t";
		}
		out << ";\n";
	}
	out << "];\n\n\n";

	out << "docTermMat =[";
	for (int ithR = 0; ithR < m_docTermMat.rows(); ++ithR)
	{
		for (int ithC = 0; ithC < m_docTermMat.cols(); ++ithC)
		{
			out << m_docTermMat.coeff(ithR, ithC) << "\t";
		}
		out << ";\n";
	}
	out << "];\n\n\n";

	out << "docTermMatLSI=[";
	for (int ithR = 0; ithR < m_docTermMatLSI.rows(); ++ithR)
	{
		for (int ithC = 0; ithC < m_docTermMatLSI.cols(); ++ithC)
		{
			out << m_docTermMatLSI.coeff(ithR, ithC) << "\t";
		}
		out << ";\n";
	}
	out << "];\n\n\n";


	out << "docTopicMatLSI=[";
	for (int ithR = 0; ithR < m_docTopicMatLSI.rows(); ++ithR)
	{
		for (int ithC = 0; ithC < m_docTopicMatLSI.cols(); ++ithC)
		{
			out << m_docTopicMatLSI.coeff(ithR, ithC) << "\t";
		}
		out << ";\n";
	}
	out << "];\n\n\n";


	out << "pos2d=[";
	for (int ithPos = 0; ithPos < m_position2D.size(); ++ithPos)
	{
		out << m_position2D[ithPos].x() << " " << m_position2D[ithPos].y() << ";\n";
	}
	out << "];\n\n\n";
}

void WordLocator::compute2DStatistics(const QVector<QVector2D>& position, QRectF& coordRange, float& radius)
{
	int nPnts = position.size();
	if (nPnts == 0)
		return;
	qreal minPnt[2] = {position[0].x(),position[0].y()};
	qreal maxPnt[2] = {position[0].x(),position[0].y()};

	for (int ithPnt = 1; ithPnt < nPnts; ++ithPnt)
	{
		const QVector2D& p = position[ithPnt];
		minPnt[0] = qMin(minPnt[0], p.x());
		minPnt[1] = qMin(minPnt[1], p.y());
		maxPnt[0] = qMax(maxPnt[0], p.x());
		maxPnt[1] = qMax(maxPnt[1], p.y());
	}

	coordRange.setLeft(minPnt[0]);
	coordRange.setTop(minPnt[1]);
	coordRange.setRight(maxPnt[0]);
	coordRange.setBottom(maxPnt[1]);

	qreal w = coordRange.width() * .5f;
	qreal h = coordRange.height() * .5f;
	radius = qSqrt(w*w + h*h);

}


void WordLocator::compute2DDist( const QVector<QVector2D>& position2D, float& avgDist2D, float& minDist2D, QVector2D& centroid )
{
	int nPnts = position2D.size();

	avgDist2D = 0.f;
	minDist2D = FLT_MAX;
	centroid = QVector2D(0,0);
	for (int ithPnt = 0; ithPnt < nPnts; ++ithPnt)
	{
		for (int jthPnt = ithPnt; jthPnt < nPnts; ++jthPnt)
		{
			float dist = (position2D[ithPnt] - position2D[jthPnt]).length();
			avgDist2D += dist;
			minDist2D = qMin(dist, minDist2D);
		}
		centroid += position2D[ithPnt];
	}
	avgDist2D = avgDist2D * 2.f / (nPnts * nPnts);
	centroid  /=  nPnts;
}

void WordLocator::getNormalized2DPositions( QVector<QVector2D>& pos, float avgDist, float* radius)
{
	float avgD, minD;
	QVector2D centroid;
	compute2DDist(m_position2D, avgD, minD, centroid);
	pos.resize(m_position2D.size());

	float scale = (avgD < 1e-10) ? 1 : (avgDist / avgD);
	for (int ithP = 0; ithP < m_position2D.size(); ++ithP)
		pos[ithP] = (m_position2D[ithP] - centroid) * scale;

	if (radius)
		*radius = m_radius * scale;
}

void WordLocator::movePoints( QVector<QVector2D>& pos, const QVector2D& offset )
{
	for (int ithP = 0; ithP < pos.size(); ++ithP)
	{
		pos[ithP] += offset;
	}
}
