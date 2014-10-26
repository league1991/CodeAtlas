#pragma once

namespace CodeAtlas
{
	class FunctionUIItem:public NodeUIItem
	{
	public:
		FunctionUIItem(const SymbolNode::WeakPtr& node,  UIItem* parent = 0);
	private:
		void							paint(QPainter *painter, const QStyleOptionGraphicsItem *option/* =0 */, QWidget *widget /* = 0 */);
		void							getModeThreshold( float& minNormal, float& maxNormal, float& minFrozen, float& maxFrozen );
		virtual QList<UIItemModel::Ptr>&getUIItemModelList()const{return m_funcModelList;}
		void							initUIItemModel();

		void							drawVectorBuilding(QPainter* painter);

		void							initIconList();

		static QList<UIItemModel::Ptr>  m_funcModelList;

		static ImageArray				m_publicImgArray[3];
		static ImageArray				m_privateImgArray[3];
		static ImageArray				m_protectedImgArray[3];
	};



}