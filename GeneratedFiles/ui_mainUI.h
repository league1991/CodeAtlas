/********************************************************************************
** Form generated from reading UI file 'mainUI.ui'
**
** Created by: Qt User Interface Compiler version 5.3.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINUI_H
#define UI_MAINUI_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <codeview.h>

QT_BEGIN_NAMESPACE

class Ui_MainUI
{
public:
    QAction *actionUpdate;
    QAction *actionActivate;
    CodeAtlas::CodeView *centralwidget;
    QMenuBar *menubar;
    QStatusBar *statusbar;
    QToolBar *toolBar;

    void setupUi(QMainWindow *MainUI)
    {
        if (MainUI->objectName().isEmpty())
            MainUI->setObjectName(QStringLiteral("MainUI"));
        MainUI->resize(660, 436);
        actionUpdate = new QAction(MainUI);
        actionUpdate->setObjectName(QStringLiteral("actionUpdate"));
        actionActivate = new QAction(MainUI);
        actionActivate->setObjectName(QStringLiteral("actionActivate"));
        actionActivate->setCheckable(true);
        centralwidget = new CodeAtlas::CodeView(MainUI);
        centralwidget->setObjectName(QStringLiteral("centralwidget"));
        MainUI->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainUI);
        menubar->setObjectName(QStringLiteral("menubar"));
        menubar->setGeometry(QRect(0, 0, 660, 20));
        MainUI->setMenuBar(menubar);
        statusbar = new QStatusBar(MainUI);
        statusbar->setObjectName(QStringLiteral("statusbar"));
        MainUI->setStatusBar(statusbar);
        toolBar = new QToolBar(MainUI);
        toolBar->setObjectName(QStringLiteral("toolBar"));
        MainUI->addToolBar(Qt::TopToolBarArea, toolBar);

        toolBar->addAction(actionUpdate);
        toolBar->addAction(actionActivate);

        retranslateUi(MainUI);

        QMetaObject::connectSlotsByName(MainUI);
    } // setupUi

    void retranslateUi(QMainWindow *MainUI)
    {
        MainUI->setWindowTitle(QApplication::translate("MainUI", "CodeAtlas", 0));
        actionUpdate->setText(QApplication::translate("MainUI", "Update", 0));
        actionActivate->setText(QApplication::translate("MainUI", "Activate", 0));
        toolBar->setWindowTitle(QApplication::translate("MainUI", "toolBar", 0));
    } // retranslateUi

};

namespace Ui {
    class MainUI: public Ui_MainUI {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINUI_H
