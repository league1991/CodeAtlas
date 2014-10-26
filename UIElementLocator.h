#pragma once

#include "SymbolModifier.h"

namespace CodeAtlas
{
	class SymbolTree;

	class UIElementLocator:public SymbolModifier
	{
		//#define ALWAYS_TRIVAL_LAYOUT					1
	public:
		UIElementLocator():SymbolModifier(){}
		void			clearCache(){}
		void			modifyTree();
		void			updateDirtyData(){}
	private:
		typedef Eigen::Triplet<double>		Triplet;
		typedef Eigen::SparseMatrix<double> SparseMatrix;
		typedef Eigen::MatrixXf				MatrixXf;
		typedef Eigen::VectorXd				VectorXd;

		// recursive function
		// set all children's 2D position and this node's size
		bool			layoutTree(SymbolNode::Ptr& node);

		bool			layoutProjects(const SymbolNode::Ptr& root, float& radius);

		bool			buildProjVEMat(	const SymbolNode::Ptr&       rootNode,
			Eigen::SparseMatrix<double>& vtxEdgeMat);
		void			updateDirtyProjUIItem();
		void			updateUIItemTree( SymbolNode::Ptr& globalSym, SymbolNode::Ptr& proj);
		void			getOrAddUIAttr(SymbolNode::Ptr& node, float radius = 1.f);

		void			writeResultToNode( QList<SymbolNode::Ptr>& nodeList, MatrixXf& pos2D, VectorXf& radiusVec, VectorXi* levelVec = NULL);

	};



}
