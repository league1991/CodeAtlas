#include "stdafx.h"


using namespace CodeAtlas;

void CodeAtlas::TextProcessor::setFont( const QFont& font )
{
	m_fontMetric = QFontMetrics(font);
}

void CodeAtlas::TextProcessor::breakTextByWidth( const QString& str, const int maxWidth, QString& displayStr, QRect& displayRect )
{
	const QString& src = str;
	QStringList wordList;
	int startPos = 0;
	for (int i = 0; i < src.length(); ++i)
	{
		if (src.at(i).isUpper())
		{
			int l = i - startPos;
			if (l > 0)
			{
				wordList << src.mid(startPos, l);
				startPos = i;
			}
		}
	}
	wordList << src.mid(startPos);

	int lineWidth = 0;
	QString& res = displayStr;
	res = "";
	for (int ithPart = 0; ithPart < wordList.size(); ++ithPart)
	{
		const QString& s = wordList[ithPart];
		int w = m_fontMetric.width(s);
		if (w < maxWidth && lineWidth + w > maxWidth)
		{
			res += "\n" + s;
		}
		else
			res += s;
	}

	displayRect = m_fontMetric.boundingRect(res);
}

QString CodeAtlas::TextProcessor::breakTextByLength(const QString& str, const int maxLineCount, const QString& nextLine)
{
	QString displayStr = "";
	int totalLength = str.length();
	int linePos  = 0;
	int wordPos  = 0;

	bool isLastULine = false;
	bool isLastUpper = false;
	for (int i = 0; i < totalLength; ++i)
	{
		QChar c = str.at(i);

		bool isThisUpper = c.isUpper();
		if ((!isLastUpper && isThisUpper) || isLastULine)
		{
			int lineLength = i - linePos;
			int wordLength = i - wordPos;
			if(lineLength > maxLineCount)
			{
				int l = wordPos - linePos;
				if (l == 0)
					l = i - linePos;			// no other words before current word in this line
				displayStr += str.mid(linePos, l) + nextLine;
				linePos += l;
			}
			wordPos = i;
		}
		isLastULine = (c == QChar('_'));
		isLastUpper = isThisUpper;
	}
	int l = wordPos - linePos;
	if (l == 0 || totalLength - linePos < maxLineCount)
		displayStr += str.mid(linePos);
	else
		displayStr += str.mid(linePos, l) + nextLine + str.mid(wordPos);

	return displayStr;
}

QStringList CodeAtlas::TextProcessor::breakTextByLength( const QString& str, const int maxLineCount )
{
	QStringList displayStr;
	int totalLength = str.length();
	int linePos  = 0;
	int wordPos  = 0;

	bool isLastULine = false;
	bool isLastUpper = false;
	for (int i = 0; i < totalLength; ++i)
	{
		QChar c = str.at(i);

		bool isThisUpper = c.isUpper();
		if ((!isLastUpper && isThisUpper) || isLastULine)
		{
			int lineLength = i - linePos;
			int wordLength = i - wordPos;
			if(lineLength > maxLineCount)
			{
				int l = wordPos - linePos;
				if (l == 0)
					l = i - linePos;			// no other words before current word in this line
				displayStr += str.mid(linePos, l);
				linePos += l;
			}
			wordPos = i;
		}
		isLastULine = (c == QChar('_'));
		isLastUpper = isThisUpper;
	}
	int l = wordPos - linePos;
	if (l == 0 || totalLength - linePos < maxLineCount)
		displayStr += str.mid(linePos);
	else
	{
		displayStr += str.mid(linePos, l);
		displayStr += str.mid(wordPos);
	}

	return displayStr;
}

QString CodeAtlas::TextProcessor::breakWords(const QString& str)
{
	QString displayStr = "";
	int wordPos = 0;
	for (int i = 0; i < str.length(); ++i)
	{
		if (str.at(i).isUpper())
		{
			int l = i - wordPos;
			if (l > 0)
			{
				displayStr += str.mid(wordPos, l) + " ";
				wordPos = i;
			}
		}
		else if (str.at(i) == QChar('_'))
		{
			int l = i - wordPos;
			if (l > 0)
			{
				displayStr += str.mid(wordPos, l) + " ";
				wordPos = i+1;
			}
		}
	}
	if (str.length() - wordPos > 0)
	{
		displayStr += str.mid(wordPos);
	}
	return displayStr;
}
QString CodeAtlas::TextProcessor::findLastPart( const QString& str, const QString& del )
{
	int start = str.lastIndexOf(del);
	if (start != -1)
	{
		return str.mid(start+del.length());
	}
	return str;
}

CodeAtlas::TextProcessor::TextProcessor() :m_fontMetric(QFont("Tahoma", 7, QFont::Light)), m_wordSplitter("[A-Za-z][a-z]*")
{

}
