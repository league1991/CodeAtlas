#include "StdAfx.h"
#include "CodeView.h"

using namespace CodeAtlas; 

QImage CodeAtlas::CodeView::m_cursorImg;

CodeView::CodeView( QWidget *parent /*= 0*/ ):
QGraphicsView(parent), m_receiveMsg(true), m_mutex(QMutex::Recursive),
m_alwaysSeeCursor(true)
{ 
	setScene(&UIManager::instance()->getScene());
	//setCacheMode(CacheBackground);
	setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	setResizeAnchor(QGraphicsView::AnchorUnderMouse);
	setMouseTracking(true);
	scale(1, 1);
	//setMinimumSize(400, 400);
	setWindowTitle(tr("Code View")); 
	//setCacheMode(CacheNone); 
	setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
}
void CodeView::wheelEvent(QWheelEvent *event)
{
	const QPointF p0scene = mapToScene(event->pos());

	qreal factor = std::pow(1.0008, event->delta());
	scale(factor, factor);

	const QPointF p1mouse = mapFromScene(p0scene);
	const QPointF move = p1mouse - event->pos(); // The move
	horizontalScrollBar()->setValue(move.x() + horizontalScrollBar()->value());
	verticalScrollBar()->setValue(move.y() + verticalScrollBar()->value());

	//computeLod();
}


CodeAtlas::CodeView::~CodeView( void )
{

}

bool CodeAtlas::CodeView::event( QEvent * event )
{
	bool res = true;
	m_mutex.lock();
	if (m_receiveMsg)
	{
		//qDebug("view event");
		res = QGraphicsView::event(event);
		//update();
	}
	m_mutex.unlock();
	return res;
}

void CodeAtlas::CodeView::lockView()
{
	m_mutex.lock();
	m_receiveMsg = false;
	m_mutex.unlock();
}

void CodeAtlas::CodeView::unlockView()
{
	m_mutex.lock();
	m_receiveMsg = true;
	m_mutex.unlock();
}

void CodeAtlas::CodeView::drawBackground( QPainter * painter, const QRectF & rect )
{
	QSize   viewSize   = m_curSize;
	QPointF topLeftPnt = this->mapToScene(0,0);
	QPointF bottomRightPnt = this->mapToScene(viewSize.width(), viewSize.height());
	QRectF  sceneRect(topLeftPnt, bottomRightPnt);
	QGraphicsScene* scene = this->scene();
	if (!scene)
		return;

	QList<QGraphicsItem*> itemList = scene->items(sceneRect, Qt::IntersectsItemBoundingRect);
	QList<QRect> rectList;
	QList<float> radiusList;
	QVector<float> valList;
	QVector<QRgb>  clrList;
	SymbolTreeBuilder& treeBuilder =DBManager::instance()->getDB().getSymbolCollector();
	SymbolTree& tree = treeBuilder.getSymbolTree();
	SmartDepthIterator it(tree.getRoot(), SmartDepthIterator::PREORDER, SymbolInfo::Project, SymbolInfo::Root | SymbolInfo::Project);
	
	for (SymbolNode::Ptr proj; proj = *it; ++it)
	{
		ProjectAttr::Ptr projAttr = proj->getAttr<ProjectAttr>();
		if (!projAttr)
			continue;
		
		const QList<SymbolNode::Ptr>& globalSymbols = projAttr->getGlobalSymList();
		foreach (SymbolNode::Ptr gloSym, globalSymbols)
		{
			UIElementAttr::Ptr uiAttr = gloSym->getAttr<UIElementAttr>();
			if (uiAttr.isNull())
				continue;
			NodeUIItem::Ptr uiItem = uiAttr->getUIItem();
			if (uiItem.isNull())
				continue;

			QRectF bRect = uiItem->boundingRect();
			QSizeF  bSize = bRect.size();
			QPoint topLeft = mapFromScene(uiItem->mapToScene(bRect.topLeft()));
			QPoint bottomRight = mapFromScene(uiItem->mapToScene(bRect.bottomRight()));
			QRect renderRect(topLeft, bottomRight);
			rectList.push_back(renderRect); 

			float ratio = renderRect.size().width() / bRect.size().width();
			radiusList.push_back(uiItem->getEntityRadius() * ratio);
			valList.push_back(uiItem->getLevel());
			clrList.push_back(UISetting::getTerritoryColor(qHash(uiItem->name())).rgb());
		}
	}

	
	float lod = QStyleOptionGraphicsItem::levelOfDetailFromTransform(painter->worldTransform());
	BackgroundRenderer& renderer = UISetting::getBackgroundRenderer();
	renderer.setLod(lod);
	renderer.setLodInterLimit(0.02,0.15);
	renderer.setEmptyTerritory(qRgba(233,229,220,255));
	renderer.setRenderSize(m_curSize.width(), m_curSize.height(), 0.25);
	if (lod > 0.03  && 0)
	{
		renderer.setRenderMode(BackgroundRenderer::RENDER_PLAIN_CLR);
		renderer.setIsShaded(true);
	}
	else
	{
		renderer.setRenderMode(BackgroundRenderer::RENDER_TERRAIN);
		renderer.setIsShaded(false);
	}
	renderer.setPixelSizeFactor(10);
	renderer.render(rectList, radiusList, valList, clrList);
	painter->drawImage(sceneRect, renderer.getFinalImage());

	computeLod();
}

void CodeAtlas::CodeView::computeLod()
{
	// set node's lod
	LodManager lodManager;
	QRectF viewRect = this->mapToScene(this->rect()).boundingRect();
	lodManager.setViewRect(viewRect);
	lodManager.setLod(this->transform());
	lodManager.setRoot(DBManager::instance()->getDB().getSymbolCollector().getSymbolTree().getRoot());
	lodManager.compute();
}


void CodeAtlas::CodeView::drawForeground( QPainter *painter, const QRectF &rect )
{	
	struct TxtItem
	{
		unsigned m_type;
		QString  m_txt;
		QRect    m_frameRect;
		LodStatus m_lodStatus;
	};

	QFont varFont("Tahoma", 7, QFont::Light);
	QFont funcFont("Tahoma", 7, QFont::Light);
	QFont classFont("Tahoma", 8, QFont::DemiBold);
	QFont projFont("Tahoma", 8, QFont::Bold);
	QFontMetrics varMetrics(varFont), funcMetrics(funcFont), classMetrics(classFont), projMetrics(projFont);

	vector<OverlapData> overlapVector;
	vector<TxtItem>     overlapNodes;
	QRect viewRect(QPoint(), m_curSize);

	SymbolTreeBuilder& treeBuilder =DBManager::instance()->getDB().getSymbolCollector();
	SymbolTree& tree = treeBuilder.getSymbolTree();
	SmartDepthIterator it(tree.getRoot(), SmartDepthIterator::PREORDER,SymbolInfo::All, SymbolInfo::All & ~SymbolInfo::Block);

	const QString nextLine(QChar::LineSeparator);
	const int lineLength = 15;
	for (SymbolNode::Ptr node; node = *it; ++it)
	{
		UIElementAttr::Ptr uiAttr = node->getAttr<UIElementAttr>();
		if (uiAttr.isNull())
			continue;
		
		NodeUIItem::Ptr uiItem = uiAttr->getUIItem();
		if (uiItem.isNull())
			continue;

		if (uiItem->getLodStatus() == LOD_INVISIBLE)
			continue;

		bool isExpanded = uiItem->getLodStatus() == LOD_EXPANDED;

		SymbolInfo info = node->getSymInfo();
		unsigned type = (unsigned)info.elementType();
		if (isExpanded && (type & (SymbolInfo::Class | SymbolInfo::Project)))
			continue;

		QString lastPart = TextProcessor::findLastPart(uiItem->name());
		QString dispName = TextProcessor::breakTextByLength(lastPart, lineLength, nextLine);

		QPointF pos;
		QSize   titleSize; 
		if (type & (SymbolInfo::Variable | SymbolInfo::Enum))
		{
			titleSize = varMetrics.size(0,dispName);
			pos.ry() += uiItem->getEntityRadius();
		}
		else if (type & SymbolInfo::FunctionSignalSlot)
		{
			titleSize = funcMetrics.size(0,dispName);
			pos.ry() += uiItem->getEntityRadius();
		}
		else if (type & SymbolInfo::Class)
		{
			titleSize = classMetrics.size(0,dispName);
		}
		else if (type & SymbolInfo::Project)
		{
			titleSize = projMetrics.size(0,dispName);
		}
		else 
			continue;

		QPoint  posi = mapFromScene(uiItem->mapToScene(pos));
		QRect   titleRect(QPoint(), titleSize);
		titleRect.moveCenter(posi);
		if (type & (SymbolInfo::Variable | SymbolInfo::Enum | SymbolInfo::FunctionSignalSlot))
			titleRect.translate(0, titleSize.height() / 2);

		if (!titleRect.intersects(viewRect))
			continue;

		const int padding = 8;
		QRect affectRect = UIUtility::expandRect(titleRect, padding);

		float radius = uiItem->getEntityRadius();
		float firstPriority = uiItem->isSelected() ? 3 : 2;
		float secondPriority = radius * radius * uiItem->getLevel() / (titleRect.width() * titleRect.height());
		if (type & (SymbolInfo::Variable))
		{
			secondPriority /= 10;
		}
		overlapVector.push_back(OverlapData(affectRect, Priority(firstPriority, secondPriority)));

		TxtItem txtItem;
		txtItem.m_txt = dispName;
		txtItem.m_type = type;
		txtItem.m_frameRect = titleRect;
		txtItem.m_lodStatus = uiItem->getLodStatus();
		overlapNodes.push_back(txtItem);
	}

	m_overlapSolver.clearViewData();
	m_overlapSolver.setOverlapData(overlapVector);
	m_overlapSolver.compute();
	
	const vector<OverlapData>& result = m_overlapSolver.getOverlapData();

	painter->setWorldTransform(QTransform());
	for (int i = 0; i < result.size(); ++i)
	{
		if (!result[i].m_isShown)
			continue;

		TxtItem& txtItem = overlapNodes[i];
		unsigned type = txtItem.m_type;
		const QRect& rect = txtItem.m_frameRect;
		
		if (0&& type & (SymbolInfo::Project | SymbolInfo::Class | SymbolInfo::Enum))
		{
			painter->setBrush(QBrush(QColor(255,255,255,150)));
			painter->setPen(Qt::NoPen);
			const int padding = 5;
			QRect roundedRect = rect.adjusted(-padding, 0, padding, 0);
			painter->drawRoundedRect(roundedRect, padding, padding, Qt::AbsoluteSize);
			//painter->drawRect(rect);
		}

		QFont* font;
		if (type & (SymbolInfo::Variable | SymbolInfo::Enum))
		{
			font = &varFont;
			painter->setPen(QPen(QColor(50,50,50)));
		}
		else if (type & SymbolInfo::FunctionSignalSlot)
		{
			font = &funcFont;
			painter->setPen(QPen(QColor(50,50,50)));
		}
		else if (type & SymbolInfo::Class)
		{
			font = &classFont;
			painter->setPen(QPen(QColor(0,0,0,150)));
		}
		else if (type & SymbolInfo::Project)
		{
			font = &projFont;
			painter->setPen(QPen(QColor(0,0,0)));
		}
		else
			continue;

		painter->setFont(*font);
		QString& brokenName = overlapNodes[i].m_txt;

		QRect& r = txtItem.m_frameRect;
		QPen oriPen = painter->pen();
		if (txtItem.m_lodStatus == LOD_HIGHLIGHTED)
			painter->setPen(QPen(QColor(255,207,81)));
		else
			painter->setPen(QPen(QColor(255,255,255)));

		const int o = 1;
		painter->drawText(r.translated(-o,-o), Qt::AlignCenter,brokenName);
		painter->drawText(r.translated(-o, o), Qt::AlignCenter,brokenName);
		painter->drawText(r.translated( o,-o), Qt::AlignCenter,brokenName);
		painter->drawText(r.translated( o, o), Qt::AlignCenter,brokenName);

		painter->setPen(oriPen);
		painter->drawText(result[i].m_rect, Qt::AlignCenter,brokenName);

	}

	// draw cursor

	drawCursorIcon(painter);
}

void CodeAtlas::CodeView::drawCursorIcon(QPainter* painter)
{
	SymbolNode::Ptr cursorNode =UIManager::instance()->getScene().getCurCursorNode();
	if (!cursorNode)
		return;

	UIElementAttr::Ptr uiAttr = cursorNode->getAttr<UIElementAttr>();
	if (!uiAttr)
		return;

	NodeUIItem::Ptr uiItem = uiAttr->getUIItem();
	if (!uiItem)
		return;
	QPointF pos = uiItem->mapToScene(QPointF());
	QPoint  posi = mapFromScene(pos);
	QRect   cursorRect(QPoint(), QSize(30,30));
	cursorRect.moveCenter(posi);
	QRect viewRect(QPoint(), m_curSize);

	QRect   outlineRect = UIUtility::scaleRect(cursorRect, 4);
	QPen cursorPen(QColor(47,91,140,200));
	cursorPen.setWidth(2);
	painter->setPen(cursorPen);
	QBrush cursorBrush(QColor(80,135,197,50));
	painter->setBrush(cursorBrush);
	painter->drawEllipse(outlineRect);

	//qDebug() << "draw icon";
	if (m_cursorImg.isNull())
		m_cursorImg.load(":/CodeAtlas/Resources/cursor.png");

	painter->drawImage(cursorRect, m_cursorImg);
	//painter->drawEllipse(cursorRect);
}
void CodeAtlas::CodeView::resizeEvent( QResizeEvent * event )
{
	QSize s = event->size();
	m_curSize = s;

	m_overlapSolver.setViewRect(this->rect());
	//computeLod();
}

void CodeAtlas::CodeView::paintEvent( QPaintEvent *event )
{
	bool res = true;
	m_mutex.lock();
	if (m_receiveMsg)
	{
		//qDebug("view paint event");
		QGraphicsView::paintEvent(event);
	}
	m_mutex.unlock();
}

void CodeAtlas::CodeView::centerViewWhenNecessary()
{
	if (!m_alwaysSeeCursor)
		return;
	SymbolNode::Ptr cursorNode =UIManager::instance()->getScene().getCurCursorNode();
	if (!cursorNode)
		return;

	UIElementAttr::Ptr uiAttr = cursorNode->getAttr<UIElementAttr>();
	if (!uiAttr)
		return;

	NodeUIItem::Ptr uiItem = uiAttr->getUIItem();
	if (!uiItem)
		return;
	QPointF pos = uiItem->mapToScene(QPointF());
	QPoint  posi = mapFromScene(pos);
	QRect viewRect(QPoint(), m_curSize);

	if (!viewRect.contains(posi))
	{
		centerOn(pos);
		return;
	}
}
