#pragma once

class SymbolUtility
{
public:
	static bool findSymbolUnderCursor(CPlusPlus::Symbol*& finalSymbol, CPlusPlus::Document::Ptr& finalDoc);
	
private:
	// QModelIndex is an object used to extract Classes or member var/funs under cursor 
	typedef CPlusPlus::OverviewModel OverviewModel;
	static QModelIndex indexForPosition(
		OverviewModel *overviewModel,
		int line, int column,
		const QModelIndex &rootIndex = QModelIndex());

	static OverviewModel* getOverviewModel();

	// used to extract symbols under cursor
	static OverviewModel* s_overviewModel;
};

