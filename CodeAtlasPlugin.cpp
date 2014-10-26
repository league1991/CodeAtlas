// Test1.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "codeAtlasPlugin.h"

using namespace CodeAtlas;


CodeAtlasPlugin::CodeAtlasPlugin() 
{ 
	// Do nothing
} 

CodeAtlasPlugin::~CodeAtlasPlugin() 
{ 
	// Do notning 
} 

bool CodeAtlasPlugin::initialize(const QStringList& args, QString *errMsg) 
{

	Q_UNUSED(args); 
	Q_UNUSED(errMsg); 

	new UIManager;
	new DBManager;

	// menu actions
	Core::ActionContainer* ac = Core::ActionManager::actionContainer(Core::Constants::M_HELP); 
	QAction* getInfoAction = ac->menu()->addAction("Show Code Atlas"); 
	connect(getInfoAction, SIGNAL(triggered()), this, SLOT(getClassInfo()));

	// communications between db and ui
	connect(DBManager::instance(), SIGNAL(dbUpdated()), UIManager::instance(), SLOT(updateScene()));
	return true; 
}

void CodeAtlasPlugin::extensionsInitialized() 
{
	// Do nothing 
}

void CodeAtlasPlugin::shutdown() 
{ 
	// Do nothing 
}

void CodeAtlasPlugin::getClassInfo()
{
	UIManager::instance()->getMainUI().show();
	return;

// 	ExtensionSystem::PluginManager* pm = ExtensionSystem::PluginManager::instance();
// 	CppTools::Internal::CppLocatorData*locD = (CppTools::Internal::CppLocatorData*) ExtensionSystem::PluginManager::getObjectByClassName("CppTools::Internal::CppLocatorData");
// 	
// 	//CppTools::Internal::CppLocatorData d(NULL);
// 	QList<CppTools::ModelItemInfo>f = locD->functions();
// 	QList<CppTools::ModelItemInfo>c = locD->classes();
// 	//const QList<CppTools::Internal::CppLocatorData*>objs = ExtensionSystem::PluginManager::getObjects<CppTools::Internal::CppLocatorData>();
// 	CppTools::Internal::CppModelManager* mgr = CppTools::Internal::CppModelManager::instance();
// 	//QList<CppTools::Internal::CppModelManager*> mgr = ExtensionSystem::PluginManager::getObjects<CppTools::Internal::CppModelManager>();
// 	CPlusPlus::Snapshot snapshot = mgr->snapshot();
// 	CPlusPlus::Document::Ptr doc =  
// 		snapshot.document("H:/Programs/codeVisualization/codeVis/codeVis/main.cpp");

	//DBManager::instance()->getDB().clearCallGraph();


// 	CodeAtlasDB* db = CodeAtlasDB::instance();
// 	db->reset(snapshot);
// 	db->update();
// 	if (!doc) 
// 		return; 

	
/*	qDebug("\n\n######################################################\n");
	for (int ithSymbol = 0; ithSymbol < doc->globalSymbolCount(); ++ithSymbol)
	{
		CPlusPlus::Symbol* globalSym = doc->globalSymbolAt(ithSymbol);
		if (globalSym->isClass())
		{
			qDebug()<< globalSym->name();
		}
		if (globalSym->isNamespace())
		{
			const CPlusPlus::Scope *globalScope = globalSym->asScope();
			if (globalScope)
			{
				CPlusPlus::Scope::iterator it = globalScope->firstMember();
				while (it != globalScope->lastMember()) {
					const CPlusPlus::Symbol *current_symbol = *it;
					++it;
				}
			}
			qDebug("namespace");
		}
		if (globalSym->isFunction())
		{
			const CPlusPlus::Scope *globalScope = globalSym->asScope();
			if (globalSym->name() && globalSym->name()->asQualifiedNameId())
			{
				qDebug("%s", globalSym->name()->asQualifiedNameId()->identifier()->chars());
			}
			if (globalScope)
			{
				
				for (int ithMember = 0; ithMember < globalScope->memberCount(); ++ithMember)
				{
					qDebug("\t scope");
					const CPlusPlus::Symbol *current_symbol = globalScope->memberAt(ithMember);

					if (current_symbol->isFunction())
					{
						if (current_symbol->name() && current_symbol->name()->asQualifiedNameId())
						{
							qDebug("\t\t%s", globalSym->name()->asQualifiedNameId()->identifier()->chars());
						}
					}
					if (current_symbol->isBlock())
					{
						qDebug("\tfunction block");
						for (int ithBlkMem = 0; ithBlkMem < current_symbol->asBlock()->memberCount();++ithBlkMem)
						{
							CPlusPlus::Symbol * blkSym = current_symbol->asBlock()->memberAt(ithBlkMem);

							if (blkSym->name() && 
								blkSym->name()->asNameId() &&
								blkSym->name()->asNameId()->identifier())
							{
								qDebug("\t\t%s", blkSym->name()->asNameId()->identifier()->chars());
							}
							else
							{
								qDebug("\t\t---");
							}
						}
					} 
					if (current_symbol->isUsingDeclaration())
					{
						qDebug("\t using decl.");
					}
					if (current_symbol->isArgument())
					{
						qDebug("\t decl");
					}
				}
			} 
		}
	}
	qDebug("___________________________________________\n");
	CPlusPlus::Control *ctrl = doc->control();
	for (CPlusPlus::Control::IdentifierIterator it = ctrl->firstIdentifier();
		it != ctrl->lastIdentifier()&&0; ++it)
	{
		if (*it )
		{
			qDebug("\t\t%s", (*it)->chars());
		}
	}
	qDebug("___________________________________________\n");
	for (CPlusPlus::Control::StringLiteralIterator it = ctrl->firstStringLiteral();
		it != ctrl->lastStringLiteral()&&0; ++it)
	{
		if (*it )
		{
			qDebug("\t\t%s", (*it)->chars());
		}
	}
	qDebug("___________________________________________\n");
	for (CPlusPlus::Control::NumericLiteralIterator it = ctrl->firstNumericLiteral();
		it != ctrl->lastNumericLiteral()&&0; ++it)
	{
		if (*it )
		{
			qDebug("\t\t%s", (*it)->chars());
		}
	}
	qDebug("___________________________________________\n");
	for (CPlusPlus::Symbol **it = ctrl->firstSymbol();
		it != ctrl->lastSymbol()&&0; ++it)
	{
		if ((*it)->name() && 
			(*it)->name()->asNameId() &&
			(*it)->name()->asNameId()->identifier())
		{
			qDebug("\t\t%s", (*it)->name()->asNameId()->identifier()->chars());
		}
	}
	qDebug("___________________________________________\n\n\n");*/

	//doc->check();
	//doc->parse();
 
//	const CppTools::CppModelManagerInterface::WorkingCopy workingCopy = mgr->workingCopy();
// 	QString fileName = doc->fileName();
// 	const QByteArray unpreprocessedSource = getSource(fileName, workingCopy);
// 
// 	doc = snapshot.preprocessedDocument(unpreprocessedSource, fileName);
// //	doc->tokenize();
// 	doc->check(); 
	//doc->parse();
 
// 	CPlusPlus::AST* ast = doc->translationUnit()->ast();
// 	CPlusPlus::AST* ast2 = doc->control()->translationUnit()->ast();
// 
// 	CPlusPlus::TranslationUnitAST* trAst = ast->asTranslationUnit();
	
	//MyVisitor visitor(doc);
	//visitor.accept(ast);
//	for (DeclarationListAST *it = ast->declaration_list; it; it = it->next) {
//		this->declaration(it->value);
//	}
// 
// 	CallGraphAnalyzer refVisitor(doc, ParserTreeItem::Ptr());
// 	refVisitor(NULL);
}

Q_EXPORT_PLUGIN(CodeAtlasPlugin) 
