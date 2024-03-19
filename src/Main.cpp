/*******************************************************************************
 * Copyright (C) 2020 by Steve Allewell                                        *
 * steve.allewell@gmail.com                                                    *
 *                                                                             *
 * This program is free software; you can redistribute it and/or modify        *
 * it under the terms of the GNU General Public License as published by        *
 * the Free Software Foundation; either version 2 of the License, or           *
 * (at your option) any later version.                                         *
 ******************************************************************************/


#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QUrl>

#include <KAboutData>
#include <KLocalizedString>

#include "MainWindow.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("pvfViewer");

    KAboutData aboutData(QStringLiteral("pvfViewer"),                           // component name
                         QString(i18n("pvfViewer")),                            // display name
                         QStringLiteral("0.1.0"),                               // version
                         i18n("A PC Stitch pvf pattern viewer."),               // short description
                         KAboutLicense::GPL_V2,                                 // license
                         i18n("(c)2020 Steve Allewell"),                        // copyright
                         QString(),                                             // other text
                         QStringLiteral("https://userbase.kde.org/pvfViewer")   // home page
                         // bug address defaults to submit@bugs.kde.org
               );

    aboutData.addAuthor(QStringLiteral("Steve Allewell"), i18n("Project Lead"), QStringLiteral("steve.allewell@gmail.com"));
    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    KAboutData::setApplicationData(aboutData);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("pvfViewer")));

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);

    parser.process(app);
    aboutData.processCommandLine(&parser);

    MainWindow *mainWindow = new MainWindow();
    mainWindow->show();

    for (int i = 0 ; i < parser.positionalArguments().count() ; ++i) {
        mainWindow->fileOpen(QUrl::fromUserInput(parser.positionalArguments().at(i), QDir::currentPath()));
    }

    return app.exec();
}
