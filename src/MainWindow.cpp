/*******************************************************************************
 * Copyright (C) 2020 by Steve Allewell                                        *
 * steve.allewell@gmail.com                                                    *
 *                                                                             *
 * This program is free software; you can redistribute it and/or modify        *
 * it under the terms of the GNU General Public License as published by        *
 * the Free Software Foundation; either version 2 of the License, or           *
 * (at your option) any later version.                                         *
 ******************************************************************************/


#include "MainWindow.h"

#include <QFileDialog>
#include <QTabWidget>

#include <KActionCollection>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KRecentFilesAction>

#include <poppler-qt5.h>

#include "ViewerTab.h"


MainWindow::MainWindow()
    :   m_tabWidget(new QTabWidget(this))
{
    setObjectName(QStringLiteral("MainWindow#"));

    m_tabWidget->setTabsClosable(true);

    KActionCollection *actions = actionCollection();

    setCentralWidget(m_tabWidget);

    KStandardAction::open(this, SLOT(fileOpen()), actions);
    KStandardAction::openRecent(this, SLOT(fileOpen(QUrl)), actions)->loadEntries(KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("RecentFiles")));

    QAction *action = new QAction(i18n("Export PDF..."), this);
    connect(action, SIGNAL(triggered()), this, SLOT(fileExport()));
    actions->addAction(QStringLiteral("fileExportPDF"), action);
    action->setEnabled(false); // initially disabled when no tabs present

    action = KStandardAction::print(this, SLOT(filePrint()), actions);
    action->setEnabled(false); // initially disabled when no tabs present

    KStandardAction::quit(this, SLOT(quit()), actions);

    setupGUI(KXmlGuiWindow::Default, QStringLiteral("pvfViewerui.rc"));

    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(currentChanged(int)));
    connect(m_tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeRequested(int)));
}


void MainWindow::fileOpen()
{
    QList<QUrl> urls = QFileDialog::getOpenFileUrls(this, i18n("Open file"), QUrl::fromLocalFile(QDir::homePath()), i18n("Pattern Viewer File (*.pvf)"));

    for (QUrl url : urls) {
        fileOpen(url);
    }
}


void MainWindow::fileOpen(const QUrl &url)
{
    if (!url.isValid())
        return;

    ViewerTab *viewerTab = new ViewerTab(m_tabWidget);

    if (viewerTab->load(url) == 0) {
        KRecentFilesAction *action = static_cast<KRecentFilesAction *>(actionCollection()->action(QStringLiteral("file_open_recent")));
        action->addUrl(url);
        action->saveEntries(KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("RecentFiles")));

        m_tabWidget->setCurrentIndex(m_tabWidget->addTab(viewerTab, viewerTab->title()));
    } else {
        delete viewerTab;
    }
}


void MainWindow::filePrint()
{
    static_cast<ViewerTab *>(m_tabWidget->currentWidget())->print();  // filePrint will not be enabled if there are no tabs
}


void MainWindow::fileExport()
{
    if (ViewerTab *viewerTab = static_cast<ViewerTab*>(m_tabWidget->currentWidget())) {
        QUrl url = QFileDialog::getSaveFileUrl(this, i18n("Export As..."),
            QUrl::fromLocalFile(QDir::homePath()), i18n("PDF File (*.pdf)"));

        if (url.isValid()) {
            QFile file(url.toLocalFile());
            file.open(QIODevice::WriteOnly);

            QDataStream stream(&file);

            QByteArray pdfData = viewerTab->pdfData();
            stream.writeRawData(pdfData.constData(), pdfData.size());

            file.close();
        }
    }
}


void MainWindow::quit()
{
    KXmlGuiWindow::close();
}


void MainWindow::currentChanged(int index)
{
    Q_UNUSED(index);

    if (index == -1) {
        // no more tabs, disable print and export
        actionCollection()->action("file_print")->setEnabled(false);
        actionCollection()->action("fileExportPDF")->setEnabled(false);
    } else {
        actionCollection()->action("file_print")->setEnabled(true);
        actionCollection()->action("fileExportPDF")->setEnabled(true);
    }
    if (ViewerTab *viewerTab = static_cast<ViewerTab *>(m_tabWidget->currentWidget())) {
        viewerTab->splitterMoved(0, 0);
    }
}


void MainWindow::closeRequested(int index)
{
    QWidget *viewerTab = m_tabWidget->widget(index);

    m_tabWidget->removeTab(index);
    delete viewerTab;
}
