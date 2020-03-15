/*******************************************************************************
 * Copyright (C) 2020 by Steve Allewell                                        *
 * steve.allewell@gmail.com                                                    *
 *                                                                             *
 * This program is free software; you can redistribute it and/or modify        *
 * it under the terms of the GNU General Public License as published by        *
 * the Free Software Foundation; either version 2 of the License, or           *
 * (at your option) any later version.                                         *
 ******************************************************************************/


#ifndef MainWindow_H
#define MainWindow_H


#include <QUrl>

#include <KXmlGuiWindow>


class QTabWidget;


class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow() = default;

public slots:
    void fileOpen(const QUrl &url);

protected slots:
    void fileOpen();
    void filePrint();
    void fileExport();
    void quit();

private slots:
    void currentChanged(int);
    void closeRequested(int);

private:
    QTabWidget  *m_tabWidget;
};


#endif

