#pragma once
namespace CodeAtlas
{
class NodeUIItem;
class ProjectUIItem: public NodeUIItem
{
public:
	ProjectUIItem(const SymbolNode::WeakPtr& node,  UIItem* parent = 0);

	void			paint(QPainter *painter, const QStyleOptionGraphicsItem *option=0, QWidget *widget = 0);
private:
	void							getModeThreshold( float& minNormal, float& maxNormal, float& minFrozen, float& maxFrozen );
	virtual QList<UIItemModel::Ptr>&getUIItemModelList()const;
	void							initUIItemModel();
	static QList<UIItemModel::Ptr>  m_projModelList;
};
}