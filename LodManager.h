#pragma once

namespace CodeAtlas
{
	enum  LodStatus
	{
		LOD_INVALID     = 0,
		LOD_INVISIBLE   = 0x1,
		LOD_FOLDED		= (0x1 << 1),
		LOD_EXPANDED    = (0x1 << 2),
		LOD_HIGHLIGHTED = (0x1 << 3)
	};
	class LodSetting
	{
	public:
		virtual LodStatus computeLod(UIItem::Ptr& item, float lod) = 0;
		bool setLodStatus(const SymbolNode::Ptr& node, LodStatus lodStatus);
		static UIItem::Ptr getUIItem(const SymbolNode::Ptr& node);
	};
	class AbsoluteLodSetting:public LodSetting
	{
	public:
		AbsoluteLodSetting(float foldedLod = 0, float expandedLod = 0):m_foldedLod(foldedLod), m_expandedLod(expandedLod){}
		virtual LodStatus computeLod(UIItem::Ptr& item, float lod);
		void   setLodThreshold(float foldedLod, float expandedLod);
	private:
		float m_foldedLod, m_expandedLod;
	};

	class LodManager
	{
	public:
		LodManager():m_lod(1.f),m_maxProjRadius(1), m_minProjRadius(1), m_avgProjRadius(1), m_maxClassRadius(1){}
		~LodManager(){}

		void setRoot(SymbolNode::Ptr root){m_root = root;}
		void setLod(float lod){m_lod = lod;}
		void setLod(const QTransform & worldTransform)
		{m_lod = QStyleOptionGraphicsItem::levelOfDetailFromTransform(worldTransform);}
		void setViewRect(const QRectF& viewRect){m_viewRect = viewRect;}
		virtual void compute();

	protected:
		void computeChildLod( SymbolNode::Ptr& parent, LodStatus parentStatus, float parentProjD = 0, float parentClassD = 0, int depth = 0);
		void setAllEdgeExpanded();
		void countProjRadius();

		inline float getViewSize(float radius)
		{
			return m_lod * radius;
		}
		void computeEdgeLod(SymbolNode::Ptr& parent);
		float			m_lod;
		SymbolNode::Ptr m_root;

		AbsoluteLodSetting m_absLod;

		float			m_maxProjRadius;
		float			m_minProjRadius;
		float			m_avgProjRadius;
		float			m_maxClassRadius;
		QRectF			m_viewRect;
	};

}