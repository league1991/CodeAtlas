#include "stdafx.h"
using namespace CodeAtlas;

QColor UISetting::s_territoryColor[] = 
{
	QColor(211,208,116),
	QColor(243,180,95),
	QColor(237,137,79),
	QColor(216,206,192),
	QColor(217,148,159),

	QColor(178,202,155),
	QColor(252,218,173),
	QColor(223,231,223),
	QColor(174,229,167),
	QColor(183,198,201),

	QColor(213,200,218),
	QColor(238,165,149),
	QColor(240,222,120),
	QColor(191,178,129),
	QColor(224,185,132)
};

QColor UISetting::s_oceanColor(123,152,217);


const QColor& UISetting::getTerritoryColor(unsigned seed)
{
	int id = seed % (sizeof(s_territoryColor) / sizeof(QColor));
	return s_territoryColor[id];
}


QColor CodeAtlas::UISetting::s_varBackgroundClr(191,234,119);
QColor CodeAtlas::UISetting::s_varCountourClr(125,184,29);

QPen CodeAtlas::UISetting::s_publicMemPen( QBrush(), 0, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin );
QPen CodeAtlas::UISetting::s_protectedMemPen( QBrush(), 4, Qt::DotLine, Qt::SquareCap, Qt::BevelJoin );
QPen CodeAtlas::UISetting::s_privateMemPen( QBrush(), 4, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin );



QColor CodeAtlas::UISetting::s_funcBackgroundClr(216,175,226);
QColor CodeAtlas::UISetting::s_funcCountourClr(132,55,151);

QFont CodeAtlas::UISetting::s_funcVarFont("Arial", 20);

QImage CodeAtlas::UISetting::s_heightMap;


BackgroundRenderer& CodeAtlas::UISetting::getBackgroundRenderer()
{
	if (m_renderer == NULL)
	{
		m_renderer = new BackgroundRenderer;
	}
	return *m_renderer;
}

QImage& CodeAtlas::UISetting::getAltitudeMap()
{
	if (s_heightMap.isNull())
	{
		s_heightMap = QImage(":/CodeAtlas/Resources/altitude.png");
	}
	return s_heightMap;
}

BackgroundRenderer* CodeAtlas::UISetting::m_renderer = NULL;
