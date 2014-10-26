#include "StdAfx.h"
#include "WordExtractor.h"

using namespace CodeAtlas;

QSet<QString> WordExtractor::m_uselessWords;
QRegExp       WordExtractor::m_wordSplitter("[A-Za-z][a-z]*");

WordExtractor::WordExtractor()
{
}

QStringList WordExtractor::normalizeWords( const QString& str )
{
// 	QStringList wordList;
// 	int pos = 0;
// 	while ((pos = m_wordSplitter.indexIn(str, pos)) != -1) {
// 		pos += m_wordSplitter.matchedLength();
// 
// 		QString newWord = m_wordSplitter.cap(0).toLower();
// 		if (!getUselessWords().contains(newWord) && newWord.length() > 1)
// 		{
// 			wordList << newWord;
// 		}
// 	}

//	qDebug() << "input str: " << str;

	int beg = 0;
	int strLength = str.size();
	const QChar* pStr = str.constData();
	const QChar  a('a'), z('z'), A('A'), Z('Z');
	QStringList wList;
	while(1)
	{
		// find word begin
		for (; beg < strLength; ++beg)
		{
			const QChar* c = pStr +beg;
			if ((*c >= a && *c <= z) || (*c >= A && *c <= Z))
				break;
		}

		if (beg >= strLength)
			break;

		// find word end
		int end = beg+1;
		for (; end < strLength; ++end)
		{
			const QChar* c = pStr + end;
			if (!(*c >= a && *c <= z))
				break;
		}

		// add to list
		int length = end - beg;
		if (length >= 3)
		{
			QString word(pStr+beg, length);
			if (!getUselessWords().contains(word))
			{
				wList << word.toLower();
				//qDebug() << word;
			}
		}

		beg = end;
	}

	return wList;
}

QSet<QString>& CodeAtlas::WordExtractor::getUselessWords()
{
	if (m_uselessWords.size() == 0)
	{
		// skip all cpp keywords
		m_uselessWords
			// c++ key words
			<<"asm"		<<"do"		<<"if"		<<"return"	<<"typedef"
			<<"auto"	<<"double"	<<"inline"	<<"short"	<<"typeid"
			<<"bool"	<<"dynamic_cast"		<<"int"		<<"signed"		<<"typename"
			<<"break"	<<"else"	<<"long"	<<"sizeof"	<<"union"
			<<"case"	<<"enum"	<<"mutable"	<<"static"	<<"unsigned"
			<<"catch"	<<"explicit"<<"namespace"			<<"static_cast"	<<"using"
			<<"char"	<<"export"	<<"new"		<<"struct"	<<"virtual"
			<<"class"	<<"extern"	<<"operator"<<"switch"	<<"void"
			<<"const"	<<"false"	<<"private"	<<"template"<<"volatile"
			<<"cast"	<<"float"	<<"protected"			<<"this"	
			<<"continue"			<<"for"		<<"public"	<<"throw"		<<"while"
			<<"default"	<<"friend"	<<"register"<<"true"	<<"wchar_t"
			<<"delete"	<<"goto"	<<"reinterpret"	<<"try"

			// addtional words
			<<"string";
	}
	return m_uselessWords;
}
void WordExtractor::findOccurrence( const QString& wordStr, const QStringList& textSet, QVector<int>& occurrence )
{
	occurrence.resize(textSet.size());
	occurrence.fill(0);
	if (!wordStr.size())
		return;

	int wordLength = wordStr.size();
	QStringMatcher strMatcher(wordStr);
	const QChar underLine('_');
	for (int ithText = 0; ithText < textSet.size(); ++ithText)
	{
		int pos = 0;
		const QString& text = textSet[ithText];
		for(;(pos = strMatcher.indexIn(text, pos)) != -1; pos += wordLength)
		{
			// try match whold word
			if (pos > 0)
			{
				QChar preChar = text.at(pos-1);
				if (preChar.isLetter() || preChar.isNumber() || preChar == underLine)
					continue;
			}
			int end = pos + wordLength;
			if (end < text.size())
			{
				QChar postChar = text.at(end);
				if (postChar.isLetter() || postChar.isNumber() || postChar == underLine)
					continue;
			}
			occurrence[ithText]++;
		}
	}
}
void CodeAtlas::WordExtractor::findOccurrence( const QString& textStr, const QStringList& wordSet, QHash<QString, int>& occurrence )
{
	occurrence.clear();
	if (wordSet.size() == 0 || textStr.size() == 0)
		return;

	// construct match string, for example: \b(mail|letter|correspondence)\b
	const QStringList::ConstIterator pIter = wordSet.constBegin();
	QString matchStr;
	for (int ithStr = 0; ithStr < wordSet.size(); ++ithStr)
		if (wordSet[ithStr].trimmed().size() != 0)
			matchStr += ("|" + wordSet[ithStr]);
	matchStr.replace(0, 1, "\\b(");
	matchStr += ")\\b";

	// match 
	QRegExp reg(matchStr);
	int pos = 0;
	while ((pos = reg.indexIn(textStr, pos)) != -1) {
		pos += reg.matchedLength();

		QString newWord = reg.cap(0);
		if (!occurrence.contains(newWord))
			occurrence[newWord] = 1;
		else
			occurrence[newWord]++;
	}
}
