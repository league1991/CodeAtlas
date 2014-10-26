#pragma once
namespace CodeAtlas
{
#define TOTAL_MODES 5
	enum UIInteractMode
	{
		// when item is interactive
		UI_NORMAL     = 0,
		UI_HOVERING   = 1,
		UI_SELECTED   = 2,
		UI_FROZEN	  = 3,
		UI_INVISIBLE  = 4
	};

	class NodeUIItem;
	// ui model used to represent different ui behavior
	class UIItemModel
	{
	public:
		typedef QSharedPointer<UIItemModel> Ptr;
		UIItemModel();
		UIItemModel(unsigned rectAlignMode, float wFactor, float hFactor);

		virtual QPainterPath	shape(const NodeUIItem& item) = 0;
		virtual void			paint(const NodeUIItem& item,
									  QPainter *painter, const QStyleOptionGraphicsItem *option)=0;

		// bounding rectangle
		void					setRectStretchFactor(float wFactor, float hFactor){m_wFactor = wFactor; m_hFactor = hFactor;}
		void					setEntityStretchFactor(float wFactor, float hFactor){m_entityWFactor = wFactor; m_entityHFactor = hFactor;}
		void					setRectAlignMode(unsigned mode){m_rectAlignMode = mode;}

		// clamp item actual size
		// if the actual lod is out of range, scale item to the same view size as in the threshold
		void					setViewSizeRange(float minSize, float maxSize){m_minViewSize = minSize; m_maxViewSize = maxSize;}

		static QRectF			centerScaleRect(const QRectF& rect, float wFactor, float hFactor);

		// pen, brush and font
		void					setDefaultPen(const QPen& pen);
		void					setDefaultBrush(const QBrush& brush);
		void					setDefaultFont(const QFont& font);
		void					setDefaultScaleBehavior(bool isScaleInvariant);
		void					setPen(const QPen& pen, UIInteractMode mode){m_pen[mode] = pen;}
		void					setBrush(const QBrush& brush, UIInteractMode mode){m_brush[mode] = brush;}
		void					setFont(const QFont&font, UIInteractMode mode){m_font[mode] = font;}


	protected:
		QRectF					computeRect(const QRectF& rect, float viewLod);
		SymbolInfo				getNodeSymInfo(const NodeUIItem& item);
		void					drawText(	QPainter* painter, const QString& txt, const QFont& font, 
											const QPointF& basePoint, unsigned alignFlag = Qt::AlignHCenter | Qt::AlignTop);
		float					m_wFactor, m_hFactor;
		float					m_entityWFactor, m_entityHFactor;
		unsigned				m_rectAlignMode;
		float					m_minViewSize,m_maxViewSize;

		QPen					m_pen[TOTAL_MODES];
		QBrush					m_brush[TOTAL_MODES];
		QFont					m_font[TOTAL_MODES];
		bool					m_isScaleInVariant[TOTAL_MODES];
	};

	class CircleItemModel:public UIItemModel
	{
	public:
		typedef QSharedPointer<CircleItemModel> Ptr;
		CircleItemModel();
		virtual QPainterPath    shape(const NodeUIItem& item);
		virtual void			paint(	const NodeUIItem& item,QPainter *painter, 
											const QStyleOptionGraphicsItem *option/* =0 */);
	protected:
	};

	class TextItemModel:public UIItemModel
	{
	public:
		typedef QSharedPointer<TextItemModel> Ptr;
		TextItemModel();

		void					setFontShowSizeRange(float minSize, float maxSize){m_minShowSize = minSize; m_maxShowSize = maxSize;}
		void					setHasShape(bool hasShape){m_hasShape = hasShape;}
		void					setTxtAlignMode(unsigned mode){m_txtAlignMode = mode;}		
		virtual QPainterPath    shape(const NodeUIItem& item);
		virtual	void			paint(const NodeUIItem& item,QPainter *painter, const QStyleOptionGraphicsItem *option);
	protected:
		float					m_minShowSize;
		float					m_maxShowSize;
		bool					m_hasShape;
		unsigned				m_txtAlignMode;
	};

	class VariableItemModel: public UIItemModel
	{
	public:
		typedef QSharedPointer<VariableItemModel> Ptr;
		VariableItemModel(){}
		QPainterPath	shape(const NodeUIItem& item);
		void			paint(const NodeUIItem& item, QPainter *painter, const QStyleOptionGraphicsItem *option);
	private:
		void			drawTreeShape(QPainter* painter, const QRectF& entityRect);
	};


	class FunctionItemModel: public UIItemModel
	{
	public:
		typedef QSharedPointer<FunctionItemModel> Ptr;
		FunctionItemModel(){}
		QPainterPath	shape(const NodeUIItem& item);
		void			paint(const NodeUIItem& item, QPainter *painter, const QStyleOptionGraphicsItem *option);
	private:
	};
}