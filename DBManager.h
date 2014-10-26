/**************************************************************************
**
** Copyright (c) 2013 Denis Mingulov
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

namespace CodeAtlas {



class DBManager : public QObject
{
	Q_OBJECT

public:
	explicit DBManager(QObject *parent = 0);

	virtual ~DBManager();

	//! Get an instance of Manager
	static DBManager *instance();

	bool canFetchMore(QStandardItem *item) const;

	void fetchMore(QStandardItem *item, bool skipRoot = false);
	
	void setEnable(bool isEnable);

	CodeAtlasDB& getDB(){return d->db;}

signals:
	void stateChanged(bool state);

	void treeDataUpdate(QSharedPointer<QStandardItem> result);

	void requestTreeDataUpdate();

	void requestDocumentUpdated(CPlusPlus::Document::Ptr doc);

	void requestResetCurrentState();

	void requestClearCache();

	void requestClearCacheAll();

	void requestSetFlatMode(bool flat);

signals:
	// my signals
	void dbUpdated();	// db updated, now request UIManager to update scene objects
public slots:
	void gotoLocation(const QString &fileName, int line = 0, int column = 0);
	void gotoLocations(const QList<QVariant> &locations);
	void gotoLocations(const QSet<SymbolLocation>& locations);

	void onRequestTreeDataUpdate();
	void setFlatMode(bool flat);
	void connectCursorUpdateSignal(Core::IEditor *editor, const QString &fileName);

protected slots:
	void onWidgetIsCreated();
	void onWidgetVisibilityIsChanged(bool visibility);
	void onStateChanged(bool state);
	void onProjectListChanged();
	void onDocumentUpdated(CPlusPlus::Document::Ptr doc);
	void onTaskStarted(Core::Id type);
	void onAllTasksFinished(Core::Id type);
	void onTreeDataUpdate(QSharedPointer<QStandardItem> result);


protected:
	//! Perform an initialization
	void initialize();

	inline bool state() const;
	void setState(bool state);


private:
		/*!
   \class ManagerPrivate
   \internal
   \brief The ManagerPrivate class contains private class data for the Manager
    class.
   \sa Manager
*/

	class ManagerPrivate
	{
	public:
		ManagerPrivate() : state(true), disableCodeParser(false){}

		//! State mutex
		QMutex mutexState;

		//! code state/changes parser
		CodeAtlasDB db;

		//! separate thread for the parser
		QThread dbThread;

		//! Internal manager state. \sa Manager::state
		bool state;

		//! there is some massive operation ongoing so temporary we should wait
		bool disableCodeParser;

	};
	//! private class data pointer
	ManagerPrivate *d;
};

}