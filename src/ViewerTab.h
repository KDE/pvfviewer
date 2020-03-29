/*******************************************************************************
 * Copyright (C) 2020 by Steve Allewell                                        *
 * steve.allewell@gmail.com                                                    *
 *                                                                             *
 * This program is free software; you can redistribute it and/or modify        *
 * it under the terms of the GNU General Public License as published by        *
 * the Free Software Foundation; either version 2 of the License, or           *
 * (at your option) any later version.                                         *
 ******************************************************************************/


#ifndef ViewerTab_H
#define ViewerTab_H


#include <QPixmap>
#include <QString>
#include <QTimer>
#include <QWidget>

#include <poppler-qt5.h>

#include "ui_ViewerTab.h"


class QResizeEvent;
class QShowEvent;
class QUrl;


class ViewerTab : public QWidget
{
    Q_OBJECT

public:
    explicit ViewerTab(QWidget *parent);
    ~ViewerTab();

    int load(const QUrl &url);

    QString title() const;

    QByteArray pdfData();

    void print();

protected slots:
    void renderPages();
    void splitterMoved(int pos, int index);
    void valueChanged(int value);

protected:
    void quickScale();
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

private:
    Ui::ViewerTab   ui;

    Poppler::Document *m_document;

    QByteArray      m_pdfData;
    QPixmap         m_pixmap;
    QString         m_tabTitle;
    QString         m_website;

    QTimer          m_timer;
};


#endif
