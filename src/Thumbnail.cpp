/*******************************************************************************
 * Copyright (C) 2020 by Steve Allewell                                        *
 * steve.allewell@gmail.com                                                    *
 *                                                                             *
 * This program is free software; you can redistribute it and/or modify        *
 * it under the terms of the GNU General Public License as published by        *
 * the Free Software Foundation; either version 2 of the License, or           *
 * (at your option) any later version.                                         *
 ******************************************************************************/


#include <QPainter>

#include "Thumbnail.h"


Thumbnail::Thumbnail(Poppler::Page *pdfPage, QListWidget *parent)
    :   QListWidgetItem(parent, QListWidgetItem::UserType),
        m_pdfPage(pdfPage)
{
}


Thumbnail::~Thumbnail()
{
    delete m_pdfPage;
}


QImage Thumbnail::image() const
{
    return m_image;
}


int Thumbnail::heightForWidth(int width) const
{
    QSize pageSize = m_pdfPage->pageSize();

    return (int)((double)width * pageSize.height() / pageSize.width());
}


void Thumbnail::renderPage()
{
    QSize pageSize = m_pdfPage->pageSize();
    QSize iconSize = listWidget()->iconSize();

    double dpi = (double)(iconSize.width()) * 72 / pageSize.width();
    m_image = m_pdfPage->renderToImage(dpi, dpi);

    QPainter p(&m_image);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.drawRect(m_image.rect());
    p.end();

    setImage(m_image);
}


void Thumbnail::setImage(const QImage &image)
{
    setIcon(QPixmap::fromImage(image));
}
