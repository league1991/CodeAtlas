#pragma once
#include "NodeUIItem.h"
namespace CodeAtlas
{
	class UIUtility
	{
	public:
		static void drawArrow(QPainter* painter, const QPointF& headPnt, const QPointF& tailPnt, const float arrowHeadRatio, const float arrowWidthRatio);

		static void drawText( QPainter* painter, const QString& txt, const QFont& font, const QPointF& basePoint, unsigned alignFlag = Qt::AlignHCenter | Qt::AlignTop );

		static NodeUIItem::Ptr getUIItem( const SymbolNode::Ptr& node );

		static QColor adjustLightness(const QColor& color, float dLight)
		{
			qreal h,s,l,a;
			color.getHslF(&h, &s, &l, &a);
			return QColor::fromHslF(h,s,l+dLight, a);
		}

		// Rect
		template<typename RectType, typename ValType>
		inline static RectType adjustRect(const RectType& rect, ValType widthFactor, ValType heightFactor)
		{
			ValType dw = rect.width() * 0.5f * (widthFactor - 1.f);
			ValType dh = rect.height() * 0.5f * (heightFactor - 1.f);
			return rect.adjusted(-dw, -dh, dw, dh);
		}

		template<typename RectType, typename ValType>
		inline static RectType expandRect(const RectType& rect, ValType padding)
		{
			return rect.adjusted(-padding, -padding, padding, padding);
		}

		template<typename RectType, typename ValType>
		inline static RectType scaleRect(const RectType& rect, ValType factor)
		{
			ValType delWidth = rect.width() * (factor - 1) * 0.5;
			ValType delHeight= rect.height() * (factor - 1) * 0.5;
			return rect.adjusted(-delWidth, -delHeight, delWidth, delHeight);
		}

		template<typename RectType>
		inline static float rectRadius(const RectType& rect)
		{
			float d = rect.width() * rect.width() + rect.height() * rect.height();
			return 0.5f * sqrt(d);
		}

		
		// Size
		template<typename SizeType, typename ValType>
		inline static SizeType expandSize(const SizeType& s, ValType padding)
		{
			return SizeType(s.width()+padding, s.height() + padding);
		}

	};
}