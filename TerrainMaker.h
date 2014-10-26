#pragma once

namespace CodeAtlas{

class SceneProjData;
class TerrainMaker
{
public:
	TerrainMaker(void);
	~TerrainMaker(void);

	void drawBackground(QPainter *painter, const QRectF &rect, const QHash<SymbolPath, SceneProjData>& projects );
	// coordinate transform
	// transform scene coord scenePos to image coord pixelPos
	// w,h   width and height of image
	// sceneRect  scene position of image
	static QPoint sceneToImage( const QPointF& scenePos, const QRectF& sceneRect, const QSize& imgSize);

	// transform scene size to image size
	static QSize sceneToImage( const QSizeF& sceneSize, const QRectF& sceneRect, const QSize& imgSize );

	// transform scene size to image size
	static QRect sceneToImage( const QRectF& rect, const QRectF& sceneRect, const QSize& imgSize);

	// approximate e ^ (-x^2)
	static inline float fastKernel(float x2)
	{
		return 1.f / (1+x2*(1+0.5*x2));
	}
	static inline float fastSqrt(float x)
	{
		float xhalf = 0.5f*x;
		int i = *(int*)&x;
		i = 0x5f3759df - (i >> 1);			// 1st iteration
		x = *(float*)&i;
		x = x*(1.5f - xhalf*x*x);			// newton iteration
		return 1.f / x;
	}
private:
	static inline unsigned getColor(float h, float lightness = 1.f)
	{
		int i = qMin(unsigned(h * 4.f), sizeof(m_highColorMap)/sizeof(int)-1);
		unsigned c = m_highColorMap[i];
		unsigned char* pC = (unsigned char*)&c;
		pC[0] *= lightness;
		pC[1] *= lightness;
		pC[2] *= lightness;
		//pC[3] *= lightness;
		return c;
	}
	static float    m_projLOD;
	static float    m_itemAffectRadius;
	static unsigned m_highColorMap[25];
};

}