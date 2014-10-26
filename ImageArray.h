#pragma once

namespace CodeAtlas
{
	class ImageArray
	{
	public:
		ImageArray():m_nRows(0), m_nCols(0){}

		bool loadImage(const QImage& img, const QSize& elementSize);

		bool isEmpty()const{return m_imageList.size() == 0;}
		const QImage& getImage(int ithRow, int ithCol)const;
		
		int  getNumRows()const{return m_nRows;}
		int  getNumCols()const{return m_nCols;}
	private:
		QList<QImage> m_imageList;
		int m_nRows, m_nCols;
		QSize m_elementSize;
	};
}