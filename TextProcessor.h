#pragma once

namespace CodeAtlas
{
	class TextProcessor
	{
	public:
		TextProcessor();

		void				setFont(const QFont& font);

		static QString		findLastPart(const QString& str, const QString& del = "::");
		static QString		breakTextByLength(const QString& str, const int maxLineCount, const QString& nextLine = "<br>");
		static QStringList	breakTextByLength(const QString& str, const int maxLineCount);
		void				breakTextByWidth( const QString& str, const int maxWidth, QString& displayStr, QRect& displayRect );
		QString				breakWords(const QString& str);
	private:

		QFontMetrics m_fontMetric;
		QRegExp      m_wordSplitter;
	};
}