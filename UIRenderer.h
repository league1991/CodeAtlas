#pragma once
namespace CodeAtlas
{

	class UIRenderer
	{
	public: 
		enum EffectType
		{
			EFFECT_NORMAL = 0,
			EFFECT_LIGHTEN= 1,
			EFFECT_HIGHLIGHTED = 2, 
		};

		UIRenderer(float minSize = 10.f, float maxSize = 500.f):m_minSize(minSize), m_maxSize(maxSize){}
		void setShowSizeRange(float minSize, float maxSize);
	protected:
		bool   checkRectSize(QPainter* painter, const QRectF& rect);
		float  m_minSize, m_maxSize;
	};

	class IconRenderer: public UIRenderer
	{
	public:
		IconRenderer():m_darkenColor(50,50,50,255), m_lightenColor(50,50,50,255){}

		void setDarkenColor(const QColor& darkenClr){m_darkenColor = darkenClr;}
		void setLightenColor(const QColor& lightenClr){m_lightenColor = lightenClr;}

		void paint(QPainter* painter, const QImage& img, const QRectF& rect, EffectType mode = EFFECT_NORMAL);
	private:

		QColor m_lightenColor;
		QColor m_darkenColor;
	};

	class TextRenderer: public UIRenderer
	{
	public:
		TextRenderer():UIRenderer(10.f,100.f), m_font("Arial",20)
		{
			m_textColor[0] = QColor(50,50,50,160);
			m_textColor[1] = QColor(50,50,50,255);
			m_textColor[2] = QColor(100,0,0,255);
		}
		void paintRect( QPainter* painter, const QString& text, const QRectF& rect, 
						EffectType mode        = UIRenderer::EFFECT_NORMAL , 
						unsigned int alignFlag = Qt::AlignHCenter|Qt::AlignTop);
		//void paintBelow(QPainter* painter, const QString& text, const QRectF& rect, EffectType mode = UIRenderer::EFFECT_NORMAL);
	private:
		QRectF getTextRect(const QRectF& rect)
		{
			QSizeF inSize = rect.size();
			const float  w = inSize.width();
			const float  h = inSize.height();
			const float  newH = h * 0.3f;
			const float  newW = w * 1.f;
			const float  wOffset = 0.5 * (newW - w);
			const float  hOffset = 0.5 * (h - newH);
			return rect.adjusted(0, hOffset, 0, 0);
		}
		QFont  m_font;
		QColor m_textColor[3];
		QColor m_textDarkenColor;
		QColor m_textLightenColor;
	};

	class CircleRenderer:public UIRenderer
	{
	public:
		static void drawSolidCircle(const QRectF& rect, const QColor& color, QPainter* painter)
		{
			painter->setPen(Qt::NoPen);
			painter->setBrush(QBrush(color));
			painter->drawEllipse(rect);
		}
		static void drawEmptyCircle(const QRectF& rect, const QColor& color, QPainter* painter, float width = 2, Qt::PenStyle style = Qt::DashLine)
		{
			QPen pen(style);
			pen.setColor(color);
			pen.setWidthF(width);
			painter->setPen(pen);
			painter->setBrush(Qt::NoBrush);
			painter->drawEllipse(rect);
		}
	};
}