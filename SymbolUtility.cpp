#include "stdafx.h"
#include "SymbolUtility.h"

using namespace CodeAtlas;
OverviewModel* SymbolUtility::s_overviewModel = NULL;


bool SymbolUtility::findSymbolUnderCursor(CPlusPlus::Symbol*& finalSymbol, CPlusPlus::Document::Ptr& finalDoc)
{
	typedef TextEditor::BaseTextEditor TxtEditor;
	typedef TextEditor::BaseTextEditorWidget TxtEditorWidget;

	TxtEditor* txtEditor = dynamic_cast<TxtEditor*>(Core::EditorManager::currentEditor());
	if (!txtEditor)
		return false;

	TxtEditorWidget* editorWidget = txtEditor->editorWidget();
	if (!editorWidget)
		return false;

	QTextCursor tc;
	tc = editorWidget->textCursor();
	int line = 0;
	int column = 0;
	const int pos = tc.position();
	editorWidget->convertPosition(pos, &line, &column);

	const CPlusPlus::Snapshot &snapshot = CppTools::CppModelManagerInterface::instance()->snapshot();
	CPlusPlus::Document::Ptr doc = snapshot.document(txtEditor->document()->filePath());
	if (!doc)
		return false; 

	finalDoc = doc;
	CPlusPlus::OverviewModel* overviewModel = getOverviewModel();
	overviewModel->rebuild(doc);
	QModelIndex idx = indexForPosition(overviewModel, line, column);
	finalSymbol = overviewModel->symbolFromIndex(idx);

	if (finalSymbol)
	{
		const CPlusPlus::Overview& overview = SymbolInfo::getNamePrinter();
		QString name = overview.prettyName(finalSymbol->name()).trimmed();
		QString type = overview.prettyType(finalSymbol->type()).trimmed();
	}
	return finalSymbol != NULL;
}

QModelIndex SymbolUtility::indexForPosition( CPlusPlus::OverviewModel *overviewModel, int line, int column, const QModelIndex &rootIndex )
{
	QModelIndex lastIndex = rootIndex;

	const int rowCount = overviewModel->rowCount(rootIndex);
	for (int row = 0; row < rowCount; ++row) {
		const QModelIndex index = overviewModel->index(row, 0, rootIndex);
		Symbol *symbol = overviewModel->symbolFromIndex(index);
		if (symbol && symbol->line() > unsigned(line))
			break;
		lastIndex = index;
	}

	if (lastIndex != rootIndex) {
		// recurse
		lastIndex = indexForPosition(overviewModel, line, column, lastIndex);
	}

	return lastIndex;
}

OverviewModel* SymbolUtility::getOverviewModel()
{
	if (s_overviewModel == NULL)
	{
		s_overviewModel = new OverviewModel();
	}
	return s_overviewModel;
}


