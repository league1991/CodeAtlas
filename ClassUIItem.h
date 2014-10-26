#pragma once
namespace CodeAtlas
{
	class ClassUIItem:public NodeUIItem
	{
	public:
		ClassUIItem(const SymbolNode::WeakPtr& node,  UIItem* parent = 0);
		void			paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0, QWidget *widget = 0);
		QPainterPath    shape()const;
	private:
		void							getModeThreshold( float& minNormal, float& maxNormal, float& minFrozen, float& maxFrozen );
		virtual QList<UIItemModel::Ptr>&getUIItemModelList()const{return m_classModelList;}
		void							initUIItemModel();
		static QList<UIItemModel::Ptr>  m_classModelList;
	};

}