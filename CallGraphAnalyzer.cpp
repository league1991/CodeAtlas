#include "StdAfx.h"
#include "callgraphanalyzer.h"


/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "stdafx.h"

using namespace CPlusPlus;
using namespace CppTools;
using namespace CodeAtlas;
Symbol *findDefinition(Symbol *symbol, const Snapshot &snapshot, SymbolFinder *symbolFinder)
{
	if (symbol->isFunction())
		return 0; // symbol is a function definition.

	else if (!symbol->type()->isFunctionType())
		return 0; // not a function declaration

	return symbolFinder->findMatchingDefinition(symbol, snapshot);
}
LookupItem skipForwardDeclarations(const QList<LookupItem> &resolvedSymbols)
{
	QList<LookupItem> candidates = resolvedSymbols;

	LookupItem result = candidates.first();
	const FullySpecifiedType ty = result.type().simplified();

	if (ty->isForwardClassDeclarationType()) {
		while (!candidates.isEmpty()) {
			LookupItem r = candidates.takeFirst();

			if (!r.type()->isForwardClassDeclarationType()) {
				result = r;
				break;
			}
		}
	}

	if (ty->isObjCForwardClassDeclarationType()) {
		while (!candidates.isEmpty()) {
			LookupItem r = candidates.takeFirst();

			if (!r.type()->isObjCForwardClassDeclarationType()) {
				result = r;
				break;
			}
		}
	}

	if (ty->isObjCForwardProtocolDeclarationType()) {
		while (!candidates.isEmpty()) {
			LookupItem r = candidates.takeFirst();

			if (!r.type()->isObjCForwardProtocolDeclarationType()) {
				result = r;
				break;
			}
		}
	}

	return result;
}

CallGraphAnalyzer::CallGraphAnalyzer(Document::Ptr doc, 
									 SymbolTree elementTree)
: ASTVisitor(doc->translationUnit()),
//	_id(0),
//	_declSymbol(0),
//	_doc(doc),
//	_snapshot(snapshot),
//	_context(doc, snapshot),
//	_originalSource(originalSource),
//	_source(_doc->utf8Source()),
	_currentScope(0),
	m_elementTree(elementTree.getRoot())
{
#ifdef USING_QT_4
	CppTools::Internal::CppModelManager* mgr = CppTools::Internal::CppModelManager::instance();
#elif defined(USING_QT_5)
	CppTools::CppModelManagerInterface *mgr = CppTools::CppModelManagerInterface::instance();
	if (!mgr)
		return;
#endif
	const CppTools::CppModelManagerInterface::WorkingCopy workingCopy = mgr->workingCopy();

	_snapshot = mgr->snapshot();

	QString fileName = doc->fileName();
	_originalSource = getSource(fileName, workingCopy);
	_doc = _snapshot.preprocessedDocument(_originalSource, fileName);
	_doc->check();
	_snapshot.insert(_doc);
	_source = _doc->utf8Source();
	setTranslationUnit(_doc->translationUnit());

	_context = LookupContext(_doc, _snapshot);
	typeofExpression.init(_doc, _snapshot, _context.bindings());

	prepareLines(_originalSource);
}
/*
CallGraphAnalyzer::CallGraphAnalyzer(const LookupContext &context)
: ASTVisitor(context.thisDocument()->translationUnit()),
//	_id(0),
//	_declSymbol(0),
	_doc(context.thisDocument()),
	_snapshot(context.snapshot()),
	_context(context),
	_originalSource(_doc->utf8Source()),
	_source(_doc->utf8Source()),
	_currentScope(0)
{
	typeofExpression.init(_doc, _snapshot, _context.bindings());

	prepareLines(_originalSource);
}*/

QByteArray CallGraphAnalyzer::getSource(const QString &fileName,
					                    const CppTools::CppModelManagerInterface::WorkingCopy &workingCopy)
{
	if (workingCopy.contains(fileName)) {
		return workingCopy.source(fileName);
	} else {
		QString fileContents;
		::Utils::TextFileFormat format;
		QString error;
		QTextCodec *defaultCodec = Core::EditorManager::defaultTextCodec();
		::Utils::TextFileFormat::ReadResult result = ::Utils::TextFileFormat::readFile(
			fileName, defaultCodec, &fileContents, &format, &error);
		if (result != ::Utils::TextFileFormat::ReadSuccess)
			qWarning() << "Could not read " << fileName << ". Error: " << error;

		return fileContents.toUtf8();
	}
}

// QList<Usage> CallGraphAnalyzer::usages() const
// { return _usages; }

// QList<int> CallGraphAnalyzer::references() const
// { return _references; }

void CodeAtlas::CallGraphAnalyzer::extractCallList()
{
	_processed.clear();
// 	_references.clear();
// 	_usages.clear();

	m_nthCallLayer = 0;

	if (AST *ast = _doc->translationUnit()->ast())
		translationUnit(ast->asTranslationUnit());
}

/*
void CallGraphAnalyzer::operator()(Symbol *symbol)
{
//	if (! symbol)
//		return; 

/ *	_id = symbol->identifier();

	if (! _id)
		return;* /

	_processed.clear();
	_references.clear();
	_usages.clear();
//	_declSymbol = symbol;
//	_declSymbolFullyQualifiedName = LookupContext::fullyQualifiedName(symbol);

	// get the canonical id
//	_id = _doc->control()->identifier(_id->chars(), _id->size());
	m_nthCallLayer = 0;

	if (AST *ast = _doc->translationUnit()->ast())
		translationUnit(ast->asTranslationUnit());
}*/

void CallGraphAnalyzer::reportResult(unsigned tokenIndex, const Name *name, Scope *scope)
{
	if (! (tokenIndex && name != 0))
		return;

// 	if (name->identifier() != _id)
// 		return;

	if (! scope)
		scope = _currentScope;

	const QList<LookupItem> candidates = _context.lookup(name, scope);
	reportResult(tokenIndex, candidates);
}

void CallGraphAnalyzer::reportResult(unsigned tokenIndex, const Identifier *id, Scope *scope)
{
	reportResult(tokenIndex, static_cast<const Name *>(id), scope);
}

void CallGraphAnalyzer::reportResult(unsigned tokenIndex, const QList<LookupItem> &candidates)
{
	if (_processed.contains(tokenIndex))
		return;

	const bool isStrongResult = checkCandidates(candidates);

	if (isStrongResult)
		reportResult(tokenIndex);
}

QString CallGraphAnalyzer::matchingLine(const Token &tk) const
{
	const char *beg = _source.constData();
#ifdef USING_QT_4
	const char *cp = beg + tk.offset;
#elif defined(USING_QT_5)
	const char *cp = beg + tk.byteOffset;
#endif
	for (; cp != beg - 1; --cp) {
		if (*cp == '\n')
			break;
	}

	++cp;

	const char *lineEnd = cp + 1;
	for (; *lineEnd; ++lineEnd) {
		if (*lineEnd == '\n')
			break;
	}

	return QString::fromUtf8(cp, lineEnd - cp);
}

void CallGraphAnalyzer::reportResult(unsigned tokenIndex)
{
	const Token &tk = tokenAt(tokenIndex);
	qDebug("report %s", tk.spell());
	if (tk.generated())
		return;
	else if (_processed.contains(tokenIndex))
		return;

	_processed.insert(tokenIndex);

	unsigned line, col;
	getTokenStartPosition(tokenIndex, &line, &col);
	QString lineText;
	if (line < _sourceLineEnds.size())
		lineText = fetchLine(line);
	else
		lineText = matchingLine(tk);

	if (col)
		--col;  // adjust the column position.

#ifdef USING_QT_4
	const int len = tk.f.length;
#elif defined(USING_QT_5)
	const int len = tk.f.bytes;
#endif

// 	const Usage u(_doc->fileName(), lineText, line, col, len);
// 	_usages.append(u);
// 	_references.append(tokenIndex);
}

bool CallGraphAnalyzer::isLocalScope(Scope *scope)
{
	if (scope) {
		if (scope->isBlock() || scope->isTemplate() || scope->isFunction())
			return true;
	}

	return false;
}

bool CallGraphAnalyzer::checkCandidates(const QList<LookupItem> &candidates) const
{
	for (int i = candidates.size() - 1; i != -1; --i) {
		const LookupItem &r = candidates.at(i);

		if (Symbol *s = r.declaration()) {
// 			if (_declSymbol->isTypenameArgument()) {
// 				if (s != _declSymbol)
// 					return false;
// 			}
/*
			if (isLocalScope(_declSymbol->enclosingScope()) || isLocalScope(s->enclosingScope())) {
				if (_declSymbol->isClass() && _declSymbol->enclosingScope()->isTemplate()
					&& s->isClass() && s->enclosingScope()->isTemplate()) {
						// for definition of functions of class defined outside the class definition
						Scope *templEnclosingDeclSymbol = _declSymbol->enclosingScope();
						Scope *scopeOfTemplEnclosingDeclSymbol
							= templEnclosingDeclSymbol->enclosingScope();
						Scope *templEnclosingCandidateSymbol = s->enclosingScope();
						Scope *scopeOfTemplEnclosingCandidateSymbol
							= templEnclosingCandidateSymbol->enclosingScope();

						if (scopeOfTemplEnclosingCandidateSymbol != scopeOfTemplEnclosingDeclSymbol)
							return false;
				} else if (_declSymbol->isClass() && _declSymbol->enclosingScope()->isTemplate()
					&& s->enclosingScope()->isClass()
					&& s->enclosingScope()->enclosingScope()->isTemplate()) {
						// for declaration inside template class
						Scope *templEnclosingDeclSymbol = _declSymbol->enclosingScope();
						Scope *scopeOfTemplEnclosingDeclSymbol
							= templEnclosingDeclSymbol->enclosingScope();
						Scope *templEnclosingCandidateSymbol = s->enclosingScope()->enclosingScope();
						Scope *scopeOfTemplEnclosingCandidateSymbol
							= templEnclosingCandidateSymbol->enclosingScope();

						if (scopeOfTemplEnclosingCandidateSymbol !=  scopeOfTemplEnclosingDeclSymbol)
							return false;
				} else if (s->enclosingScope()->isTemplate() && ! _declSymbol->isTypenameArgument()) {
					if (_declSymbol->enclosingScope()->isTemplate()) {
						if (s->enclosingScope()->enclosingScope() != _declSymbol->enclosingScope()->enclosingScope())
							return false;
					} else {
						if (s->enclosingScope()->enclosingScope() != _declSymbol->enclosingScope())
							return false;
					}
				} else if (_declSymbol->enclosingScope()->isTemplate() && s->isTemplate()) {
					if (_declSymbol->enclosingScope()->enclosingScope() != s->enclosingScope())
						return false;
				} else if (! s->isUsingDeclaration()
					&& s->enclosingScope() != _declSymbol->enclosingScope()) {
						return false;
				}
			}

			if (compareFullyQualifiedName(LookupContext::fullyQualifiedName(s), _declSymbolFullyQualifiedName))
				return true;*/
		}
	}

	return false;
}

void CallGraphAnalyzer::checkExpression(unsigned startToken, unsigned endToken, Scope *scope)
{
#ifdef USING_QT_4
	const unsigned begin = tokenAt(startToken).begin();
	const unsigned end = tokenAt(endToken).end();
#elif defined(USING_QT_5)
	const unsigned begin = tokenAt(startToken).bytesBegin();
	const unsigned end = tokenAt(endToken).bytesEnd();
#endif

	const QByteArray expression = _source.mid(begin, end - begin);
	// qDebug() << "*** check expression:" << expression;

	if (! scope)
		scope = _currentScope;

	// make possible to instantiate templates
	typeofExpression.setExpandTemplates(true);
	const QList<LookupItem> results = typeofExpression(expression, scope, TypeOfExpression::Preprocess);
	reportResult(endToken, results);
}

Scope *CallGraphAnalyzer::switchScope(Scope *scope)
{
	if (! scope)
		return _currentScope;

	Scope *previousScope = _currentScope;
	_currentScope = scope;
	return previousScope;
}

void CallGraphAnalyzer::statement(StatementAST *ast)
{
	accept(ast);
}

void CallGraphAnalyzer::expression(ExpressionAST *ast)
{

	accept(ast);
}
void CallGraphAnalyzer::callExpression(ExpressionAST *ast)
{ 
	accept(ast);
}

void CallGraphAnalyzer::declaration(DeclarationAST *ast)
{
	accept(ast);
}

const Name *CallGraphAnalyzer::name(NameAST *ast)
{
	if (ast) {
		accept(ast);
		return ast->name;
	}

	return 0;
}

void CallGraphAnalyzer::specifier(SpecifierAST *ast)
{
	accept(ast);
}

void CallGraphAnalyzer::ptrOperator(PtrOperatorAST *ast)
{
	accept(ast);
}

void CallGraphAnalyzer::coreDeclarator(CoreDeclaratorAST *ast)
{
	accept(ast);
}

void CallGraphAnalyzer::postfixDeclarator(PostfixDeclaratorAST *ast)
{
	accept(ast);
}

// AST
bool CallGraphAnalyzer::visit(ObjCSelectorArgumentAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::objCSelectorArgument(ObjCSelectorArgumentAST *ast)
{
	if (! ast)
		return;

	// unsigned name_token = ast->name_token;
	// unsigned colon_token = ast->colon_token;
}
#ifdef USING_QT_4
bool CallGraphAnalyzer::visit(AttributeAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::attribute(AttributeAST *ast)
{
	if (! ast)
		return;

	// unsigned identifier_token = ast->identifier_token;
	// unsigned lparen_token = ast->lparen_token;
	// unsigned tag_token = ast->tag_token;
	for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
		this->expression(it->value);
	}
	// unsigned rparen_token = ast->rparen_token;
}
#elif defined(USING_QT_5)
#endif

bool CallGraphAnalyzer::visit(DeclaratorAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::declarator(DeclaratorAST *ast, Scope *symbol)
{
	if (! ast)
		return;

	for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
		this->specifier(it->value);
	}
	for (PtrOperatorListAST *it = ast->ptr_operator_list; it; it = it->next) {
		this->ptrOperator(it->value);
	}

	Scope *previousScope = switchScope(symbol);

	this->coreDeclarator(ast->core_declarator);
	for (PostfixDeclaratorListAST *it = ast->postfix_declarator_list; it; it = it->next) {
		this->postfixDeclarator(it->value);
	}
	for (SpecifierListAST *it = ast->post_attribute_list; it; it = it->next) {
		this->specifier(it->value);
	}
	// unsigned equals_token = ast->equals_token;
	this->expression(ast->initializer);
	(void) switchScope(previousScope);
}

bool CallGraphAnalyzer::visit(QtPropertyDeclarationItemAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::qtPropertyDeclarationItem(QtPropertyDeclarationItemAST *ast)
{
	if (! ast)
		return;

	// unsigned item_name_token = ast->item_name_token;
	this->expression(ast->expression);
}

bool CallGraphAnalyzer::visit(QtInterfaceNameAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::qtInterfaceName(QtInterfaceNameAST *ast)
{
	if (! ast)
		return;

	/*const Name *interface_name =*/ this->name(ast->interface_name);
	for (NameListAST *it = ast->constraint_list; it; it = it->next) {
		/*const Name *value =*/ this->name(it->value);
	}
}

bool CallGraphAnalyzer::visit(BaseSpecifierAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::baseSpecifier(BaseSpecifierAST *ast)
{
	if (! ast)
		return;

	// unsigned virtual_token = ast->virtual_token;
	// unsigned access_specifier_token = ast->access_specifier_token;
	/*const Name *name =*/ this->name(ast->name);
	// BaseClass *symbol = ast->symbol;
}

bool CallGraphAnalyzer::visit(CtorInitializerAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::ctorInitializer(CtorInitializerAST *ast)
{
	if (! ast)
		return;

	// unsigned colon_token = ast->colon_token;
	for (MemInitializerListAST *it = ast->member_initializer_list; it; it = it->next) {
		this->memInitializer(it->value);
	}
	// unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
}

bool CallGraphAnalyzer::visit(EnumeratorAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::enumerator(EnumeratorAST *ast)
{
	if (! ast)
		return;

	// unsigned identifier_token = ast->identifier_token;
	reportResult(ast->identifier_token, identifier(ast->identifier_token));
	// unsigned equal_token = ast->equal_token;
	this->expression(ast->expression);
}

bool CallGraphAnalyzer::visit(DynamicExceptionSpecificationAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::exceptionSpecification(ExceptionSpecificationAST *ast)
{
	if (! ast)
		return;

	if (DynamicExceptionSpecificationAST *dyn = ast->asDynamicExceptionSpecification()) {
		// unsigned throw_token = ast->throw_token;
		// unsigned lparen_token = ast->lparen_token;
		// unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
		for (ExpressionListAST *it = dyn->type_id_list; it; it = it->next) {
			this->expression(it->value);
		}
		// unsigned rparen_token = ast->rparen_token;
	} else if (NoExceptSpecificationAST *no = ast->asNoExceptSpecification()) {
		this->expression(no->expression);
	}
}

bool CallGraphAnalyzer::visit(MemInitializerAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::memInitializer(MemInitializerAST *ast)
{
	if (! ast)
		return;

	if (_currentScope && _currentScope->isFunction()) {
		Class *classScope = _currentScope->enclosingClass();
		if (! classScope) {
			if (ClassOrNamespace *binding = _context.lookupType(_currentScope)) {
				foreach (Symbol *s, binding->symbols()) {
					if (Class *k = s->asClass()) {
						classScope = k;
						break;
					}
				}
			}
		}

		if (classScope) {
			Scope *previousScope = switchScope(classScope);
			/*const Name *name =*/ this->name(ast->name);
			(void) switchScope(previousScope);
		}
	}
	this->expression(ast->expression);
}

bool CallGraphAnalyzer::visit(NestedNameSpecifierAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::nestedNameSpecifier(NestedNameSpecifierAST *ast)
{
	if (! ast)
		return;

	/*const Name *class_or_namespace_name =*/ this->name(ast->class_or_namespace_name);
	// unsigned scope_token = ast->scope_token;
}

bool CallGraphAnalyzer::visit(ExpressionListParenAST *ast)
{
	// unsigned lparen_token = ast->lparen_token;
	for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
		this->expression(it->value);
	}
	// unsigned rparen_token = ast->rparen_token;
	return false;
}

void CallGraphAnalyzer::newPlacement(ExpressionListParenAST *ast)
{
	if (! ast)
		return;

	// unsigned lparen_token = ast->lparen_token;
	for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
		this->expression(it->value);
	}
	// unsigned rparen_token = ast->rparen_token;
}

bool CallGraphAnalyzer::visit(NewArrayDeclaratorAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::newArrayDeclarator(NewArrayDeclaratorAST *ast)
{
	if (! ast)
		return;

	// unsigned lbracket_token = ast->lbracket_token;
	this->expression(ast->expression);
	// unsigned rbracket_token = ast->rbracket_token;
}

bool CallGraphAnalyzer::visit(NewTypeIdAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::newTypeId(NewTypeIdAST *ast)
{
	if (! ast)
		return;

	for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
		this->specifier(it->value);
	}
	for (PtrOperatorListAST *it = ast->ptr_operator_list; it; it = it->next) {
		this->ptrOperator(it->value);
	}
	for (NewArrayDeclaratorListAST *it = ast->new_array_declarator_list; it; it = it->next) {
		this->newArrayDeclarator(it->value);
	}
}

bool CallGraphAnalyzer::visit(OperatorAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::cppOperator(OperatorAST *ast)
{
	if (! ast)
		return;

	// unsigned op_token = ast->op_token;
	// unsigned open_token = ast->open_token;
	// unsigned close_token = ast->close_token;
}

bool CallGraphAnalyzer::visit(ParameterDeclarationClauseAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::parameterDeclarationClause(ParameterDeclarationClauseAST *ast)
{
	if (! ast)
		return;

	for (ParameterDeclarationListAST *it = ast->parameter_declaration_list; it; it = it->next) {
		this->declaration(it->value);
	}
	// unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
}

bool CallGraphAnalyzer::visit(TranslationUnitAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::translationUnit(TranslationUnitAST *ast)
{
	if (! ast)
		return;

	Scope *previousScope = switchScope(_doc->globalNamespace());
	for (DeclarationListAST *it = ast->declaration_list; it; it = it->next) {
		qDebug("############### Declare ###############");
		this->declaration(it->value);
	}
	(void) switchScope(previousScope);
}

bool CallGraphAnalyzer::visit(ObjCProtocolRefsAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::objCProtocolRefs(ObjCProtocolRefsAST *ast)
{
	if (! ast)
		return;

	// unsigned less_token = ast->less_token;
	for (NameListAST *it = ast->identifier_list; it; it = it->next) {
		/*const Name *value =*/ this->name(it->value);
	}
	// unsigned greater_token = ast->greater_token;
}

bool CallGraphAnalyzer::visit(ObjCMessageArgumentAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::objCMessageArgument(ObjCMessageArgumentAST *ast)
{
	if (! ast)
		return;

	this->expression(ast->parameter_value_expression);
}

bool CallGraphAnalyzer::visit(ObjCTypeNameAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::objCTypeName(ObjCTypeNameAST *ast)
{
	if (! ast)
		return;

	// unsigned lparen_token = ast->lparen_token;
	// unsigned type_qualifier_token = ast->type_qualifier_token;
	this->expression(ast->type_id);
	// unsigned rparen_token = ast->rparen_token;
}

bool CallGraphAnalyzer::visit(ObjCInstanceVariablesDeclarationAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::objCInstanceVariablesDeclaration(ObjCInstanceVariablesDeclarationAST *ast)
{
	if (! ast)
		return;

	// unsigned lbrace_token = ast->lbrace_token;
	for (DeclarationListAST *it = ast->instance_variable_list; it; it = it->next) {
		this->declaration(it->value);
	}
	// unsigned rbrace_token = ast->rbrace_token;
}

bool CallGraphAnalyzer::visit(ObjCPropertyAttributeAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::objCPropertyAttribute(ObjCPropertyAttributeAST *ast)
{
	if (! ast)
		return;

	// unsigned attribute_identifier_token = ast->attribute_identifier_token;
	// unsigned equals_token = ast->equals_token;
	/*const Name *method_selector =*/ this->name(ast->method_selector);
}

bool CallGraphAnalyzer::visit(ObjCMessageArgumentDeclarationAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::objCMessageArgumentDeclaration(ObjCMessageArgumentDeclarationAST *ast)
{
	if (! ast)
		return;

	this->objCTypeName(ast->type_name);
	for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
		this->specifier(it->value);
	}
	/*const Name *param_name =*/ this->name(ast->param_name);
	// Argument *argument = ast->argument;
}

bool CallGraphAnalyzer::visit(ObjCMethodPrototypeAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::objCMethodPrototype(ObjCMethodPrototypeAST *ast)
{
	if (! ast)
		return;

	// unsigned method_type_token = ast->method_type_token;
	this->objCTypeName(ast->type_name);
	/*const Name *selector =*/ this->name(ast->selector);
	Scope *previousScope = switchScope(ast->symbol);
	for (ObjCMessageArgumentDeclarationListAST *it = ast->argument_list; it; it = it->next) {
		this->objCMessageArgumentDeclaration(it->value);
	}
	// unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
	for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
		this->specifier(it->value);
	}
	// ObjCMethod *symbol = ast->symbol;
	(void) switchScope(previousScope);
}

bool CallGraphAnalyzer::visit(ObjCSynthesizedPropertyAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::objCSynthesizedProperty(ObjCSynthesizedPropertyAST *ast)
{
	if (! ast)
		return;

	// unsigned property_identifier_token = ast->property_identifier_token;
	// unsigned equals_token = ast->equals_token;
	// unsigned alias_identifier_token = ast->alias_identifier_token;
}

bool CallGraphAnalyzer::visit(LambdaIntroducerAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::lambdaIntroducer(LambdaIntroducerAST *ast)
{
	if (! ast)
		return;

	// unsigned lbracket_token = ast->lbracket_token;
	this->lambdaCapture(ast->lambda_capture);
	// unsigned rbracket_token = ast->rbracket_token;
}

bool CallGraphAnalyzer::visit(LambdaCaptureAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::lambdaCapture(LambdaCaptureAST *ast)
{
	if (! ast)
		return;

	// unsigned default_capture_token = ast->default_capture_token;
	for (CaptureListAST *it = ast->capture_list; it; it = it->next) {
		this->capture(it->value);
	}
}

bool CallGraphAnalyzer::visit(CaptureAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::capture(CaptureAST *ast)
{
	if (! ast)
		return;

	this->name(ast->identifier);
}

bool CallGraphAnalyzer::visit(LambdaDeclaratorAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::lambdaDeclarator(LambdaDeclaratorAST *ast)
{
	if (! ast)
		return;

	// unsigned lparen_token = ast->lparen_token;
	this->parameterDeclarationClause(ast->parameter_declaration_clause);
	// unsigned rparen_token = ast->rparen_token;
	for (SpecifierListAST *it = ast->attributes; it; it = it->next) {
		this->specifier(it->value);
	}
	// unsigned mutable_token = ast->mutable_token;
	this->exceptionSpecification(ast->exception_specification);
	this->trailingReturnType(ast->trailing_return_type);
}

bool CallGraphAnalyzer::visit(TrailingReturnTypeAST *ast)
{
	(void) ast;
	Q_ASSERT(!"unreachable");
	return false;
}

void CallGraphAnalyzer::trailingReturnType(TrailingReturnTypeAST *ast)
{
	if (! ast)
		return;

	// unsigned arrow_token = ast->arrow_token;
	for (SpecifierListAST *it = ast->attributes; it; it = it->next) {
		this->specifier(it->value);
	}
	for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
		this->specifier(it->value);
	}
	this->declarator(ast->declarator);
}


// StatementAST
bool CallGraphAnalyzer::visit(QtMemberDeclarationAST *ast)
{
	// unsigned q_token = ast->q_token;
	// unsigned lparen_token = ast->lparen_token;
	this->expression(ast->type_id);
	// unsigned rparen_token = ast->rparen_token;
	return false;
}

bool CallGraphAnalyzer::visit(CaseStatementAST *ast)
{
	// unsigned case_token = ast->case_token;
	this->expression(ast->expression);
	// unsigned colon_token = ast->colon_token;
	this->statement(ast->statement);
	return false;
}

bool CallGraphAnalyzer::visit(CompoundStatementAST *ast)
{
	// unsigned lbrace_token = ast->lbrace_token;
	Scope *previousScope = switchScope(ast->symbol);
	for (StatementListAST *it = ast->statement_list; it; it = it->next) {
		this->statement(it->value);
	}
	// unsigned rbrace_token = ast->rbrace_token;
	// Block *symbol = ast->symbol;
	(void) switchScope(previousScope);
	return false;
}

bool CallGraphAnalyzer::visit(DeclarationStatementAST *ast)
{
	this->declaration(ast->declaration);
	return false;
}

bool CallGraphAnalyzer::visit(DoStatementAST *ast)
{
	// unsigned do_token = ast->do_token;
	this->statement(ast->statement);
	// unsigned while_token = ast->while_token;
	// unsigned lparen_token = ast->lparen_token;
	this->expression(ast->expression);
	// unsigned rparen_token = ast->rparen_token;
	// unsigned semicolon_token = ast->semicolon_token;
	return false;
}

bool CallGraphAnalyzer::visit(ExpressionOrDeclarationStatementAST *ast)
{
	this->statement(ast->expression);
	this->statement(ast->declaration);
	return false;
}

bool CallGraphAnalyzer::visit(ExpressionStatementAST *ast)
{
	this->expression(ast->expression);
	// unsigned semicolon_token = ast->semicolon_token;
	return false;
}

bool CallGraphAnalyzer::visit(ForeachStatementAST *ast)
{
	// unsigned foreach_token = ast->foreach_token;
	// unsigned lparen_token = ast->lparen_token;
	Scope *previousScope = switchScope(ast->symbol);
	for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
		this->specifier(it->value);
	}
	this->declarator(ast->declarator);
	this->expression(ast->initializer);
	// unsigned comma_token = ast->comma_token;
	this->expression(ast->expression);
	// unsigned rparen_token = ast->rparen_token;
	this->statement(ast->statement);
	// Block *symbol = ast->symbol;
	(void) switchScope(previousScope);
	return false;
}

bool CallGraphAnalyzer::visit(RangeBasedForStatementAST *ast)
{
	Scope *previousScope = switchScope(ast->symbol);
	for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
		this->specifier(it->value);
	}
	this->declarator(ast->declarator);
	// unsigned comma_token = ast->comma_token;
	this->expression(ast->expression);
	// unsigned rparen_token = ast->rparen_token;
	this->statement(ast->statement);
	// Block *symbol = ast->symbol;
	(void) switchScope(previousScope);
	return false;
}

bool CallGraphAnalyzer::visit(ForStatementAST *ast)
{
	// unsigned for_token = ast->for_token;
	// unsigned lparen_token = ast->lparen_token;
	Scope *previousScope = switchScope(ast->symbol);
	this->statement(ast->initializer);
	this->expression(ast->condition);
	// unsigned semicolon_token = ast->semicolon_token;
	this->expression(ast->expression);
	// unsigned rparen_token = ast->rparen_token;
	this->statement(ast->statement);
	// Block *symbol = ast->symbol;
	(void) switchScope(previousScope);
	return false;
}

bool CallGraphAnalyzer::visit(IfStatementAST *ast)
{
	// unsigned if_token = ast->if_token;
	// unsigned lparen_token = ast->lparen_token;
	Scope *previousScope = switchScope(ast->symbol);
	this->expression(ast->condition);
	// unsigned rparen_token = ast->rparen_token;
	this->statement(ast->statement);
	// unsigned else_token = ast->else_token;
	this->statement(ast->else_statement);
	// Block *symbol = ast->symbol;
	(void) switchScope(previousScope);
	return false;
}

bool CallGraphAnalyzer::visit(LabeledStatementAST *ast)
{
	// unsigned label_token = ast->label_token;
	// unsigned colon_token = ast->colon_token;
	this->statement(ast->statement);
	return false;
}

bool CallGraphAnalyzer::visit(BreakStatementAST *ast)
{
	(void) ast;
	// unsigned break_token = ast->break_token;
	// unsigned semicolon_token = ast->semicolon_token;
	return false;
}

bool CallGraphAnalyzer::visit(ContinueStatementAST *ast)
{
	(void) ast;
	// unsigned continue_token = ast->continue_token;
	// unsigned semicolon_token = ast->semicolon_token;
	return false;
}

bool CallGraphAnalyzer::visit(GotoStatementAST *ast)
{
	(void) ast;
	// unsigned goto_token = ast->goto_token;
	// unsigned identifier_token = ast->identifier_token;
	// unsigned semicolon_token = ast->semicolon_token;
	return false;
}

bool CallGraphAnalyzer::visit(ReturnStatementAST *ast)
{
	// unsigned return_token = ast->return_token;
	this->expression(ast->expression);
	// unsigned semicolon_token = ast->semicolon_token;
	return false;
}

bool CallGraphAnalyzer::visit(SwitchStatementAST *ast)
{
	// unsigned switch_token = ast->switch_token;
	// unsigned lparen_token = ast->lparen_token;
	Scope *previousScope = switchScope(ast->symbol);
	this->expression(ast->condition);
	// unsigned rparen_token = ast->rparen_token;
	this->statement(ast->statement);
	// Block *symbol = ast->symbol;
	(void) switchScope(previousScope);
	return false;
}

bool CallGraphAnalyzer::visit(TryBlockStatementAST *ast)
{
	// unsigned try_token = ast->try_token;
	this->statement(ast->statement);
	for (CatchClauseListAST *it = ast->catch_clause_list; it; it = it->next) {
		this->statement(it->value);
	}
	return false;
}

bool CallGraphAnalyzer::visit(CatchClauseAST *ast)
{
	// unsigned catch_token = ast->catch_token;
	// unsigned lparen_token = ast->lparen_token;
	Scope *previousScope = switchScope(ast->symbol);
	this->declaration(ast->exception_declaration);
	// unsigned rparen_token = ast->rparen_token;
	this->statement(ast->statement);
	// Block *symbol = ast->symbol;
	(void) switchScope(previousScope);
	return false;
}

bool CallGraphAnalyzer::visit(WhileStatementAST *ast)
{
	// unsigned while_token = ast->while_token;
	// unsigned lparen_token = ast->lparen_token;
	Scope *previousScope = switchScope(ast->symbol);
	this->expression(ast->condition);
	// unsigned rparen_token = ast->rparen_token;
	this->statement(ast->statement);
	// Block *symbol = ast->symbol;
	(void) switchScope(previousScope);
	return false;
}

bool CallGraphAnalyzer::visit(ObjCFastEnumerationAST *ast)
{
	// unsigned for_token = ast->for_token;
	// unsigned lparen_token = ast->lparen_token;
	Scope *previousScope = switchScope(ast->symbol);
	for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
		this->specifier(it->value);
	}
	this->declarator(ast->declarator);
	this->expression(ast->initializer);
	// unsigned in_token = ast->in_token;
	this->expression(ast->fast_enumeratable_expression);
	// unsigned rparen_token = ast->rparen_token;
	this->statement(ast->statement);
	// Block *symbol = ast->symbol;
	(void) switchScope(previousScope);
	return false;
}

bool CallGraphAnalyzer::visit(ObjCSynchronizedStatementAST *ast)
{
	// unsigned synchronized_token = ast->synchronized_token;
	// unsigned lparen_token = ast->lparen_token;
	this->expression(ast->synchronized_object);
	// unsigned rparen_token = ast->rparen_token;
	this->statement(ast->statement);
	return false;
}


// ExpressionAST
bool CallGraphAnalyzer::visit(IdExpressionAST *ast)
{
	/*const Name *name =*/ this->name(ast->name);
	return false;
}

bool CallGraphAnalyzer::visit(CompoundExpressionAST *ast)
{
	// unsigned lparen_token = ast->lparen_token;
	this->statement(ast->statement);
	// unsigned rparen_token = ast->rparen_token;
	return false;
}

bool CallGraphAnalyzer::visit(CompoundLiteralAST *ast)
{
	// unsigned lparen_token = ast->lparen_token;
	this->expression(ast->type_id);
	// unsigned rparen_token = ast->rparen_token;
	this->expression(ast->initializer);
	return false;
}

bool CallGraphAnalyzer::visit(QtMethodAST *ast)
{
	// unsigned method_token = ast->method_token;
	// unsigned lparen_token = ast->lparen_token;
	this->declarator(ast->declarator);
	// unsigned rparen_token = ast->rparen_token;
	return false;
}

bool CallGraphAnalyzer::visit(BinaryExpressionAST *ast)
{
	this->expression(ast->left_expression);
	// unsigned binary_op_token = ast->binary_op_token;
	this->expression(ast->right_expression);
	return false;
}

bool CallGraphAnalyzer::visit(CastExpressionAST *ast)
{
	// unsigned lparen_token = ast->lparen_token;
	this->expression(ast->type_id);
	// unsigned rparen_token = ast->rparen_token;
	this->expression(ast->expression);
	return false;
}

bool CallGraphAnalyzer::visit(ConditionAST *ast)
{
	for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
		this->specifier(it->value);
	}
	this->declarator(ast->declarator);
	return false;
}

bool CallGraphAnalyzer::visit(ConditionalExpressionAST *ast)
{
	this->expression(ast->condition);
	// unsigned question_token = ast->question_token;
	this->expression(ast->left_expression);
	// unsigned colon_token = ast->colon_token;
	this->expression(ast->right_expression);
	return false;
}

bool CallGraphAnalyzer::visit(CppCastExpressionAST *ast)
{
	// unsigned cast_token = ast->cast_token;
	// unsigned less_token = ast->less_token;
	this->expression(ast->type_id);
	// unsigned greater_token = ast->greater_token;
	// unsigned lparen_token = ast->lparen_token;
	this->expression(ast->expression);
	// unsigned rparen_token = ast->rparen_token;
	return false;
}

bool CallGraphAnalyzer::visit(DeleteExpressionAST *ast)
{
	// unsigned scope_token = ast->scope_token;
	// unsigned delete_token = ast->delete_token;
	// unsigned lbracket_token = ast->lbracket_token;
	// unsigned rbracket_token = ast->rbracket_token;
	this->expression(ast->expression);
	return false;
}

bool CallGraphAnalyzer::visit(ArrayInitializerAST *ast)
{
	// unsigned lbrace_token = ast->lbrace_token;
	for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
		this->expression(it->value);
	}
	// unsigned rbrace_token = ast->rbrace_token;
	return false;
}

bool CallGraphAnalyzer::visit(NewExpressionAST *ast)
{
	// unsigned scope_token = ast->scope_token;
	// unsigned new_token = ast->new_token;
	this->newPlacement(ast->new_placement);
	// unsigned lparen_token = ast->lparen_token;
	this->expression(ast->type_id);
	// unsigned rparen_token = ast->rparen_token;
	this->newTypeId(ast->new_type_id);
	this->expression(ast->new_initializer);
	return false;
}

bool CallGraphAnalyzer::visit(TypeidExpressionAST *ast)
{
	// unsigned typeid_token = ast->typeid_token;
	// unsigned lparen_token = ast->lparen_token;
	this->expression(ast->expression);
	// unsigned rparen_token = ast->rparen_token;
	return false;
}

bool CallGraphAnalyzer::visit(TypenameCallExpressionAST *ast)
{
	// unsigned typename_token = ast->typename_token;
	/*const Name *name =*/ this->name(ast->name);
	this->expression(ast->expression);
	return false;
}

bool CallGraphAnalyzer::visit(TypeConstructorCallAST *ast)
{
	for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
		this->specifier(it->value);
	}
	this->expression(ast->expression);
	return false;
}

bool CallGraphAnalyzer::visit(SizeofExpressionAST *ast)
{
	// unsigned sizeof_token = ast->sizeof_token;
	// unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
	// unsigned lparen_token = ast->lparen_token;
	this->expression(ast->expression);
	// unsigned rparen_token = ast->rparen_token;
	return false;
}

bool CallGraphAnalyzer::visit(PointerLiteralAST *ast)
{
	(void) ast;
	// unsigned literal_token = ast->literal_token;
	return false;
}

bool CallGraphAnalyzer::visit(NumericLiteralAST *ast)
{
	(void) ast;
	// unsigned literal_token = ast->literal_token;
	return false;
}

bool CallGraphAnalyzer::visit(BoolLiteralAST *ast)
{
	(void) ast;
	// unsigned literal_token = ast->literal_token;
	return false;
}

bool CallGraphAnalyzer::visit(ThisExpressionAST *ast)
{
	(void) ast;
	// unsigned this_token = ast->this_token;
	return false;
}

bool CallGraphAnalyzer::visit(NestedExpressionAST *ast)
{
	// unsigned lparen_token = ast->lparen_token;
	this->expression(ast->expression);
	// unsigned rparen_token = ast->rparen_token;
	return false;
}

bool CallGraphAnalyzer::visit(StringLiteralAST *ast)
{
	// unsigned literal_token = ast->literal_token;
	this->expression(ast->next);
	return false;
}

bool CallGraphAnalyzer::visit(ThrowExpressionAST *ast)
{
	// unsigned throw_token = ast->throw_token;
	this->expression(ast->expression);
	return false;
}

bool CallGraphAnalyzer::visit(TypeIdAST *ast)
{
	for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
		this->specifier(it->value);
	}
	this->declarator(ast->declarator);
	return false;
}

bool CallGraphAnalyzer::visit(UnaryExpressionAST *ast)
{
	// unsigned unary_op_token = ast->unary_op_token;
	this->expression(ast->expression);
	return false;
}

bool CallGraphAnalyzer::visit(ObjCMessageExpressionAST *ast)
{
	// unsigned lbracket_token = ast->lbracket_token;
	this->expression(ast->receiver_expression);
	/*const Name *selector =*/ this->name(ast->selector);
	for (ObjCMessageArgumentListAST *it = ast->argument_list; it; it = it->next) {
		this->objCMessageArgument(it->value);
	}
	// unsigned rbracket_token = ast->rbracket_token;
	return false;
}

bool CallGraphAnalyzer::visit(ObjCProtocolExpressionAST *ast)
{
	(void) ast;
	// unsigned protocol_token = ast->protocol_token;
	// unsigned lparen_token = ast->lparen_token;
	// unsigned identifier_token = ast->identifier_token;
	// unsigned rparen_token = ast->rparen_token;
	return false;
}

bool CallGraphAnalyzer::visit(ObjCEncodeExpressionAST *ast)
{
	// unsigned encode_token = ast->encode_token;
	this->objCTypeName(ast->type_name);
	return false;
}

bool CallGraphAnalyzer::visit(ObjCSelectorExpressionAST *ast)
{
	// unsigned selector_token = ast->selector_token;
	// unsigned lparen_token = ast->lparen_token;
	/*const Name *selector =*/ this->name(ast->selector);
	// unsigned rparen_token = ast->rparen_token;
	return false;
}

bool CallGraphAnalyzer::visit(LambdaExpressionAST *ast)
{
	this->lambdaIntroducer(ast->lambda_introducer);
	this->lambdaDeclarator(ast->lambda_declarator);
	this->statement(ast->statement);
	return false;
}

bool CallGraphAnalyzer::visit(BracedInitializerAST *ast)
{
	// unsigned lbrace_token = ast->lbrace_token;
	for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
		this->expression(it->value);
	}
	// unsigned comma_token = ast->comma_token;
	// unsigned rbrace_token = ast->rbrace_token;
	return false;
}


// DeclarationAST
bool CallGraphAnalyzer::visit(SimpleDeclarationAST *ast)
{
	// unsigned qt_invokable_token = ast->qt_invokable_token;
	for (SpecifierListAST *it = ast->decl_specifier_list; it; it = it->next) {
		this->specifier(it->value);
	}
	for (DeclaratorListAST *it = ast->declarator_list; it; it = it->next) {
		this->declarator(it->value);
	}
	// unsigned semicolon_token = ast->semicolon_token;
	// List<Declaration *> *symbols = ast->symbols;
	return false;
}

bool CallGraphAnalyzer::visit(EmptyDeclarationAST *ast)
{
	(void) ast;
	// unsigned semicolon_token = ast->semicolon_token;
	return false;
}

bool CallGraphAnalyzer::visit(AccessDeclarationAST *ast)
{
	(void) ast;
	// unsigned access_specifier_token = ast->access_specifier_token;
	// unsigned slots_token = ast->slots_token;
	// unsigned colon_token = ast->colon_token;
	return false;
}

bool CallGraphAnalyzer::visit(QtObjectTagAST *ast)
{
	(void) ast;
	// unsigned q_object_token = ast->q_object_token;
	return false;
}

bool CallGraphAnalyzer::visit(QtPrivateSlotAST *ast)
{
	// unsigned q_private_slot_token = ast->q_private_slot_token;
	// unsigned lparen_token = ast->lparen_token;
	// unsigned dptr_token = ast->dptr_token;
	// unsigned dptr_lparen_token = ast->dptr_lparen_token;
	// unsigned dptr_rparen_token = ast->dptr_rparen_token;
	// unsigned comma_token = ast->comma_token;
	for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
		this->specifier(it->value);
	}
	this->declarator(ast->declarator);
	// unsigned rparen_token = ast->rparen_token;
	return false;
}

bool CallGraphAnalyzer::visit(QtPropertyDeclarationAST *ast)
{
	// unsigned property_specifier_token = ast->property_specifier_token;
	// unsigned lparen_token = ast->lparen_token;
	this->expression(ast->type_id);
	/*const Name *property_name =*/ this->name(ast->property_name);
	for (QtPropertyDeclarationItemListAST *it = ast->property_declaration_item_list; it; it = it->next) {
		this->qtPropertyDeclarationItem(it->value);
	}
	// unsigned rparen_token = ast->rparen_token;
	return false;
}

bool CallGraphAnalyzer::visit(QtEnumDeclarationAST *ast)
{
	// unsigned enum_specifier_token = ast->enum_specifier_token;
	// unsigned lparen_token = ast->lparen_token;
	for (NameListAST *it = ast->enumerator_list; it; it = it->next) {
		/*const Name *value =*/ this->name(it->value);
	}
	// unsigned rparen_token = ast->rparen_token;
	return false;
}

bool CallGraphAnalyzer::visit(QtFlagsDeclarationAST *ast)
{
	// unsigned flags_specifier_token = ast->flags_specifier_token;
	// unsigned lparen_token = ast->lparen_token;
	for (NameListAST *it = ast->flag_enums_list; it; it = it->next) {
		/*const Name *value =*/ this->name(it->value);
	}
	// unsigned rparen_token = ast->rparen_token;
	return false;
}

bool CallGraphAnalyzer::visit(QtInterfacesDeclarationAST *ast)
{
	// unsigned interfaces_token = ast->interfaces_token;
	// unsigned lparen_token = ast->lparen_token;
	for (QtInterfaceNameListAST *it = ast->interface_name_list; it; it = it->next) {
		this->qtInterfaceName(it->value);
	}
	// unsigned rparen_token = ast->rparen_token;
	return false;
}

bool CallGraphAnalyzer::visit(AsmDefinitionAST *ast)
{
	(void) ast;
	// unsigned asm_token = ast->asm_token;
	// unsigned volatile_token = ast->volatile_token;
	// unsigned lparen_token = ast->lparen_token;
	// unsigned rparen_token = ast->rparen_token;
	// unsigned semicolon_token = ast->semicolon_token;
	return false;
}

bool CallGraphAnalyzer::visit(ExceptionDeclarationAST *ast)
{
	for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
		this->specifier(it->value);
	}
	this->declarator(ast->declarator);
	// unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
	return false;
}

bool CallGraphAnalyzer::visit(FunctionDefinitionAST *ast)
{
	// unsigned qt_invokable_token = ast->qt_invokable_token;
	for (SpecifierListAST *it = ast->decl_specifier_list; it; it = it->next) {
		this->specifier(it->value);
	}


	SymbolPath::findFunctionDefPath(ast->symbol, _context, m_curFunctionPath);
	m_callList[m_curFunctionPath]  = QList<CGItemInfo>();
	m_ithCall = 0;

	this->declarator(ast->declarator, ast->symbol);
	Scope *previousScope = switchScope(ast->symbol);
	this->ctorInitializer(ast->ctor_initializer);
	this->statement(ast->function_body);
	// Function *symbol = ast->symbol;
	(void) switchScope(previousScope);
	return false;
}

bool CallGraphAnalyzer::visit(LinkageBodyAST *ast)
{
	// unsigned lbrace_token = ast->lbrace_token;
	for (DeclarationListAST *it = ast->declaration_list; it; it = it->next) {
		this->declaration(it->value);
	}
	// unsigned rbrace_token = ast->rbrace_token;
	return false;
}

bool CallGraphAnalyzer::visit(LinkageSpecificationAST *ast)
{
	// unsigned extern_token = ast->extern_token;
	// unsigned extern_type_token = ast->extern_type_token;
	this->declaration(ast->declaration);
	return false;
}

bool CallGraphAnalyzer::visit(NamespaceAST *ast)
{
	// unsigned namespace_token = ast->namespace_token;
	// unsigned identifier_token = ast->identifier_token;
	reportResult(ast->identifier_token, identifier(ast->identifier_token));
	Scope *previousScope = switchScope(ast->symbol);
	for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
		this->specifier(it->value);
	}
	this->declaration(ast->linkage_body);
	// Namespace *symbol = ast->symbol;
	(void) switchScope(previousScope);
	return false;
}

bool CallGraphAnalyzer::visit(NamespaceAliasDefinitionAST *ast)
{
	// unsigned namespace_token = ast->namespace_token;
	// unsigned namespace_name_token = ast->namespace_name_token;
	// unsigned equal_token = ast->equal_token;
	/*const Name *name =*/ this->name(ast->name);
	// unsigned semicolon_token = ast->semicolon_token;
	return false;
}

bool CallGraphAnalyzer::visit(ParameterDeclarationAST *ast)
{
	for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
		this->specifier(it->value);
	}
	this->declarator(ast->declarator);
	// unsigned equal_token = ast->equal_token;
	this->expression(ast->expression);
	// Argument *symbol = ast->symbol;
	return false;
}

bool CallGraphAnalyzer::visit(StaticAssertDeclarationAST *ast)
{
	this->expression(ast->expression);
	return false;
}

bool CallGraphAnalyzer::visit(TemplateDeclarationAST *ast)
{
	// unsigned export_token = ast->export_token;
	// unsigned template_token = ast->template_token;
	// unsigned less_token = ast->less_token;

	Scope *previousScope = switchScope(ast->symbol);

	for (DeclarationListAST *it = ast->template_parameter_list; it; it = it->next) {
		this->declaration(it->value);
	}
	// unsigned greater_token = ast->greater_token;
	this->declaration(ast->declaration);

	(void) switchScope(previousScope);
	return false;
}

bool CallGraphAnalyzer::visit(TypenameTypeParameterAST *ast)
{
	// unsigned classkey_token = ast->classkey_token;
	// unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
	/*const Name *name =*/ this->name(ast->name);
	// unsigned equal_token = ast->equal_token;
	this->expression(ast->type_id);
	// TypenameArgument *symbol = ast->symbol;
	return false;
}

bool CallGraphAnalyzer::visit(TemplateTypeParameterAST *ast)
{
	// unsigned template_token = ast->template_token;
	// unsigned less_token = ast->less_token;
	for (DeclarationListAST *it = ast->template_parameter_list; it; it = it->next) {
		this->declaration(it->value);
	}
	// unsigned greater_token = ast->greater_token;
	// unsigned class_token = ast->class_token;
	// unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
	/*const Name *name =*/ this->name(ast->name);
	// unsigned equal_token = ast->equal_token;
	this->expression(ast->type_id);
	// TypenameArgument *symbol = ast->symbol;
	return false;
}

bool CallGraphAnalyzer::visit(UsingAST *ast)
{
	// unsigned using_token = ast->using_token;
	// unsigned typename_token = ast->typename_token;
	/*const Name *name =*/ this->name(ast->name);
	// unsigned semicolon_token = ast->semicolon_token;
	// UsingDeclaration *symbol = ast->symbol;
	return false;
}

bool CallGraphAnalyzer::visit(UsingDirectiveAST *ast)
{
	// unsigned using_token = ast->using_token;
	// unsigned namespace_token = ast->namespace_token;
	/*const Name *name =*/ this->name(ast->name);
	// unsigned semicolon_token = ast->semicolon_token;
	// UsingNamespaceDirective *symbol = ast->symbol;
	return false;
}

bool CallGraphAnalyzer::visit(ObjCClassForwardDeclarationAST *ast)
{
	for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
		this->specifier(it->value);
	}
	// unsigned class_token = ast->class_token;
	for (NameListAST *it = ast->identifier_list; it; it = it->next) {
		/*const Name *value =*/ this->name(it->value);
	}
	// unsigned semicolon_token = ast->semicolon_token;
	// List<ObjCForwardClassDeclaration *> *symbols = ast->symbols;
	return false;
}

bool CallGraphAnalyzer::visit(ObjCClassDeclarationAST *ast)
{
	for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
		this->specifier(it->value);
	}
	// unsigned interface_token = ast->interface_token;
	// unsigned implementation_token = ast->implementation_token;
	/*const Name *class_name =*/ this->name(ast->class_name);

	Scope *previousScope = switchScope(ast->symbol);

	// unsigned lparen_token = ast->lparen_token;
	/*const Name *category_name =*/ this->name(ast->category_name);
	// unsigned rparen_token = ast->rparen_token;
	// unsigned colon_token = ast->colon_token;
	/*const Name *superclass =*/ this->name(ast->superclass);
	this->objCProtocolRefs(ast->protocol_refs);
	this->objCInstanceVariablesDeclaration(ast->inst_vars_decl);
	for (DeclarationListAST *it = ast->member_declaration_list; it; it = it->next) {
		this->declaration(it->value);
	}
	// unsigned end_token = ast->end_token;
	// ObjCClass *symbol = ast->symbol;
	(void) switchScope(previousScope);
	return false;
}

bool CallGraphAnalyzer::visit(ObjCProtocolForwardDeclarationAST *ast)
{
	for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
		this->specifier(it->value);
	}
	// unsigned protocol_token = ast->protocol_token;
	for (NameListAST *it = ast->identifier_list; it; it = it->next) {
		/*const Name *value =*/ this->name(it->value);
	}
	// unsigned semicolon_token = ast->semicolon_token;
	// List<ObjCForwardProtocolDeclaration *> *symbols = ast->symbols;
	return false;
}

bool CallGraphAnalyzer::visit(ObjCProtocolDeclarationAST *ast)
{
	for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
		this->specifier(it->value);
	}
	// unsigned protocol_token = ast->protocol_token;
	/*const Name *name =*/ this->name(ast->name);

	Scope *previousScope = switchScope(ast->symbol);

	this->objCProtocolRefs(ast->protocol_refs);
	for (DeclarationListAST *it = ast->member_declaration_list; it; it = it->next) {
		this->declaration(it->value);
	}
	// unsigned end_token = ast->end_token;
	// ObjCProtocol *symbol = ast->symbol;
	(void) switchScope(previousScope);
	return false;
}

bool CallGraphAnalyzer::visit(ObjCVisibilityDeclarationAST *ast)
{
	(void) ast;
	// unsigned visibility_token = ast->visibility_token;
	return false;
}

bool CallGraphAnalyzer::visit(ObjCPropertyDeclarationAST *ast)
{
	for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
		this->specifier(it->value);
	}
	// unsigned property_token = ast->property_token;
	// unsigned lparen_token = ast->lparen_token;
	for (ObjCPropertyAttributeListAST *it = ast->property_attribute_list; it; it = it->next) {
		this->objCPropertyAttribute(it->value);
	}
	// unsigned rparen_token = ast->rparen_token;
	this->declaration(ast->simple_declaration);
	// List<ObjCPropertyDeclaration *> *symbols = ast->symbols;
	return false;
}

bool CallGraphAnalyzer::visit(ObjCMethodDeclarationAST *ast)
{
	this->objCMethodPrototype(ast->method_prototype);
	this->statement(ast->function_body);
	// unsigned semicolon_token = ast->semicolon_token;
	return false;
}

bool CallGraphAnalyzer::visit(ObjCSynthesizedPropertiesDeclarationAST *ast)
{
	// unsigned synthesized_token = ast->synthesized_token;
	for (ObjCSynthesizedPropertyListAST *it = ast->property_identifier_list; it; it = it->next) {
		this->objCSynthesizedProperty(it->value);
	}
	// unsigned semicolon_token = ast->semicolon_token;
	return false;
}

bool CallGraphAnalyzer::visit(ObjCDynamicPropertiesDeclarationAST *ast)
{
	// unsigned dynamic_token = ast->dynamic_token;
	for (NameListAST *it = ast->property_identifier_list; it; it = it->next) {
		/*const Name *value =*/ this->name(it->value);
	}
	// unsigned semicolon_token = ast->semicolon_token;
	return false;
}

// NameAST
bool CallGraphAnalyzer::visit(ObjCSelectorAST *ast)
{
	if (ast->name)
		reportResult(ast->firstToken(), ast->name);

	for (ObjCSelectorArgumentListAST *it = ast->selector_argument_list; it; it = it->next) {
		this->objCSelectorArgument(it->value);
	}
	return false;
}

bool CallGraphAnalyzer::visit(QualifiedNameAST *ast)
{
#if 0
	// unsigned global_scope_token = ast->global_scope_token;
	for (NestedNameSpecifierListAST *it = ast->nested_name_specifier_list; it; it = it->next) {
		this->nestedNameSpecifier(it->value);
	}
	const Name *unqualified_name = this->name(ast->unqualified_name);
#endif
	qDebug("qualified name");
	for (NestedNameSpecifierListAST *it = ast->nested_name_specifier_list; it; it = it->next) {
		NestedNameSpecifierAST *nested_name_specifier = it->value;

		if (NameAST *class_or_namespace_name = nested_name_specifier->class_or_namespace_name) {
			SimpleNameAST *simple_name = class_or_namespace_name->asSimpleName();
			

			TemplateIdAST *template_id = 0;
			if (! simple_name) {
				template_id = class_or_namespace_name->asTemplateId();

				if (template_id) {
					for (ExpressionListAST *arg_it = template_id->template_argument_list; arg_it; arg_it = arg_it->next) {
						this->expression(arg_it->value);
					}
				}
			}

			if (simple_name || template_id) {
				const unsigned identifier_token = simple_name
					? simple_name->identifier_token
					: template_id->identifier_token;

				Token tk = tokenAt(identifier_token);
				qDebug("\t nested name: %s",tk.spell());
//				if (identifier(identifier_token) == _id)
//					checkExpression(ast->firstToken(), identifier_token);
			}
		}
	}

	if (NameAST *unqualified_name = ast->unqualified_name) {
		unsigned identifier_token = 0;

		if (SimpleNameAST *simple_name = unqualified_name->asSimpleName())
			identifier_token = simple_name->identifier_token;
		else if (DestructorNameAST *dtor = unqualified_name->asDestructorName())
			identifier_token = dtor->unqualified_name->firstToken();

		TemplateIdAST *template_id = 0;
		if (! identifier_token) {
			template_id = unqualified_name->asTemplateId();

			if (template_id) {
				identifier_token = template_id->identifier_token;

				for (ExpressionListAST *template_arguments = template_id->template_argument_list;
					template_arguments; template_arguments = template_arguments->next) {
						this->expression(template_arguments->value);
				}
			}
		}

		Token tk = tokenAt(identifier_token);
		qDebug("\t unqualified name %s",tk.spell());
//		if (identifier_token && identifier(identifier_token) == _id)
//			checkExpression(ast->firstToken(), identifier_token);
	}

	return false;

	return false;
}

bool CallGraphAnalyzer::visit(OperatorFunctionIdAST *ast)
{
	// unsigned operator_token = ast->operator_token;
	this->cppOperator(ast->op);
	return false;
}

bool CallGraphAnalyzer::visit(ConversionFunctionIdAST *ast)
{
	// unsigned operator_token = ast->operator_token;
	for (SpecifierListAST *it = ast->type_specifier_list; it; it = it->next) {
		this->specifier(it->value);
	}
	for (PtrOperatorListAST *it = ast->ptr_operator_list; it; it = it->next) {
		this->ptrOperator(it->value);
	}
	return false;
}

bool CallGraphAnalyzer::visit(SimpleNameAST *ast)
{
	// unsigned identifier_token = ast->identifier_token;
	Token tk = tokenAt(ast->identifier_token);
	if (m_nthCallLayer != 0)
	{
		qDebug("call name %s", tk.spell());
	}
	qDebug("simple name %s", tk.spell());
	reportResult(ast->identifier_token, ast->name);
	
	return false;
}

bool CallGraphAnalyzer::visit(TemplateIdAST *ast)
{
	// unsigned identifier_token = ast->identifier_token;
	reportResult(ast->identifier_token, ast->name);
	// unsigned less_token = ast->less_token;
	for (ExpressionListAST *it = ast->template_argument_list; it; it = it->next) {
		this->expression(it->value);
	}
	// unsigned greater_token = ast->greater_token;
	return false;
}


// SpecifierAST
bool CallGraphAnalyzer::visit(SimpleSpecifierAST *ast)
{
	(void) ast;
	// unsigned specifier_token = ast->specifier_token;
	return false;
}
#ifdef USING_QT_4
bool CallGraphAnalyzer::visit(AttributeSpecifierAST *ast)
{
	// unsigned attribute_token = ast->attribute_token;
	// unsigned first_lparen_token = ast->first_lparen_token;
	// unsigned second_lparen_token = ast->second_lparen_token;
	for (AttributeListAST *it = ast->attribute_list; it; it = it->next) {
		this->attribute(it->value);
	}
	// unsigned first_rparen_token = ast->first_rparen_token;
	// unsigned second_rparen_token = ast->second_rparen_token;
	return false;
}
#elif defined(USING_QT_5)
bool CallGraphAnalyzer::visit(AttributeSpecifierAST *ast)
{
	if (AlignmentSpecifierAST* alignAST = dynamic_cast<AlignmentSpecifierAST*>(ast))
	{
		expression(alignAST->typeIdExprOrAlignmentExpr);
	}
	else if (GnuAttributeSpecifierAST* gnuAST = dynamic_cast<GnuAttributeSpecifierAST*>(ast))
	{
		for (GnuAttributeListAST *it = gnuAST->attribute_list; it; it = it->next) 
		{
			this->attribute(it->value);
		}
	}
	return false;
}
bool CallGraphAnalyzer::attribute(GnuAttributeAST* ast)
{
	for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
		this->expression(it->value);
	}
	return false;
}
#endif

bool CallGraphAnalyzer::visit(TypeofSpecifierAST *ast)
{
	// unsigned typeof_token = ast->typeof_token;
	// unsigned lparen_token = ast->lparen_token;
	this->expression(ast->expression);
	// unsigned rparen_token = ast->rparen_token;
	return false;
}

bool CallGraphAnalyzer::visit(DecltypeSpecifierAST *ast)
{
	// unsigned typeof_token = ast->typeof_token;
	// unsigned lparen_token = ast->lparen_token;
	this->expression(ast->expression);
	// unsigned rparen_token = ast->rparen_token;
	return false;
}

bool CallGraphAnalyzer::visit(ClassSpecifierAST *ast)
{
	// unsigned classkey_token = ast->classkey_token;
	for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
		this->specifier(it->value);
	}
	/*const Name *name =*/ this->name(ast->name);

	Scope *previousScope = switchScope(ast->symbol);

	// unsigned colon_token = ast->colon_token;
	for (BaseSpecifierListAST *it = ast->base_clause_list; it; it = it->next) {
		this->baseSpecifier(it->value);
	}
	// unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
	// unsigned lbrace_token = ast->lbrace_token;
	for (DeclarationListAST *it = ast->member_specifier_list; it; it = it->next) {
		this->declaration(it->value);
	}
	// unsigned rbrace_token = ast->rbrace_token;
	// Class *symbol = ast->symbol;
	(void) switchScope(previousScope);
	return false;
}

bool CallGraphAnalyzer::visit(NamedTypeSpecifierAST *ast)
{
	/*const Name *name =*/ this->name(ast->name);
	return false;
}

bool CallGraphAnalyzer::visit(ElaboratedTypeSpecifierAST *ast)
{
	// unsigned classkey_token = ast->classkey_token;
	for (SpecifierListAST *it = ast->attribute_list; it; it = it->next) {
		this->specifier(it->value);
	}
	/*const Name *name =*/ this->name(ast->name);
	return false;
}

bool CallGraphAnalyzer::visit(EnumSpecifierAST *ast)
{
	// unsigned enum_token = ast->enum_token;
	/*const Name *name =*/ this->name(ast->name);
	// unsigned lbrace_token = ast->lbrace_token;
	Scope *previousScope = switchScope(ast->symbol);
	for (EnumeratorListAST *it = ast->enumerator_list; it; it = it->next) {
		this->enumerator(it->value);
	}
	// unsigned rbrace_token = ast->rbrace_token;
	// Enum *symbol = ast->symbol;
	(void) switchScope(previousScope);
	return false;
}


// PtrOperatorAST
bool CallGraphAnalyzer::visit(PointerToMemberAST *ast)
{
	// unsigned global_scope_token = ast->global_scope_token;
	for (NestedNameSpecifierListAST *it = ast->nested_name_specifier_list; it; it = it->next) {
		this->nestedNameSpecifier(it->value);
	}
	// unsigned star_token = ast->star_token;
	for (SpecifierListAST *it = ast->cv_qualifier_list; it; it = it->next) {
		this->specifier(it->value);
	}
	return false;
}

bool CallGraphAnalyzer::visit(PointerAST *ast)
{
	// unsigned star_token = ast->star_token;
	for (SpecifierListAST *it = ast->cv_qualifier_list; it; it = it->next) {
		this->specifier(it->value);
	}
	return false;
}

bool CallGraphAnalyzer::visit(ReferenceAST *ast)
{
	(void) ast;
	// unsigned reference_token = ast->reference_token;
	return false;
}


// PostfixAST

void CallGraphAnalyzer::findFunctionCallItemPath(CallAST* ast, QList<CGItemInfo>& itemList)
{
	SymbolFinder     symbolFinder;
	const QList<LookupItem> resolvedSymbols = typeofExpression.reference(ast,this->_doc,this->_currentScope);

	if (resolvedSymbols.isEmpty()) 
		return;

	// find best candidates
	LookupItem result = skipForwardDeclarations(resolvedSymbols);
	foreach (const LookupItem &r, resolvedSymbols)
	{
		if (Symbol *d = r.declaration()) 
		{ 
			if (d->isDeclaration() || d->isFunction()) 
			{
				const QString fileName = QString::fromUtf8(d->fileName(), d->fileNameLength());
				unsigned line, column;
				getTokenPosition(ast->base_expression->lastToken(), &line, &column);
				if (fileName  == _doc->fileName() &&
					d->line() == line && d->column() <= column)
				{
					result = r;    break;
				}
			}
		}
	}

	itemList.clear();
	// check result symbol
	if (Symbol *symbol = result.declaration()) {

		// Consider to show a pop-up displaying overrides for the function
		Function *func = symbol->type()->asFunctionType();
		QList<CPlusPlus::Symbol*> possibleSymbolList;
		if (isVirtualFunctionOverrides(func, ast)) {
			Class *startClass = symbolFinder.findMatchingClassDeclaration(func, _snapshot);
			possibleSymbolList = getOverrideSymbols(startClass, func);
			//QString name = overview.prettyName(klass->name()).trimmed();
		}else
			possibleSymbolList.append(symbol);

		for (int ithSymbol = 0; ithSymbol < possibleSymbolList.size(); ++ithSymbol)
		{
			QString fileName = QString(QLatin1String(possibleSymbolList[ithSymbol]->fileName()));	
			SymbolPath itemPath;
			for (Symbol* curSymbol = possibleSymbolList[ithSymbol]; curSymbol; curSymbol = curSymbol->enclosingScope())
			{
				if (!curSymbol->name())
					break;
				const Overview& overview = SymbolInfo::getNamePrinter();
				QString name = overview.prettyName(curSymbol->name()).trimmed();
				if (name == "")
					break;
				QString type = overview.prettyType(curSymbol->type()).trimmed();
				SymbolInfo::ElementType iconType = SymbolInfo::elementTypeFromSymbol(curSymbol);
				SymbolInfo info(name, type, iconType);
				itemPath.addParentSymbol(info);
			}
			itemList.append(CGItemInfo(itemPath, fileName, m_ithCall));
		}
/*
		Symbol *def = 0;
		def = symbolFinder.findMatchingDefinition(symbol, _snapshot);
		QString name = overview.prettyName(def->name()).trimmed();

		if (resolveTarget) {
			Symbol *lastVisibleSymbol = doc->lastVisibleSymbolAt(line, column);

			def = findDefinition(symbol, snapshot, symbolFinder);

			if (def == lastVisibleSymbol)
				def = 0; // jump to declaration then.

			if (symbol->isForwardClassDeclaration())
				def = symbolFinder->findMatchingClassDeclaration(symbol, snapshot);
		}

		link = m_widget->linkToSymbol(def ? def : symbol);
		link.linkTextStart = beginOfToken;
		link.linkTextEnd = endOfToken;
		return link;*/
	}
}
bool CallGraphAnalyzer::visit(CallAST *ast)
{ 
	qDebug("call ast +++++");
/*
	CPlusPlus::TypeOfExpression toe;
	toe.init(this->_doc,this->_snapshot);
	toe.setExpandTemplates(true);
	const QList<LookupItem> resolvedSymbols = 
		toe.reference(ast,this->_doc,this->_currentScope);

	if (!resolvedSymbols.isEmpty()) 
	{
		LookupItem result = skipForwardDeclarations(resolvedSymbols);

		// find best candidates
		foreach (const LookupItem &r, resolvedSymbols)
		{
			if (Symbol *d = r.declaration()) 
			{ 
				if (d->isDeclaration() || d->isFunction()) 
				{
					const QString fileName = QString::fromUtf8(d->fileName(), d->fileNameLength());
					int line = d->line();
					int col  = d->column();
					result = r;
				}
			}
		}
	
		// get complete declarations, stored in cmpDecl
		QString cmpDecl;
		if (Symbol *d = result.declaration()) 
		{
			Overview overview;
			overview.showArgumentNames = true;
			overview.showReturnTypes = true;
			overview.showFunctionSignatures = true;
			overview.showDefaultArguments = true;
			QString name = overview.prettyName(d->name());
			QString qn;
			// check if the function is a member of class/namespace/enum
			if (d->enclosingScope()->isClass() ||
				d->enclosingScope()->isNamespace() ||
				d->enclosingScope()->isEnum()) 
			{
					qn = overview.prettyName(LookupContext::fullyQualifiedName(d));
					QString n = overview.prettyName(d->enclosingScope()->name());
					n = overview.prettyName(d->enclosingScope()->enclosingScope()->name());
			}
			else
				qn = name;
			cmpDecl = overview.prettyType(d->type(),qn);
		}

		// check declaration symbol
		if (Symbol *symbol = result.declaration()) 
		{
			Symbol *def = 0;

			// Consider to show a pop-up displaying overrides for the function
			Function *function = symbol->type()->asFunctionType();
			
			bool isVirtualOverride = false;

			if (ExpressionAST *baseExpressionAST = ast->base_expression) 
			{
				if (IdExpressionAST *idExpressionAST = baseExpressionAST->asIdExpression()) 
				{
					NameAST *name = idExpressionAST->name;
					isVirtualOverride = name && !name->asQualifiedName();
				} 
				else if (MemberAccessAST *memberAccessAST = baseExpressionAST->asMemberAccess()) 
				{
					NameAST *name = memberAccessAST->member_name;
					const bool nameIsQualified = name && name->asQualifiedName();
					const int tkKind = tokenKind(memberAccessAST->access_token);
					isVirtualOverride = tkKind == T_ARROW && !nameIsQualified;
				}
			}
			if (isVirtualOverride) 
			{
				SymbolFinder symbolFinder;
				Class *klass = symbolFinder.findMatchingClassDeclaration(function, _snapshot);

// 				if (m_virtualFunctionAssistProvider->configure(klass, function, snapshot,
// 					inNextSplit))
// 				{
// 						m_widget->invokeAssist(TextEditor::FollowSymbol,
// 							m_virtualFunctionAssistProvider);
// 				}
// 				return Link();
			}

			//Symbol *lastVisibleSymbol = this->_doc->lastVisibleSymbolAt(line, column);
			//SymbolFinder symbolFinder;

			//def = findDefinition(symbol, this->_snapshot, &symbolFinder);

			//if (def == lastVisibleSymbol)
			//	def = 0; // jump to declaration then.

			//if (symbol->isForwardClassDeclaration())
			//	def = symbolFinder.findMatchingClassDeclaration(symbol, _snapshot);
		}
	}*/
	QList<CGItemInfo> itemList;
	findFunctionCallItemPath(ast, itemList);
	addFunctionCall(itemList);
	m_ithCall++;
	
	qDebug("caller");
	this->callExpression(ast->base_expression);
	// unsigned lparen_token = ast->lparen_token;
	qDebug("param list");
	for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
		this->expression(it->value);
	}
	// unsigned rparen_token = ast->rparen_token;
	m_nthCallLayer--;
	qDebug("call ast -----");
	return false;
}
QList<Symbol*> CodeAtlas::CallGraphAnalyzer::getOverrideSymbols(Class *startClass, Function *function)
{
	QList<Symbol *> result;
	QTC_ASSERT(startClass && function, return result);

	FullySpecifiedType referenceType = function->type();
	const Name *referenceName = function->name();
	QTC_ASSERT(referenceName && referenceType.isValid(), return result);

	// Find class hierarchy
	CppTools::TypeHierarchyBuilder builder(startClass, _snapshot);
	const CppTools::TypeHierarchy &completeHierarchy = builder.buildDerivedTypeHierarchy();

	QSet<Symbol*> classList;							// use set to avoid repeat
	QList<CppTools::TypeHierarchy> q;
	q.append(completeHierarchy);
	while (!q.isEmpty())
	{	// breath first traverse
		CppTools::TypeHierarchy curType = q.takeFirst();
		classList.insert(curType.symbol());				
		q << curType.hierarchy();
	}
	
	// check through classes
	QHash<QString, Symbol*> symbolSet;		// use hash table to avoid duplicate
	const Overview& overview = SymbolInfo::getNamePrinter();
	QString functionName = overview.prettyName(LookupContext::fullyQualifiedName(function));
	symbolSet[functionName] = function;
	for(QSet<Symbol*>::iterator pS = classList.begin(); pS != classList.end(); ++pS)
	{
		Class *c = (*pS)->asClass();
		if (!c)
			continue;
		
		for (int i = 0, total = c->memberCount(); i < total; ++i) {
			Symbol *candidate = c->memberAt(i);
			const Name *candidateName = candidate->name();
			const FullySpecifiedType candidateType = candidate->type();
			if (!candidateName || !candidateType.isValid())
				continue;
#ifdef USING_QT_4
			if (candidateName->isEqualTo(referenceName) && candidateType.isEqualTo(referenceType))
#elif defined(USING_QT_5)
			if (candidateName->match(referenceName) && candidateType.match(referenceType))
#endif
			{
				 QString fullName = overview.prettyName(LookupContext::fullyQualifiedName(candidate));
				 symbolSet[fullName] = candidate;
			}
		}
	}
	result = symbolSet.values();
	return result;
}
bool CodeAtlas::CallGraphAnalyzer::isVirtualFunctionOverrides( Function *function, CallAST* callAST )
{
	if (!function)
		return false;

	bool result = false;
	if (ExpressionAST *baseExpressionAST = callAST->base_expression) {
		if (IdExpressionAST *idExpressionAST = baseExpressionAST->asIdExpression()) {
			NameAST *name = idExpressionAST->name;
			result = name && !name->asQualifiedName();
		} else if (MemberAccessAST *memberAccessAST = baseExpressionAST->asMemberAccess()) {
			NameAST *name = memberAccessAST->member_name;
			const bool nameIsQualified = name && name->asQualifiedName();

			const bool isArrow = (this->tokenAt(memberAccessAST->access_token).kind() == T_ARROW);
			result = isArrow && !nameIsQualified;
		}
	}
	//return result && CppEditor::Internal::FunctionHelper::isVirtualFunction(function, _snapshot);
	
	bool isVirtual = function->isVirtual();

	if (!function->isVirtual())
	{
		const QString filePath = QString::fromUtf8(function->fileName(), function->fileNameLength());
		if (Document::Ptr document = _snapshot.document(filePath)) {
			LookupContext context(document, _snapshot);
			QList<LookupItem> results = context.lookup(function->name(), function->enclosingScope());
			if (!results.isEmpty()) {
				foreach (const LookupItem &item, results) {
					if (Symbol *symbol = item.declaration()) {
						if (Function *functionType = symbol->type()->asFunctionType()) {
							if (functionType == function) // already tested
								continue;
							if (functionType->isFinal())
							{	isVirtual = false; break;}
							if (functionType->isVirtual())
							{	isVirtual = true; break;}
						}
					}
				}
			}
		}
	}

	return result && isVirtual;
}

void CodeAtlas::CallGraphAnalyzer::addFunctionCall(QList<CGItemInfo>& funItems)
{
//	if(!m_elementTree->isItemExist(m_curFunctionPath))
//		return;

	QList<CGItemInfo>& callItemInfo = m_callList[m_curFunctionPath];
	callItemInfo.append(funItems);
// 	for (int ithFun = 0; ithFun < funItems.size(); ++ithFun)
// 	{
// 		if (!m_elementTree->isItemExist(funItems[ithFun]) && false)
// 			continue;
// 
// 		callItemInfo.append(CGItemInfo(funItems[ithFun], ithCall));
// 	}
}

bool CallGraphAnalyzer::visit(ArrayAccessAST *ast)
{
	this->expression(ast->base_expression);
	// unsigned lbracket_token = ast->lbracket_token;
	this->expression(ast->expression);
	// unsigned rbracket_token = ast->rbracket_token;
	return false;
}

bool CallGraphAnalyzer::visit(PostIncrDecrAST *ast)
{
	this->expression(ast->base_expression);
	// unsigned incr_decr_token = ast->incr_decr_token;
	return false;
}

bool CallGraphAnalyzer::visit(MemberAccessAST *ast)
{
	this->expression(ast->base_expression);
	// unsigned access_token = ast->access_token;
	// unsigned template_token = ast->template_token;
	Token tk;
	qDebug("mem access");
	if (ast->member_name) {
		if (SimpleNameAST *simple = ast->member_name->asSimpleName()) {
			tk = tokenAt(simple->identifier_token);
			qDebug("\t%s",tk.spell());
//			if (identifier(simple->identifier_token) == _id) {
//				checkExpression(ast->firstToken(), simple->identifier_token);
//				return false;
//			}
		}
	}

	return false;
}


// CoreDeclaratorAST
bool CallGraphAnalyzer::visit(DeclaratorIdAST *ast)
{
	// unsigned dot_dot_dot_token = ast->dot_dot_dot_token;
	/*const Name *name =*/ this->name(ast->name);
	return false;
}

bool CallGraphAnalyzer::visit(NestedDeclaratorAST *ast)
{
	// unsigned lparen_token = ast->lparen_token;
	this->declarator(ast->declarator);
	// unsigned rparen_token = ast->rparen_token;
	return false;
}


// PostfixDeclaratorAST
bool CallGraphAnalyzer::visit(FunctionDeclaratorAST *ast)
{
	// unsigned lparen_token = ast->lparen_token;
	this->parameterDeclarationClause(ast->parameter_declaration_clause);
	// unsigned rparen_token = ast->rparen_token;
	for (SpecifierListAST *it = ast->cv_qualifier_list; it; it = it->next) {
		this->specifier(it->value);
	}
	this->exceptionSpecification(ast->exception_specification);
	this->trailingReturnType(ast->trailing_return_type);
	this->expression(ast->as_cpp_initializer);
	// Function *symbol = ast->symbol;
	return false;
}

bool CallGraphAnalyzer::visit(ArrayDeclaratorAST *ast)
{
	// unsigned lbracket_token = ast->lbracket_token;
	this->expression(ast->expression);
	// unsigned rbracket_token = ast->rbracket_token;
	return false;
}

void CallGraphAnalyzer::prepareLines(const QByteArray &bytes)
{
	_sourceLineEnds.reserve(1000);
	const char *s = bytes.constData();
	_sourceLineEnds.push_back(s - 1); // we start counting at line 1, so line 0 is always empty.

	for (; *s; ++s)
		if (*s == '\n')
			_sourceLineEnds.push_back(s);
	if (s != _sourceLineEnds.back() + 1) // no newline at the end of the file
		_sourceLineEnds.push_back(s);
}

QString CallGraphAnalyzer::fetchLine(unsigned lineNr) const
{
	Q_ASSERT(lineNr < _sourceLineEnds.size());
	if (lineNr == 0)
		return QString();

	const char *start = _sourceLineEnds.at(lineNr - 1) + 1;
	const char *end = _sourceLineEnds.at(lineNr);
	return QString::fromUtf8(start, end - start);
}


void CodeAtlas::CallGraphData::updateDocCallGraph( const Document::Ptr doc, CallItem& newCallList )
{
	QString fileName = doc->fileName();
	if (!m_fileList.contains(fileName))
		m_fileList[fileName] = FileInfo();

	// get current item set
	FileInfo& fileInfo = m_fileList[fileName];
	QSet<SymbolPath>  curItemSet;
	for (QSet<int>::const_iterator pS = fileInfo.m_itemIndices.begin();
		 pS != fileInfo.m_itemIndices.end(); ++pS)
		curItemSet.insert(m_callGraph[*pS].m_itemPath);

	// get new item set
	QSet<SymbolPath>  newItemSet;
	for (CallItem::const_iterator pS = newCallList.begin();
		pS != newCallList.end(); ++pS)
		newItemSet.insert(pS.key());

	// three types of function definition items 
	// commonItemSet        items exist in both the old and the new set
	// invalidItemSet       items only exist in the old set
	// extraItemSet         items only exist in the new set
	QSet<SymbolPath>  commonItemSet = curItemSet & newItemSet;
	QSet<SymbolPath>  invalidItemSet= curItemSet - newItemSet;
	QSet<SymbolPath>  extraItemSet  = newItemSet - curItemSet;

	for (QSet<SymbolPath>::iterator pS = commonItemSet.begin();
		pS != commonItemSet.end(); ++pS)
	{
		const SymbolPath& commonItemPath = *pS;
		int commonItemIndex = m_callGraphIndex[commonItemPath];
		CallGraphItem& commonItem = m_callGraph[commonItemIndex];

		// remove itself from callee
		for (int ithCall = 0; ithCall < commonItem.m_callingList.size(); ++ithCall)
		{
			CallGraphItem& callee = m_callGraph[commonItem.m_callingList[ithCall].m_dstIndex];
			callee.m_calledList.removeAll(commonItemIndex);
		}

		// clear current call list
		commonItem.m_callingList.clear();

		// build new call connections
		QList<CallItemInfo>& curCallList = newCallList[commonItemPath];
		for (int ithCall = 0; ithCall < curCallList.size(); ++ithCall)
		{
			int calleeIdx;
			CallGraphItem& callee = findOrAddItem(curCallList[ithCall].m_itemPath, calleeIdx, curCallList[ithCall].m_itemFile);
			commonItem.m_callingList.push_back(CallGraphItem::CallGraphItemInfo(calleeIdx,curCallList[ithCall].m_callOrder));
			callee.m_calledList.push_back(commonItemIndex);
		}
	}

	// remove items that no longer exist
	for (QSet<SymbolPath>::iterator pS = invalidItemSet.begin();
		pS != invalidItemSet.end(); ++pS)
	{
		removeItem(*pS, fileName);
	}

	// insert new items
	for (QSet<SymbolPath>::iterator pS = extraItemSet.begin();
		pS != extraItemSet.end(); ++pS)
	{
		const SymbolPath& extraItemPath = *pS;
		int extraItemIndex;
		CallGraphItem& extraItem = findOrAddItem(extraItemPath, extraItemIndex, fileName);
		QList<CallItemInfo> extraCallList = newCallList[extraItemPath];
		for (int ithCall = 0; ithCall < extraCallList.size(); ++ithCall)
		{
			int calleeIdx;
			CallGraphItem& callee = findOrAddItem(extraCallList[ithCall].m_itemPath, calleeIdx, extraCallList[ithCall].m_itemFile);
			extraItem.m_callingList.push_back(CallGraphItem::CallGraphItemInfo(calleeIdx, extraCallList[ithCall].m_callOrder));
			callee.m_calledList.push_back(extraItemIndex);
		}
	}
}

CallGraphData::CallGraphItem& CallGraphData::findOrAddItem( const SymbolPath& item , int & resultIdx, const QString& fileName)
{
	// already exist such item
	if (m_callGraphIndex.contains(item))
	{
		resultIdx = m_callGraphIndex[item];
		return m_callGraph[resultIdx];
	}
	// not exist, but have "hole" to add
	if (m_callGraphHoleID.size() != 0)
	{
		QSet<int>::iterator pB = m_callGraphHoleID.begin();
		int holeID = *pB;
		m_callGraphHoleID.erase(pB);

		m_callGraph[holeID].setVaild();
		m_callGraph[holeID].m_itemPath = item;
		m_callGraph[holeID].m_callingList.clear();
		m_callGraph[holeID].m_calledList.clear();

		m_callGraphIndex[item] = holeID;
		resultIdx = holeID;
		m_fileList[fileName].m_itemIndices.insert(holeID);
		return m_callGraph[holeID];
	}
	resultIdx = m_callGraph.size();
	m_callGraphIndex[item] = resultIdx;
	m_callGraph.push_back(CallGraphItem(item));
	m_fileList[fileName].m_itemIndices.insert(resultIdx);
	return m_callGraph.back();
}

void CodeAtlas::CallGraphData::removeItem(const SymbolPath& item , const QString & fileName)
{
	if (!m_callGraphIndex.contains(item))
		return;

	// remove this item from its caller and callee
	int idx = m_callGraphIndex[item];
	CallGraphItem& curItem = m_callGraph[idx];
	for (int ithCall = 0; ithCall < curItem.m_callingList.size(); ++ithCall)
	{
		CallGraphItem& calleeItem = m_callGraph[curItem.m_callingList[ithCall].m_dstIndex];
		calleeItem.m_calledList.removeAll(idx);
	}
	for (int ithCall = 0; ithCall < curItem.m_calledList.size(); ++ithCall)
	{
		CallGraphItem& callerItem = m_callGraph[curItem.m_calledList[ithCall]];
		callerItem.m_callingList.removeAll(CallGraphItem::CallGraphItemInfo(idx,0));
	}

	curItem.setInvalid();

	m_callGraphHoleID.insert(idx);
	m_callGraphIndex.remove(item);
	m_fileList[fileName].m_itemIndices.remove(idx);
}

void CodeAtlas::CallGraphData::printCallGraph()
{
	qDebug() << "\n-----------------Call  Graph-----------------\n";
	for (int ithItem = 0; ithItem < m_callGraph.size(); ++ithItem)
	{
		CallGraphItem& cgItem = m_callGraph[ithItem];
		if (!cgItem.isValid())
			continue;
		qDebug() << cgItem.m_itemPath.toString();
		for (int ithCallee = 0; ithCallee < cgItem.m_callingList.size(); ++ithCallee)
		{
			int calleeIdx = cgItem.m_callingList[ithCallee].m_dstIndex;
			qDebug() << "\t" << cgItem.m_callingList[ithCallee].m_callOrder
				     << "\t" << m_callGraph[calleeIdx].m_itemPath.toString();
		}
	}
	qDebug() << "\n---------------------------------------------\n";
}

void CodeAtlas::CallGraphData::clear()
{
	m_fileList.clear();
	m_callGraph.clear();
	m_callGraphIndex.clear();
	m_callGraphHoleID.clear();
}

CodeAtlas::CallGraphData::CallGraphItem::CallGraphItemInfo::CallGraphItemInfo( int dstIdx, int callOrder )
{
	m_dstIndex = dstIdx; m_callOrder = callOrder;
}

bool CodeAtlas::CallGraphData::CallGraphItem::CallGraphItemInfo::operator==( const CallGraphItemInfo& info )const 
{
	return m_dstIndex == info.m_dstIndex;
}

void CodeAtlas::CallGraphData::CallGraphItem::setInvalid()
{
	m_itemPath.clear();
	m_callingList.clear();
	m_calledList.clear();
	m_isValid = false;
}
