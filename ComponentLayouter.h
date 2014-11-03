#pragma once
//#define ONLY_USE_WORD_SIMILARITY
//#define CHOOSE_IMPORTANT_EDGES

#define OGDF_GRAPH_LAYOUT
namespace CodeAtlas
{

class ComponentLayouter:public Layouter
{
public:
	void			setNodes(const QList<SymbolNode::Ptr>& nodeList, const SymbolNode::Ptr& parent = SymbolNode::Ptr());

	// compute and set children's 2D position
	bool			compute();

private:
	// struct used to store connected component information
	struct Component
	{
		Component():m_radius(0.f), m_compHash(0){}
		// used by graph layout
		SparseMatrix*m_pVEIncidenceMat;
		VectorXf	m_vtxRadius;
		VectorXf    m_edgeWeight;
		MatrixXf	m_localPos2D;
		VectorXi	m_hashID;					// id to identify different elements
		float       m_radius;
		QList<MatrixXf> m_edgePath;

		// used by word layout
		unsigned    m_compHash;
		SymbolWordAttr m_wordAttr;
		VectorXf    m_compPos2D;
	};
	struct NodeKey
	{
		SymbolInfo  m_nodeInfo;
		float		m_r;
		bool		operator<(const NodeKey& k)const
		{
			int cmp = m_nodeInfo.compare(k.m_nodeInfo);
			if (cmp != 0)
				return cmp < 0;
			return m_r < k.m_r;
		}
	};

	bool				compute(	const SparseMatrix* vtxEdgeMatrix, 
									const VectorXd* edgeWeight, 
									const QList<SymbolNode::Ptr>& childList, 
									MatrixXf& childPos, 
									VectorXf& childRadius, 
									float& totalRadius);

	// low-level layout algorithm
	// first place child nodes in each connected component
	// then place components
	// for each child, the final position is given by adding component position and its local position in component
	// compute child's local 2D position in each component
	static void			layoutByGraph( QVector<Component>& compInfo );
	// compute each component's 2D position
	static void			layoutByWord(QVector<Component>& compInfo, float& finalRadius);

	static void			orthoGraphLayout(
		const SparseMatrix& veIncidenceMat, 
		const VectorXf& rVec, 
		MatrixXf& finalPos2D, 
		float& finalRadius , 
		QList<MatrixXf>& edgeList);

	TrivalLayouter m_trivalLayouter;
};

}
