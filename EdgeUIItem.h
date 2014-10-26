#pragma once
#include "NodeUIItem.h"
namespace CodeAtlas
{
	class EdgeUIItem:public UIItem
	{
	public:
		typedef QSharedPointer<EdgeUIItem> Ptr;

		EdgeUIItem( const SymbolEdge::WeakPtr& edge, UIItem* parent = NULL );
		virtual ~EdgeUIItem();

		// virtual function that must be implemented
		QRectF			boundingRect() const{return m_path.boundingRect();}
		void			paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0, QWidget *widget = 0);
		QPainterPath	shape()const;

		static EdgeUIItem::Ptr creator( const SymbolEdge::Ptr& edge, NodeUIItem* parent );
		void			buildUI();

		float			getEdgeWidth()const{return m_edgeWidth;}
		float			getInvariantEdgeViewWidth()const;
		float			getHighlightedEdgeViewWidth()const;
		void			setEdgeWidthToDraw(float width){m_edgeWidthToDraw = width;}

		void			setDisplayAlpha(float alpha){m_displayAlpha = alpha;}

		bool			getSymbolType(SymbolInfo::ElementType& srcType, SymbolInfo::ElementType& tarType);

		inline static float   getEdgeBaseZValue(){return s_edgeBaseZValue;}
	protected:
	private:		
		float			computeEdgeWidth();
		void			drawArrow(QPainter* painter, const QColor& color);
		void            isNodeGlobal(bool& isSrcGlobal, bool& isTarGlobal);
		SymbolEdge::WeakPtr m_edge;
		EdgeUIElementAttr*  m_uiAttr;
		QPainterPath		m_path;
		float				m_level;

		float				m_edgeWidth;
		float				m_edgeWidthToDraw;

		float				m_displayAlpha;

		// shape data, use mutable keyword to avoid const constraint
		mutable QPainterPath		m_shapePath;
		mutable float				m_shapeWidth;

		static float		s_edgeBaseZValue;
	};

}