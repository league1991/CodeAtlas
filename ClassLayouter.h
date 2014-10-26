#pragma once
#include "Layouter.h"
using namespace std;


namespace CodeAtlas
{

	class ClassLayouter: public Layouter
	{
	public:
		ClassLayouter():Layouter(), m_isMergeEdgeEnd(true){}
		~ClassLayouter(){}

		bool compute();
		void clear()
		{
			Layouter::clear();
			m_edgeParam.clear();
		}

		virtual void writeResults();
	private:
		bool computeNormally();
		bool computeCluster();

		bool m_isMergeEdgeEnd;
	};
}