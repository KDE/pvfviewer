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
#include <QListWidget>
#include <QPainter>
#include <QPointer>
#include <QPrinter>
#include <QPrintDialog>
#include <QProgressDialog>
#include <QResizeEvent>
#include <QScrollBar>
#include <QShowEvent>
#include <QUrl>

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

    QScrollBar *scrollbar = ui.PagesList->verticalScrollBar();

    connect(ui.splitter, SIGNAL(splitterMoved(int,int)), this, SLOT(splitterMoved(int,int)));
    connect(scrollbar, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(renderPages()));
}


void ViewerTab::valueChanged(int value)
{
    Q_UNUSED(value);

    if (isVisible()) m_timer.start(100);
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
        KMessageBox::error(nullptr, i18n("Failed to open source file."));
        return -1;
    }

    QDataStream stream(&src);
    stream.setByteOrder(QDataStream::LittleEndian);

    if (!readData(stream, 24).contains("Pattern Viewer Data File")) {
        KMessageBox::error(nullptr, i18n("Not a PC Stitch Pattern Viewer file."));
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
        KMessageBox::error(nullptr, i18n("Failed to create PDF from data."));
        delete m_document;
        m_document = 0;
        return -1;
    }

    m_document->setRenderHint(Poppler::Document::RenderHint::Antialiasing, true);
    m_document->setRenderHint(Poppler::Document::RenderHint::TextAntialiasing, true);
    m_document->setRenderHint(Poppler::Document::RenderHint::TextHinting, true);
    m_document->setRenderHint(Poppler::Document::RenderHint::TextSlightHinting, true);
    m_document->setRenderHint(Poppler::Document::RenderHint::ThinLineSolid, true);

    int pages = m_document->numPages();

    for (int page = 0 ; page < pages ; ++page) {
        new Thumbnail(m_document->page(page), ui.PagesList);
    }

    return 0;
}


void ViewerTab::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    quickScale();
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

    if (isVisible()) quickScale();
}


void ViewerTab::splitterMoved(int pos, int index)
{
    Q_UNUSED(pos);
    Q_UNUSED(index);

    quickScale();
}


void ViewerTab::quickScale()
{
    int newIconWidth = ui.PagesList->viewport()->width() - (2 * ui.PagesList->spacing()) - ui.PagesList->contentsMargins().left() - ui.PagesList->contentsMargins().right();

    ui.PatternPreview->setPixmap(m_pixmap.scaled(ui.PatternPreview->width()-20, ui.PatternPreview->height()-20, Qt::KeepAspectRatio));

    ui.PagesList->setIconSize(QSize(newIconWidth, static_cast<Thumbnail *>(ui.PagesList->item(0))->heightForWidth(newIconWidth)));

    for (int page = 0, pages = m_document->numPages() ; page < pages ; ++page) {
        Thumbnail *thumbnail = static_cast<Thumbnail *>(ui.PagesList->item(page));
        QImage image = thumbnail->image();

        if (image.isNull()) {
            image = QImage(ui.PagesList->iconSize(), QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::white);
            thumbnail->setImage(image);
        } else {
            thumbnail->setImage(image.scaledToWidth(newIconWidth, Qt::FastTransformation));
        }
    }

    m_timer.start(100);
}


void ViewerTab::renderPages()
{
    m_timer.stop();

    QScrollBar *scrollBar = ui.PagesList->verticalScrollBar();

    int numPages = m_document->numPages();
    int pageStep = scrollBar->pageStep();
    int pageLength = (scrollBar->maximum() + pageStep) / numPages;
    int value = scrollBar->value();
    int firstPage = value / pageLength;
    int lastPage = std::min(((value + pageStep) / pageLength), numPages - 1);

    for (int page = firstPage ; page <= lastPage ; ++page) {
        Thumbnail *thumbnail = static_cast<Thumbnail *>(ui.PagesList->item(page));
        thumbnail->renderPage();
    }
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

    QPointer<QPrintDialog> printDialog = new QPrintDialog(&printer, this);

    if (printDialog->exec() == QDialog::Accepted) {
        int fromPage = 1;
        int toPage   = pageCount;

        if (printer.printRange() == QPrinter::PageRange) {
            fromPage = printer.fromPage();
            toPage   = printer.toPage();
        }

        while (toPage < pages.count()) pages.removeLast();
        while (--fromPage) pages.removeFirst();

        int pageCount = pages.count();

        QProgressDialog progress(this);
        progress.setWindowModality(Qt::WindowModal);
        progress.setRange(0, pageCount);

        QPainter painter;
        painter.begin(&printer);

        for (int page = 0 ; page < pageCount ; ++page) {
            if (progress.wasCanceled()) {
                printer.abort();
                return;
            }

            if (page > 0) {
                printer.newPage();
            }

            progress.setValue(page);
            progress.setLabelText(i18n("Printing page %1", page+1));

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

        progress.setValue(pageCount);
    }

    delete printDialog;
}
