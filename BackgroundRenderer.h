 #pragma once

namespace CodeAtlas{

#define HEIGHT_MAP_LENGTH	256
class BackgroundRenderer
{
public:
	typedef unsigned char Channel;
	BackgroundRenderer(void);
	~BackgroundRenderer(void);
 
	enum RenderMode
	{
		RENDER_TERRAIN  = 0x1,
		RENDER_COLOR	= (0x1) << 1,
		RENDER_PLAIN_CLR= (0x1) << 2,
	};
	static void			renderAffectsKernel(QPainter* painter,
											const QList<QRectF>& sites);

	void				setRenderSize(int w, int h, float bufRatio = 0.25);
	void				setHeightMap(const QImage& heightMap);
	void				setLightSrcDir(const QVector3D& dir);
	void				setLod(float lod){m_lod = lod;}
	void				setPixelSizeFactor(float factor){m_pixelFactor = factor;}
	void				setRenderMode(RenderMode mode){m_renderMode = mode;}
	void				setIsShaded(bool isShaded){m_isShaded = isShaded;}
	void				setEmptyTerritory(QRgb t){m_emptyTerritory = t;}
	void				setLodInterLimit(float minLod, float maxLod){m_minLod = minLod; m_maxLod = maxLod;}
	void				clear();

	void				render( const QList<QRect>& rectArray, 
								const QList<float>& entityRadiusArray, 
								const QVector<float>& valArray, 
								const QVector<QRgb>& colorArray = QVector<QRgb>());

	QImage&				getFinalImage(){return m_finalImg;}
	inline QRgb			getHeightColor(float h)
	{
		int idx = int(h * 20.f + 10);
		idx = (idx >= HEIGHT_MAP_LENGTH) ? (HEIGHT_MAP_LENGTH-1) : idx;
		return m_heightMapBuffer[idx];
	}

	inline QRgb			getHeightColorWithLod(float h, float lod)
	{
		QRgb c = getHeightColor(h);
		if (h < m_seaLevel)
			return c;
		QRgb res;
		float interW = (lod - m_minLod) / (m_maxLod - m_minLod);
		interW = max(min(interW, 1.f), 0.f);
		const int interWi     = (int)(interW * 256.f);
		interpolate(&c, &m_emptyTerritory, &res, interWi);
		return res;
	}
private:
	void				clearInternalBuffer();
	void				initInternalBuffer(int w, int h);
	void				initFinalBuffer(int w, int h);
	void				initDefaultHeightMap();

	// computation function
	// compute height buffer based on items
	void				computeHeightBuffer(const QList<QRect>& rectArray, 
											const QList<float>& entityRadiusArray, 
											const QVector<float>& valArray, 
											const QVector<QRgb>& colorArray);

	// compute shader based on grad of height buffer
	void				computeShadeBuffer();
	// compute territory buffer
	void				computeTerritoryAlpha();
	// compute final height map
	void				computeFinalMap();


	static void			buildKernelGrad(const QPointF& center, 
										const float radius, 
										QRadialGradient& gradObj,
										const QRgb centerClr = 0xffffffff,
										const QRgb borderClr = 0x0,
										const int   nSegment = 10);

	static void		buildEnhancedKernelGrad(const QPointF& center, 
											const float radius,
											const float innerRadius,
											QRadialGradient& gradObj,
											const QRgb centerClr = 0xffffffff,
											const QRgb borderClr = 0x0,
											const int   nSegment = 10);

	
	// cublic kernel, only defined in [0,1]
	static inline float	cubicKernel(float r)
	{
		return r * r * (2.f*r - 3.f) + 1.f;
	}

	// (r-1)^2, only defined in [0,1]
	static inline float quadricKernel(float r)
	{
		float r_1 = r - 1.f;
		return r_1 * r_1;
	}
	// -1/(r*(r-2))-1, only defined in [0,1]
	static inline float invKernel(float r)
	{
		return -1.f / (r*(r-2.f))-1.f;
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

	static inline void  interpolate(const QRgb* c0, const QRgb* c1, QRgb* res, unsigned w)
	{
		const unsigned char* c0Char  = (const unsigned char*)c0;
		const unsigned char* c1Char  = (const unsigned char*)c1;
		unsigned char*       resChar = (unsigned char*)res;
		const unsigned       one_w   = 256 - w; 
		*resChar = (*c0Char * one_w + *c1Char * w) >> 8;  c0Char++; c1Char++; resChar++;
		*resChar = (*c0Char * one_w + *c1Char * w) >> 8;  c0Char++; c1Char++; resChar++;
		*resChar = (*c0Char * one_w + *c1Char * w) >> 8;  c0Char++; c1Char++; resChar++;
		*resChar = (*c0Char * one_w + *c1Char * w) >> 8;
	}
	// final image size
	QImage		m_finalImg;
	int			m_width;
	int			m_height;
	float		m_lod;
	float		m_pixelFactor;

	// look-up image to encode high information
	QImage	    m_heightMap;				// convert height value to color
	const QRgb*	m_heightMapBuffer;
	float		m_seaLevel;					// sea level
	QRgb		m_plainClr;					// when terrain or custom clr is not used, use this clr

	// shading param, using lambert model
	float*		m_shadeBuf;					// shading buffer
	QVector3D   m_lightDir;					// light direction, point out

	// height buffer
	float       m_bufRatioX, m_bufRatioY;
	float*		m_maskBuf;
	float*		m_valDenominator;
	float*      m_valNumerator;
	float*		m_valBuf;
	float*		m_maxBuf;					// encode territory
	QRgb*		m_territoryBuf;				// territory buffer
	QRgb		m_emptyTerritory;

	int			m_bufWidth;
	int			m_bufHeight;

	RenderMode	m_renderMode;
	bool		m_isShaded;
	float		m_minLod, m_maxLod;
};

}