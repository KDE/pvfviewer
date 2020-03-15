/*******************************************************************************
 * Copyright (C) 2020 by Steve Allewell                                        *
 * steve.allewell@gmail.com                                                    *
 *                                                                             *
 * This program is free software; you can redistribute it and/or modify        *
 * it under the terms of the GNU General Public License as published by        *
 * the Free Software Foundation; either version 2 of the License, or           *
 * (at your option) any later version.                                         *
 ******************************************************************************/


#ifndef Thumbnail_H
#define Thumbnail_H


#include <QListWidgetItem>


class QImage;


class Thumbnail : public QListWidgetItem
{
public:
    Thumbnail(QImage image);
    ~Thumbnail() = default;
};


#endif
