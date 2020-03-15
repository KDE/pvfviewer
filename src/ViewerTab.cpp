/*******************************************************************************
 * Copyright (C) 2020 by Steve Allewell                                        *
 * steve.allewell@gmail.com                                                    *
 *                                                                             *
 * This program is free software; you can redistribute it and/or modify        *
 * it under the terms of the GNU General Public License as published by        *
 * the Free Software Foundation; either version 2 of the License, or           *
 * (at your option) any later version.                                         *
 ******************************************************************************/


#include "ViewerTab.h"

#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPrinter>
#include <QPrintDialog>
#include <QProgressBar>
#include <QResizeEvent>
#include <QUrl>

#include <QDebug>

#include <KLocalizedString>
#include <KMessageBox>

#include "Thumbnail.h"


QByteArray readData(QDataStream &stream, int size)
{
    std::unique_ptr<char[]> buffer(new char[size]);
    stream.readRawData(buffer.get(), size);
    return QByteArray(buffer.get(), size);
}


QByteArray readData(QDataStream &stream)
{
    int size;
    stream >> size;
    return readData(stream, size);
}


QString readString(QDataStream &stream)
{
    quint16 size;
    stream >> size;
    return QString(readData(stream, size));
}


ViewerTab::ViewerTab(QWidget *parent)
    :   QWidget(parent),
        m_document(nullptr)
{
    ui.setupUi(this);

    connect(ui.splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(splitterMoved(int, int)));
}


ViewerTab::~ViewerTab()
{
    delete m_document;
}


int ViewerTab::load(const QUrl &url)
{
    QFile src(url.toLocalFile());

    if (!src.open(QIODevice::ReadOnly))
    {
        KMessageBox::sorry(nullptr, i18n("Failed to open source file."));
        return -1;
    }

    QDataStream stream(&src);
    stream.setByteOrder(QDataStream::LittleEndian);

    if (!readData(stream, 24).contains("Pattern Viewer Data File")) {
        KMessageBox::sorry(nullptr, i18n("Not a PC Stitch Pattern Viewer file."));
        return -1;
    }

    stream.skipRawData(12);     // some random data here, TODO determine contents
                                // possibly a file format version

    ui.LicenceLabel->setText(QString(readData(stream, 30)));

    stream.skipRawData(23);     // some random data here, TODO determine contents

    ui.DesignerLabel->setText(readString(stream));

    QString copyright(readString(stream));
    if (copyright.at(0).unicode() == 65533)
        copyright.replace(0, 1, QChar(0x00A9));
    ui.CopyrightLabel->setText(copyright);


    m_tabTitle = readString(stream);

    readString(stream);         // there is another length encoded string here
                                // but no content in any of the files checked

    m_website = readString(stream);

    m_pixmap.loadFromData(readData(stream));

    m_pdfData = readData(stream);

    m_document = Poppler::Document::loadFromData(m_pdfData);

    if (!m_document || m_document->isLocked()) {
        KMessageBox::sorry(nullptr, i18n("Failed to create PDF from data."));
        delete m_document;
        m_document = 0;
        return -1;
    }

    int pages = m_document->numPages();

    for (int page = 0 ; page < pages ; ++page) {
        Poppler::Page *pdfPage = m_document->page(page);

        if (pdfPage == 0) {
            KMessageBox::sorry(nullptr, i18n("Failed to read an expected page."));
            continue;
        }

        QImage image = pdfPage->renderToImage(150, 150);

        if (image.isNull()) {
            KMessageBox::sorry(nullptr, i18n("Failed to create image for page."));
        }

        QPainter p(&image);
        p.drawRect(0,0,image.width()-1,image.height()-1);
        p.end();

        ui.PagesList->addItem(new Thumbnail(image));

        delete pdfPage;
    }

    return 0;
}


QString ViewerTab::title() const
{
    return m_tabTitle;
}


QByteArray ViewerTab::pdfData()
{
    return m_pdfData;
}


void ViewerTab::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    int pagesWidth = ui.PagesList->viewport()->width() - (ui.PagesList->spacing() * 2);
    ui.PagesList->setIconSize(QSize(pagesWidth, pagesWidth*2));

    ui.PatternPreview->setPixmap(m_pixmap.scaled(ui.PatternPreview->width()-20, ui.PatternPreview->height()-20, Qt::KeepAspectRatio));
}


void ViewerTab::splitterMoved(int pos, int index)
{
    Q_UNUSED(pos);
    Q_UNUSED(index);

    // recalculate the sizes of the image and the pdf pages
    int pagesWidth = ui.PagesList->viewport()->width() - (ui.PagesList->spacing() * 2);
    ui.PagesList->setIconSize(QSize(pagesWidth, pagesWidth*2));

    ui.PatternPreview->setPixmap(m_pixmap.scaled(ui.PatternPreview->width()-20, ui.PatternPreview->height()-20, Qt::KeepAspectRatio));
}


void ViewerTab::print()
{
    QPrinter printer;

    QList<Poppler::Page *> pages;

    int pageCount = m_document->numPages();

    for (int page = 0 ; page < pageCount ; ++page) {
        pages.append(m_document->page(page));
    }

    printer.setFullPage(true);
    printer.setPrintRange(QPrinter::AllPages);
    printer.setFromTo(1, pageCount);

    QPrintDialog *printDialog = new QPrintDialog(&printer, this);

    if (printDialog->exec() == QDialog::Accepted) {
        int fromPage = 1;
        int toPage   = pageCount;

        if (printer.printRange() == QPrinter::PageRange) {
            fromPage = printer.fromPage();
            toPage   = printer.toPage();
        }

        while (toPage < pages.count()) pages.removeLast();
        while (--fromPage) pages.removeFirst();

        int pagesCount = pages.count();


        QPainter painter;
        painter.begin(&printer);

        for (int page = 0 ; page < pagesCount ; ++page) {
            if (page > 0) {
                printer.newPage();
            }

            Poppler::Page *pdfPage = (printer.pageOrder() == QPrinter::FirstPageFirst)?pages.takeFirst():pages.takeLast();

            if (pdfPage) {
                QSizeF pageSize  = pdfPage->pageSizeF(); // size in points 1/72 inch
                QRect  paintRect = painter.window();

                double scale = printer.paperRect(QPrinter::DevicePixel).width() / printer.paperRect(QPrinter::Point).width();

                double hScale = paintRect.width()  / pageSize.width();
                double vScale = paintRect.height() / pageSize.height();

                scale = std::min(hScale, vScale);

                QImage image = pdfPage->renderToImage(300, 300);

                painter.drawImage(QRectF(QPointF(0, 0), scale * pdfPage->pageSizeF()), image);
            }
        }

        painter.end();
    }

    delete printDialog;
}
