#include "StdAfx.h"
#include "TerrainMaker.h"
using namespace CodeAtlas;

float TerrainMaker::m_projLOD = 4;
float TerrainMaker::m_itemAffectRadius = 3;

unsigned TerrainMaker::m_highColorMap[25]={
	// ocean
	0xff4c4d8d,
	0xff4779ae,
	0xff7eb1e2,
	0xffdeecfa,
	// land
	0xff327e33,
	0xff53a03e,
	0xff91ba45,
	0xffa6c848,
	0xffd0de5e,
	0xffe6ea6b,
	0xfff1f16b,
	0xfff2e259,
	0xfff1cb44,
	0xffefbc3e,
	0xffe99d3c,
	0xffe5813c,
	0xffe4503d,
	0xffd65d61,
	0xffbf7791,
	0xffa87c9e,
	0xff957c9f,
	0xff7584a5,
	0xffacbdcf,
	0xffe6ebf1,
	0xffffffff
};
TerrainMaker::TerrainMaker(void)
{
}

TerrainMaker::~TerrainMaker(void)
{
}

QPoint TerrainMaker::sceneToImage(		const QPointF& scenePos,
										const QRectF&  sceneRect,
										const QSize&   imgSize
										)
{
	QPoint pixelPos;
	float dx = scenePos.x() - sceneRect.left();
	float dy = scenePos.y() - sceneRect.top();
	float normalX = dx / sceneRect.width();
	float normalY = dy / sceneRect.height();
	pixelPos.setX(normalX * imgSize.width() );
	pixelPos.setY(normalY * imgSize.height());
	return pixelPos;
}

QSize TerrainMaker::sceneToImage( const QSizeF& sceneSize, const QRectF& sceneRect, const QSize& imgSize )
{
	QSize pixelSize;
	float normalSx = sceneSize.width()  / sceneRect.width();
	float normalSy = sceneSize.height() / sceneRect.height();
	pixelSize.setWidth(imgSize.width() * normalSx);
	pixelSize.setHeight(imgSize.height() * normalSy);
	return pixelSize;
}


void TerrainMaker::drawBackground(QPainter *painter, const QRectF &rect, 
								  const QHash<SymbolPath, SceneProjData>& projects )
{
	
	float lod = QStyleOptionGraphicsItem::levelOfDetailFromTransform(painter->worldTransform());

	QRect viewRect = painter->viewport();
	const int   w0 = viewRect.width();
	const int   h0 = viewRect.height();
	const int   scaleRatio = 4;
	const int   w = qMax(1, w0/scaleRatio);
	const int   h = qMax(1, h0/scaleRatio);
	QSize imgSize(w,h);

	float* kernelArray  = new float[w*h];		// store gaussian kernels' map
	float* highFieldArray= new float[w*h];		// store high field
	float* weightArray  = new float[w*h];
	float* maxValBuffer = new float[w*h];
	const  float oceanLevel = 0.05f; 
	for (int i = 0; i < w*h; ++i)
	{
		maxValBuffer[i] = oceanLevel;
		kernelArray[i] = 1.f;
		highFieldArray[i] = 0.f;
		weightArray[i] = 0.f;
	}

	for (QHash<SymbolPath, SceneProjData>::ConstIterator pProj = projects.constBegin();
		 pProj != projects.constEnd(); ++pProj)
	{
		const SceneProjData& projData = pProj.value();
		// ignore projects outside
		if (!rect.intersects(projData.getProjectRange()))
			continue;

		QPointF projPnt = projData.getProjectUIItem()->pos();
		QPoint  projPntImg = sceneToImage(projPnt, rect, imgSize);
		int     projLevel = projData.projLevel();
		int     nSymLevel = projData.nSymbolLevels();
		const QList<ClassUIOldItem*>& symList = projData.getClassUIItemList();
		for (int ithSym = 0; ithSym < symList.size(); ++ithSym)
		{
			// ignore items outside
			ClassUIOldItem* pItem = symList[ithSym];
			QRectF       itemRange = (pItem->getAffectRange().translated(projPnt));
			if (!rect.intersects(itemRange))
				continue;

			// get range in image coordinate
			QRect        itemRangeImg = sceneToImage(itemRange, rect, imgSize);
			float        radiusImg    = itemRangeImg.width() / 2;
			itemRangeImg &= QRect(QPoint(0,0), imgSize);
			if (itemRangeImg.isEmpty())
				continue;

			// fill array
			float        actualLevel = projLevel + 1 + float(pItem->getLevel()+1)/float(nSymLevel);
			QPointF      itemCenter = pItem->pos() + projPnt;
			QPoint       itemCenterImg = sceneToImage(itemCenter, rect, imgSize);
			for (int j = itemRangeImg.top(); j < itemRangeImg.bottom(); ++j)
			{
				for (int i = itemRangeImg.left(); i < itemRangeImg.right(); ++i)
				{
					int idx = j*w + i;
					int di = i - itemCenterImg.x();
					int dj = j - itemCenterImg.y();
					int d2 = di*di + dj*dj;

					float x2 = float(d2) * 20.f / (radiusImg*radiusImg);
					float w  = fastKernel(x2);
					kernelArray[idx] *= (1.f-w);

					float weight = 1.f / float(x2 + 1e-5f);
					highFieldArray[idx] += weight * actualLevel;
					weightArray[idx]    += weight;
				}
			}
		}
	}

	for (int i = 0; i < w*h; ++i)
	{
		highFieldArray[i] = (1.f-kernelArray[i]) * highFieldArray[i] / (weightArray[i] + 1e-5f);
	}

	QImage backgroundImg(w0, h0, QImage::Format_ARGB32);
	backgroundImg.fill(GlobalSetting::getOceanColor());
	QRect  imgRect = backgroundImg.rect();
	QRgb* pImgBuf = (QRgb*)backgroundImg.bits();

	// composite highField, use double linear interpolation
	const float dp = 0.45 / lod;
	QVector3D vecLight(1,-1,3);
	vecLight.normalize();
	const float scaleRatioF[2] = {float(w-1)/(w0), float(h-1)/(h0)};
	for (int y0 = 0; y0 < h0; ++y0)
	{
		float y = y0 * scaleRatioF[1];
		int   yi= (int)y;
		float dy = y-yi;

		for (int x0 = 0; x0 < w0; ++x0)
		{
			float x = x0 * scaleRatioF[0];
			int   xi = (int)x;
			float dx = x - xi;
			
			int   i = xi + yi * w;
			float f00   = highFieldArray[i];
			float f10   = highFieldArray[i+1];
			float f01   = highFieldArray[i+w];
			float f11   = highFieldArray[i+w+1];

			float fx0   = f00 + (f10 - f00) * dx;
			float fx1   = f01 + (f11 - f01) * dx;
			float f0y   = f00 + (f01 - f00) * dy;
			float f1y   = f10 + (f11 - f10) * dy;
			float fxy   = fx0 + (fx1 - fx0) * dy;

			float dfx = f1y - f0y;
			float dfy = fx1 - fx0;			
			float nor[3] = {-dfx, -dfy, dp};
			float l         = fastSqrt(nor[0]*nor[0] + nor[1]*nor[1] + nor[2]*nor[2]);
			float dotProduct = (vecLight.x() * nor[0] + vecLight.y()* nor[1] + vecLight.z() * nor[2]) / l; 
			pImgBuf[x0 + y0 * w0] = getColor(fxy, abs(dotProduct));
		}
	}
	delete[] kernelArray;
	delete[] weightArray;
	delete[] highFieldArray;
	/*
	for (int ithItem = 0; ithItem < m_classItems.size(); ++ithItem)
	{
		ClassUIItem& item = *m_classItems[ithItem];
		if (!rect.contains(item.pos()))
			continue;

		const QColor& color = lod < m_projLOD ? item.getProjTerritoryColor() : item.getTerritoryColor();
		QPoint centerPnt;
		QSize  fillSize;
		float radius = m_itemAffectRadius * item.getDisplayRadius();
		sceneToImage(item.pos(), rect, imgSize, centerPnt);
		sceneToImage(QSizeF(radius, radius),
			rect, imgSize, fillSize);

		QRect fillRange(centerPnt.x() - fillSize.width(),
			centerPnt.y() - fillSize.height(),
			fillSize.width() * 2, fillSize.height() * 2);

		fillRange &= imgRect;
		for (int j = fillRange.top(); j < fillRange.bottom(); ++j)
		{
			for (int i = fillRange.left(); i < fillRange.right(); ++i)
			{
				int di = i - centerPnt.x();
				int dj = j - centerPnt.y();
				int d2 = di*di + dj*dj;
				float ratio = radius / fillSize.width();
				float sceneD2 = ratio * ratio * d2  / (radius * radius);

				float weight = 1.f / (1.f + sceneD2 * 50);
				float *pWeightRef = maxValBuffer + j * imgSize.width() + i;
				if (weight > *pWeightRef)
				{
					*pWeightRef = weight;
					pImgBuf[j * imgSize.width() + i] = color.rgba();
				}
			}
		}
	}*/

	//painter->setRenderHint(QPainter::SmoothPixmapTransform);
	painter->drawImage(rect, backgroundImg);
	delete[] maxValBuffer;
}

QRect CodeAtlas::TerrainMaker::sceneToImage( const QRectF& rect, const QRectF& sceneRect, const QSize& imgSize)
{
	QRect pixelRect;
	float dx = rect.left() - sceneRect.left();
	float dy = rect.top()  - sceneRect.top();
	float normalX = dx / sceneRect.width();
	float normalY = dy / sceneRect.height();
	pixelRect.setLeft(normalX * imgSize.width());
	pixelRect.setTop(normalY * imgSize.height());
	float normalSx = rect.width()  / sceneRect.width();
	float normalSy = rect.height() / sceneRect.height();
	pixelRect.setWidth(imgSize.width() * normalSx);
	pixelRect.setHeight(imgSize.height() * normalSy);
	return pixelRect;
}
