#pragma once

namespace CodeAtlas
{

class CodeView :public QGraphicsView
{
	Q_OBJECT
public:
	CodeView(QWidget *parent = 0);
	~CodeView(void);

	// ignore message when the scene is locked
	void lockView();
	void unlockView();

	void centerViewWhenNecessary();
signals:
protected:
	virtual void    paintEvent(QPaintEvent *event);
	virtual void	drawBackground ( QPainter * painter, const QRectF & rect );
	virtual void	drawForeground(QPainter *painter, const QRectF &rect);
	void			drawCursorIcon(QPainter* painter);

	virtual void	resizeEvent ( QResizeEvent * event );
	virtual bool	event ( QEvent * event );
	void			wheelEvent(QWheelEvent *event);
	void			computeLod();

	QSize				m_curSize;
	QPointF				m_curTranslate;
	QMutex				m_mutex;
	bool				m_receiveMsg;
	bool				m_alwaysSeeCursor;

	OverlapSolver		m_overlapSolver;

	static QImage		m_cursorImg;
};

}
