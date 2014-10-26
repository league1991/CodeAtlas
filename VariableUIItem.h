#pragma once

namespace CodeAtlas
{
	class VariableUIItem:public NodeUIItem
	{
	public:
		VariableUIItem(const SymbolNode::WeakPtr& node,  UIItem* parent = 0);
		//	void			paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0, QWidget *widget = 0);
	private:
		void							getModeThreshold( float& minNormal, float& maxNormal, float& minFrozen, float& maxFrozen );
		virtual QList<UIItemModel::Ptr>&getUIItemModelList()const{return m_variableModelList;}
		void							initUIItemModel();
		void							initIconList();
		void							paint(QPainter *painter, const QStyleOptionGraphicsItem *option/* =0 */, QWidget *widget /* = 0 */);
		static QList<UIItemModel::Ptr>  m_variableModelList;

		static ImageArray				m_publicImgArray[3];
		static ImageArray				m_privateImgArray[3];
		static ImageArray				m_protectedImgArray[3];
	};
}