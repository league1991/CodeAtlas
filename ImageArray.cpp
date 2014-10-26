#include "stdafx.h"
#include "ImageArray.h"

using namespace CodeAtlas;

bool CodeAtlas::ImageArray::loadImage( const QImage& img, const QSize& elementSize )
{
	if (img.isNull() || elementSize.isEmpty())
		return false;

	m_imageList.clear();
	m_nRows = img.height() / elementSize.height();
	m_nCols = img.width() / elementSize.width();
	m_elementSize = elementSize;
	for (int i = 0; i < img.height(); i+= elementSize.height())
	{
		for (int j = 0; j < img.width(); j+= elementSize.width())
		{
			m_imageList.push_back(img.copy(j,i, elementSize.width(), elementSize.height()));
		}
	}
	return true;
}

const QImage& CodeAtlas::ImageArray::getImage( int ithRow, int ithCol )const
{
	ithRow = max(min(ithRow, m_nRows-1),0);
	ithCol = max(min(ithCol, m_nCols-1),0);
	return m_imageList[ithRow * m_nCols + ithCol];
}
