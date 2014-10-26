
namespace CodeAtlas{

	class BackgroundRenderer;
	class UISetting
	{
	public:
		UISetting(){}
		~UISetting(){}

		static const QColor& getTerritoryColor(unsigned seed = 0);
		inline static const QColor& getOceanColor(){return s_oceanColor;}

		static QColor  s_varBackgroundClr;		// background color of variable
		static QColor  s_varCountourClr;		// countour color
		static QColor  s_funcBackgroundClr;		// background color of function
		static QColor  s_funcCountourClr;		// countour color
		static QPen	   s_publicMemPen;			// used to paint the countour of public members 
		static QPen    s_protectedMemPen;		// protected members
		static QPen    s_privateMemPen;			// private members

		static QFont   s_funcVarFont;			// font used to mark func and variable's name

		static QSizeF  getTextSize(const QFont& font, const QString& txt)
		{
			QFontMetricsF metrics(font);
			QRectF rect = metrics.boundingRect(txt);
			return rect.size();
		}

		static BackgroundRenderer& getBackgroundRenderer();
		static QImage& getAltitudeMap();
	protected:
	private:
		static QImage  s_heightMap;
		static QColor  s_territoryColor[];
		static QColor  s_oceanColor;
		static BackgroundRenderer*	m_renderer;
	};



}