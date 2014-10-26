#include "StdAfx.h"
#include "backgroundRenderer.h"

using namespace CodeAtlas;
BackgroundRenderer::BackgroundRenderer(void):
m_width(0), m_height(0), 
m_bufWidth(0), m_bufHeight(0),
m_maskBuf(NULL),
m_valDenominator(NULL), m_valNumerator(NULL), 
m_valBuf(NULL),
m_shadeBuf(NULL),
m_heightMapBuffer(NULL),
m_maxBuf(NULL),
m_territoryBuf(NULL),
m_lod(0.f),
m_bufRatioX(0.f),
m_bufRatioY(0.f),
m_seaLevel(0.5),
m_renderMode(RENDER_TERRAIN),
m_isShaded(false),
m_pixelFactor(1.f),
m_minLod(0.03),
m_maxLod(0.5)
{
	m_lightDir = QVector3D(1,1,2);
	m_lightDir.normalize();
	m_emptyTerritory = qRgba(255,255,255,255);
	setHeightMap(UISetting::getAltitudeMap());
	m_plainClr = qRgb(240,237,229);
}

BackgroundRenderer::~BackgroundRenderer(void)
{
	clear();
}

void BackgroundRenderer::buildKernelGrad(  const QPointF& center, 
										 const float radius, 
										 QRadialGradient& gradObj,
										 const QRgb centerClr,
										 const QRgb borderClr,
										 const int   nSegment)
{
	gradObj = QRadialGradient(center, radius, center);
	float delVal = 1.f / nSegment;
	const Channel* pCenterClr = (const Channel*)&centerClr;
	const Channel* pBorderClr = (const Channel*)&borderClr;
	for (float pos = 0.f; pos <= 1.0001f; pos+= delVal)
	{
		float factor = cubicKernel(pos);
		float invFactor = 1.f - factor;

		QRgb finalClr;
		Channel* pClr = (Channel*)&finalClr;
		pClr[0] = factor * pCenterClr[0] + invFactor * pBorderClr[0];
		pClr[1] = factor * pCenterClr[1] + invFactor * pBorderClr[1];
		pClr[2] = factor * pCenterClr[2] + invFactor * pBorderClr[2];
		pClr[3] = factor * pCenterClr[3] + invFactor * pBorderClr[3];
		gradObj.setColorAt(pos, QColor::fromRgba(finalClr));
	}
}

void BackgroundRenderer::buildEnhancedKernelGrad( const QPointF& center, const float radius, const float innerRadius, QRadialGradient& gradObj, const QRgb centerClr /*= 0xffffffff*/, const QRgb borderClr /*= 0x0*/, const int nSegment /*= 10*/ )
{
	gradObj = QRadialGradient(center, radius, center);
	gradObj.setColorAt(0.f, QColor::fromRgba(centerClr));

	float innerRatio = innerRadius/radius;
	gradObj.setColorAt(innerRatio, QColor::fromRgba(centerClr));

	float delVal = (1.f - innerRatio) / nSegment;
	const Channel* pCenterClr = (const Channel*)&centerClr;
	const Channel* pBorderClr = (const Channel*)&borderClr;
	for (float pos = innerRatio; pos <= 1.0001f; pos+= delVal)
	{
		float normalPos = (pos - innerRatio) / (1.f - innerRatio);
		float factor = cubicKernel(normalPos);
		float invFactor = 1.f - factor;

		QRgb finalClr;
		Channel* pClr = (Channel*)&finalClr;
		pClr[0] = factor * pCenterClr[0] + invFactor * pBorderClr[0];
		pClr[1] = factor * pCenterClr[1] + invFactor * pBorderClr[1];
		pClr[2] = factor * pCenterClr[2] + invFactor * pBorderClr[2];
		pClr[3] = factor * pCenterClr[3] + invFactor * pBorderClr[3];
		gradObj.setColorAt(pos, QColor::fromRgba(finalClr));
	}
}

void BackgroundRenderer::renderAffectsKernel( QPainter* painter, const QList<QRectF>& sites )
{
	painter->setCompositionMode(QPainter::CompositionMode_Multiply);
	painter->setPen(Qt::NoPen);
	QRgb whiteClr = qRgba(255,255,255,255);
	QRgb blackClr = qRgba(0,0,0,255);
	for (int ithRect = 0; ithRect < sites.size(); ++ithRect)
	{
		const QRectF& rect = sites[ithRect];
		float w = rect.width();
		float h = rect.height();
		float r = (w < h ? w : h) * 0.5f;
		QRadialGradient gradObj;
		buildEnhancedKernelGrad(rect.center(), r, r * 0.2, gradObj, blackClr, whiteClr);
		painter->setBrush(QBrush(gradObj));
		painter->drawRect(rect);
	}
}

void BackgroundRenderer::computeHeightBuffer(const QList<QRect>& rectArray, 
											 const QList<float>& entityRadiusArray,
										     const QVector<float>& valArray,
										     const QVector<QRgb>&  colorArray)
{
	const int w = m_bufWidth, h = m_bufHeight;
	const int w_1 = w-1;
	const int h_1 = h-1;

	const QRgb initTerritory = m_emptyTerritory;
	for (int i = 0; i < w*h; ++i)
	{
		m_valNumerator[i] = m_valDenominator[i] = m_maxBuf[i] = 0.f;
		m_maskBuf[i]= 1.f;
		m_territoryBuf[i] = initTerritory;
	} 

	for (int ithPnt = 0; ithPnt < rectArray.size(); ++ithPnt)
	{
		const QRectF&  rect  = rectArray[ithPnt];
		int cx = rect.center().x() * m_bufRatioX;
		int cy = rect.center().y() * m_bufRatioY;
		int cw = rect.width() * m_bufRatioX;
		int ch = rect.height() * m_bufRatioY;
		int radius = cw < ch ? cw/2+1 : ch/2+1;
		float entityRadius = entityRadiusArray[ithPnt] * m_bufRatioX;
		float radiusRatio = qMin(entityRadius / radius, 1.f);


		const float val  = valArray[ithPnt];
		const QRgb oriClr= colorArray.size() > ithPnt ? colorArray[ithPnt] : initTerritory;
		const QRgb clr   = qRgba(qRed(oriClr), qGreen(oriClr), qBlue(oriClr), 255);	// use alpha to distinguish place without any "country" from others

		const int r2  = radius * radius;
		int      minPnt[2] = {cx-radius, cy-radius};
		int      maxPnt[2] = {cx+radius, cy+radius};
		if (minPnt[0] >= w || minPnt[1] >= h ||
			maxPnt[0] < 0  || maxPnt[1] < 0)
			continue;

		minPnt[0] = minPnt[0] < 0 ? 0 : minPnt[0];
		minPnt[1] = minPnt[1] < 0 ? 0 : minPnt[1];
		maxPnt[0] = maxPnt[0] > w_1 ? w_1 : maxPnt[0];
		maxPnt[1] = maxPnt[1] > h_1 ? h_1 : maxPnt[1];

		int begOffset = minPnt[1]*w + minPnt[0];
		float* pDeno = m_valDenominator + begOffset;
		float* pNume = m_valNumerator   + begOffset;
		float* pMask = m_maskBuf        + begOffset;
		float* pMax  = m_maxBuf			+ begOffset;
		QRgb*  pClr  = m_territoryBuf   + begOffset;
		for (int y = minPnt[1]; y <= maxPnt[1]; ++y)
		{
			float* pDenoElem = pDeno;
			float* pNumeElem = pNume;
			float* pMaskElem = pMask;
			float* pMaxElem  = pMax;
			QRgb*  pClrElem  = pClr;
			for (int x = minPnt[0]; x <= maxPnt[0]; ++x, 
				  ++pDenoElem, ++pNumeElem, ++pMaskElem, ++pMaxElem, ++pClrElem)
			{
				int dx = x - cx;
				int dy = y - cy;
				int d2 = dx*dx + dy*dy;
				if (d2 > r2)
					continue;

				float t = fastSqrt(float(d2) / float(r2));
				float m = 1.f;
				if (t > radiusRatio)
				{
					float t0 = (t - radiusRatio) / (1.f - radiusRatio);
					m = cubicKernel(t0);
				}

				float w = invKernel(t);

 				*pDenoElem += w;
 				*pNumeElem += w*(val);
 				*pMaskElem *= (1.f-m);
				if (m >= *pMaxElem)
				{
					*pMaxElem = m;
					*pClrElem = clr;
				}
			}
			pDeno += w;
			pNume += w;
			pMask += w;
			pMax  += w;
			pClr  += w;
		}
	}

	for (int i = 0; i < w*h; ++i)
	{
		if (m_valDenominator[i] > 1e-5f)
			m_valBuf[i] = m_valNumerator[i] / m_valDenominator[i] * (1.f - m_maskBuf[i]);
		else
			m_valBuf[i] = 0.f;
	}
}

void BackgroundRenderer::initInternalBuffer( int w, int h )
{
	if (m_bufWidth != w || m_bufHeight != h)
	{
		clearInternalBuffer();
		m_valBuf         = new float[w*h];
		m_valDenominator = new float[w*h];
		m_valNumerator   = new float[w*h];
		m_maskBuf        = new float[w*h];
		m_shadeBuf		 = new float[w*h];
		m_maxBuf		 = new float[w*h];
		m_territoryBuf   = new QRgb[w*h];
		m_bufWidth = w;
		m_bufHeight= h;
	}
	for (int i = 0; i < w*h; ++i)
	{
		m_valBuf[i] = m_valNumerator[i] = m_valDenominator[i] = m_maxBuf[i] = 0.f;
		m_maskBuf[i] = m_shadeBuf[i] = 1.f;		
		m_territoryBuf[i] = m_emptyTerritory;
	}
}

void BackgroundRenderer::clearInternalBuffer()
{
	delete[] m_valBuf;
	delete[] m_valDenominator;
	delete[] m_valNumerator;
	delete[] m_maskBuf;
	delete[] m_shadeBuf;
	delete[] m_maxBuf; 
	delete[] m_territoryBuf;
	m_valBuf = m_valDenominator = m_valNumerator = m_maskBuf = m_shadeBuf = m_maxBuf = NULL;
	m_territoryBuf = NULL;
	m_bufWidth = m_bufHeight = 0;
}

void BackgroundRenderer::setHeightMap( const QImage& heightMap )
{
	m_heightMap = heightMap.scaled(HEIGHT_MAP_LENGTH,1,Qt::IgnoreAspectRatio, Qt::FastTransformation);
	m_heightMapBuffer = (const QRgb*)m_heightMap.constBits();
}

void BackgroundRenderer::setLightSrcDir( const QVector3D& dir )
{
	m_lightDir = dir;
	m_lightDir.normalize();
}

void BackgroundRenderer::computeShadeBuffer()
{
	const float lDir[3] = {m_lightDir.x(), m_lightDir.y(), m_lightDir.z()};
	const float dp = 1;
	int w= m_bufWidth, h = m_bufHeight;
	float* pShadeBuf = m_shadeBuf;
	float* pBuffer = pShadeBuf;
	const float pixelSize = m_lod * m_pixelFactor;

	if (m_isShaded)
	{
		float* pHeight = m_valBuf;
		for (int y = 0; y < h-1; ++y)
		{
			for (int x = 0; x < w-1; ++x, ++pBuffer, ++pHeight)
			{
				float dhx = *pHeight - *(pHeight+1);
				float dhy = *pHeight - *(pHeight+w);
				float nor[3] = {-dhx * pixelSize, -dhy*pixelSize, dp};

				float l         = fastSqrt(nor[0]*nor[0] + nor[1]*nor[1] + nor[2]*nor[2]);
				float dotProduct = (lDir[0] * nor[0] + lDir[1]* nor[1] + lDir[2] * nor[2]) / l; 
				*pBuffer = dotProduct;
			}
			*pBuffer = *(pBuffer-1);
			++pBuffer;   ++pHeight;
		}
		for (pBuffer = pShadeBuf+w*(h-1); pBuffer < pShadeBuf+w*h; ++pBuffer)
			*pBuffer = *(pBuffer-w);
	}
	else
	{
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x, ++pBuffer)
			{
				*pBuffer = 1.f;
			}
		}
	}
}

void BackgroundRenderer::initFinalBuffer( int w, int h )
{
	if (w != m_width || h != m_height)
	{
		m_width = w;
		m_height = h;
		m_finalImg = QImage(w,h, QImage::Format_ARGB32);
	}
}

void BackgroundRenderer::setRenderSize( int w, int h, float bufRatio /*= 0.25*/ )
{
	int heightW = w * bufRatio + 1;
	int heightH = h * bufRatio + 1;
	m_bufRatioX = float(heightW) / w;
	m_bufRatioY = float(heightH) / h;
	initInternalBuffer(heightW, heightH);
	initFinalBuffer(w,h);
}

void BackgroundRenderer::computeFinalMap()
{
	QRgb* finalBuf = (QRgb*)m_finalImg.bits();

	int finalW = m_width, finalH = m_height;
	int mipW   = m_bufWidth, mipH = m_bufHeight;
 
	float stepW = float(mipW-1) / float(finalW-1);
	float stepH = float(mipH-1) / float(finalH-1);

	float interW = (m_lod - m_minLod) / (m_maxLod - m_minLod);
	interW = max(min(interW, 1.f), 0.f);
	const int interWi     = (int)(interW * 256.f);

	QRgb* pFinal = finalBuf;
	// linear interpolation
	for (int y = 0; y < finalH; ++y)
	{
		const float mipPosH = y * stepH;
		const int mipPosHi = (int)mipPosH;

		const float   dy   = mipPosH - mipPosHi;
		const float one_dy = 1.f - dy;
		const int	dyi    = dy * 256.f;

		const int offsetY0 = mipPosHi*mipW;
		const int offsetY1 = dy < 1e-3 ? 0 : mipW;

		// height value buffer
		const float* pV0 = m_valBuf + offsetY0;
		const float* pV1 = pV0 + offsetY1;

		// shade buffer
		const float* pS0 = m_shadeBuf + offsetY0;
		const float* pS1 = pS0 + offsetY1;

		// color buffer
		const QRgb*  pC0 = m_territoryBuf + offsetY0;
		const QRgb*  pC1 = pC0 + offsetY1;

		for (int x = 0; x < finalW; ++x, ++pFinal)
		{
			const float mipPosW = x * stepW;
			const int mipPosWi = (int)mipPosW;
			const float dx     = mipPosW - mipPosWi;
			const int offsetX0 = mipPosWi;
			const int offsetX1 = dx < 1e-3 ? 0 : 1;

			const float*pV00  = pV0   + offsetX0;
			const float*pV01  = pV00  + offsetX1;
			const float*pV10  = pV1   + offsetX0;
			const float*pV11  = pV10  + offsetX1;

			const float one_dx = 1.f - dx;
			const float v0    = dx * (*pV01) + one_dx * (*pV00);
			const float v1    = dx * (*pV11) + one_dx * (*pV10);
			const float v     = v1 * dy      + v0     * one_dy;
			QRgb color  =  getHeightColor(v);

			if (v < m_seaLevel)
			{
				*pFinal = color;
				continue;
			}

			if (m_renderMode & RENDER_TERRAIN)
			{
				QRgb c = color;
				interpolate(&c, &m_emptyTerritory, &color, interWi);
			}
			else if ((m_renderMode & RENDER_COLOR))
			{
				// blend territory color
				const QRgb*pC00   = pC0   + offsetX0;
				const QRgb*pC01   = pC00  + offsetX1;
				const QRgb*pC10   = pC1   + offsetX0;
				const QRgb*pC11   = pC10  + offsetX1;

				const int dxi     = (int)(dx * 256.f);
				QRgb t0,t1,t;
				interpolate(pC00, pC01, &t0, dxi);
				interpolate(pC10, pC11, &t1, dxi);
				interpolate(&t0,  &t1,  &t,  dyi);

				int tAlpha = qAlpha(t);
				QRgb oriClr = color;
				interpolate(&oriClr, &t, &color, tAlpha);
			}
			else if ((m_renderMode & RENDER_PLAIN_CLR))
			{
				color = m_emptyTerritory;
			}

			if (m_isShaded)
			{
				const float*pS00  = pS0   + offsetX0;
				const float*pS01  = pS00  + offsetX1;
				const float*pS10  = pS1   + offsetX0;
				const float*pS11  = pS10  + offsetX1;

				const float s0    = dx * (*pS01) + one_dx * (*pS00);
				const float s1    = dx * (*pS11) + one_dx * (*pS10);
				const float s     = s1 * dy      + s0     * one_dy;
				const float shade = 0.3f + 0.7f*abs(s);

				Channel* pC = (Channel*)&color;
				pC[0] *= shade;
				pC[1] *= shade;
				pC[2] *= shade;
				pC[3]  = 255;
			}
			*pFinal = color;
		}
	}
}


void BackgroundRenderer::clear()
{
	clearInternalBuffer();
	m_finalImg = QImage();
	m_width = m_height = 0;
}

void BackgroundRenderer::render( const QList<QRect>& rectArray, 
								 const QList<float>& entityRadiusArray,
								 const QVector<float>& valArray,
								 const QVector<QRgb>& colorArray)
{
	if (!m_bufWidth || !m_bufHeight || !m_width || !m_height)
		return;

	computeHeightBuffer(rectArray, entityRadiusArray, valArray, colorArray);
	if (m_renderMode & RENDER_COLOR)
		computeTerritoryAlpha();
	computeShadeBuffer();
	computeFinalMap();
}

void BackgroundRenderer::initDefaultHeightMap()
{
	m_heightMap = QImage(HEIGHT_MAP_LENGTH, 1, QImage::Format_ARGB32);
	QRgb* pH    = (QRgb*) m_heightMap.bits();
	m_heightMapBuffer = pH;
	float dH    = 255.f / HEIGHT_MAP_LENGTH;
	float  h    = 0.f;
	for (int i = 0; i < HEIGHT_MAP_LENGTH; ++i, h += dH)
	{
		int v = h;
		pH[i] = qRgba(v,v,v,255);
	}
}

void CodeAtlas::BackgroundRenderer::computeTerritoryAlpha()
{
	const float* pMax = m_maxBuf;
	const float* pMask   = m_maskBuf; 
	QRgb*        pTerr   = m_territoryBuf;

	const int n = m_bufWidth * m_bufHeight;
	for (int i = 0; i < n; i++,
		 pMax++, pTerr++, pMask++)
	{
		float height0  = 1.f - *pMax;
		unsigned char* pTerrChar = (unsigned char*)pTerr;
		float alpha = 1.f - *pMask;
		alpha *= alpha;
		alpha *= alpha;
		alpha *= alpha;
		pTerrChar[3] =  255;
	} 
	const int blurR = 3;
	const float factor = 1.f / (blurR * 2.f + 1.f);
	const int w = m_bufWidth;
	const int h = m_bufHeight;
	for (QRgb* pRow = m_territoryBuf; pRow < m_territoryBuf + w*h; pRow += w)
	{
		for (QRgb* pElem = pRow+blurR; pElem<pRow+w-blurR; ++pElem)
		{
			int r=0,g=0,b=0,a=0;
			for (QRgb*pNeigh = pElem-blurR; pNeigh <= pElem+blurR; pNeigh++)
			{
				unsigned char* pElemChar = (unsigned char*)pNeigh;
				b += (*pElemChar++); 			g += (*pElemChar++); 
				r += (*pElemChar++); 			a += (*pElemChar);
			}
			r *= factor;		g *= factor;
			b *= factor;		a *= factor;
			*pElem = qRgba(r,g,b,a);
		}
	}

	const int begOffCol  = blurR * w;
	for (QRgb* pCol = m_territoryBuf+begOffCol; pCol < m_territoryBuf + begOffCol + w; ++pCol)
	{
		QRgb* pElem = pCol;
		for (int y = blurR; y < h - blurR; ++y, pElem += w)
		{
			int r=0,g=0,b=0,a=0;
			const int endNeiOffset = blurR * w;
			for (QRgb*pNeigh = pElem-blurR*w; pNeigh <= pElem+endNeiOffset; pNeigh+=w)
			{
				unsigned char* pElemChar = (unsigned char*)pNeigh;
				b += (*pElemChar++); 			g += (*pElemChar++); 
				r += (*pElemChar++); 			a += (*pElemChar);
			}
			r *= factor;		g *= factor;
			b *= factor;		a *= factor;
			*pElem = qRgba(r,g,b,a);
		}
	}
}
