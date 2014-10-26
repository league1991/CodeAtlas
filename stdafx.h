// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"
#include <time.h>

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头中排除极少使用的资料
// Windows 头文件:

//#define USING_QT_4
#define USING_QT_5


// TODO: 在此处引用程序需要的其他头文件
#include <coreplugin/coreconstants.h> 
#include <coreplugin/actionmanager/actionmanager.h> 
#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <cpptools/cpplocatordata.h>
#include <cpptools/searchsymbols.h>
#include <cpptools/symbolfinder.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <cpptools/typehierarchybuilder.h>

#include <cplusplus/ASTVisitor.h>
#include <cplusplus/FindUsages.h>
#include <cplusplus/Symbol.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/Scope.h>
#include <cplusplus/Name.h>
#include <cplusplus/Overview.h>
#include <cplusplus/OverviewModel.h>
#include <cplusplus/Icons.h>
#include <cplusplus/ExpressionUnderCursor.h>

#include <cppeditor/cppvirtualfunctionassistprovider.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>

#ifdef USING_QT_4
#include <qt4projectmanager/qt4nodes.h>
#endif
#ifdef USING_QT_5
#include <qmakeprojectmanager/qmakenodes.h>
#endif

#include <utils/qtcassert.h>

#include <vector>
#include <queue>

#include <qmath.h>
#include <QtPlugin> 
#include <QStringList> 
#include <QMenu>
#include <QList>
#include <QQueue>
#include <QStandardItem>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QThread>
#include <QVector2D>
#include <QVector3D>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsBlurEffect>
#include <QStyleOptionGraphicsItem>
#include <QScrollBar>
#include <QStaticText>

#include <Eigen/Dense>
#include <Eigen/LU>
#include <Eigen/SVD>
#include <Eigen/SparseCore>


#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/planarity/PlanarizationLayout.h>
#include <ogdf/planarity/VariableEmbeddingInserter.h>
#include <ogdf/planarity/FastPlanarSubgraph.h>
#include <ogdf/orthogonal/OrthoLayout.h>
#include <ogdf/planarity/EmbedderMinDepthMaxFaceLayers.h>
#include <ogdf/cluster/ClusterPlanarizationLayout.h>
#include <ogdf/cluster/ClusterOrthoLayout.h>
#include <ogdf/energybased/FMMMLayout.h>
#include <ogdf/energybased/FastMultipoleEmbedder.h>
#include <ogdf/misclayout/CircularLayout.h>

#include "globalSetting.h"

#include "GlobalUtility.h"
#include "svd/SVDSolver.h"
#include "mds/StressMDSLib.h"
#include "mds/MDSSolver.h"
#include "LaplacianSolver.h"
#include "pcasolver.h"
#include "GraphUtility.h"
#include "GeometryUtility.h"
#include "BSpline.h"


#include "SymbolUtility.h"
#include "SymbolPath.h"
#include "SymbolEdgeAttr.h"
#include "SymbolEdge.h"
#include "WordExtractor.h"
#include "SymbolNodeAttr.h"
#include "SymbolNode.h"
#include "symEdgeIter.h"
#include "symnodeiter.h"
#include "SymbolModifier.h"
#include "Delaunay/DelaunayRoute.h"
#include "Layouter.h"
#include "ComponentLayouter.h"
#include "ClassLayouter.h"
#include "UIElementLocator.h"
#include "symboltreebuilder.h"
#include "callgraphanalyzer.h"
#include "CodeAtlasDB.h"
#include "dbmanager.h"
#include "UIRenderer.h"
#include "UIItemModel.h"
#include "imagearray.h"
#include "NodeUIItem.h"
#include "EdgeUIItem.h"
#include "CursorUIItem.h"
#include "ProjectUIItem.h"
#include "ClassUIItem.h"
#include "VariableUIItem.h"
#include "FunctionUIItem.h"
#include "MathUtility.h"
#include "UIUtility.h"
#include "LodManager.h"
#include "backgroundRenderer.h"
#include "TextProcessor.h"
#include "OverlapSolver.h"
#include "CodeScene.h"
#include "CodeView.h"
#include "MainUI.h"
#include "UIManager.h"
#include "CodeAtlasPlugin.h"
