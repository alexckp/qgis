/***************************************************************************
                          qgslayerproperties.cpp  -  description
                             -------------------
    begin                : Sun Aug 11 2002
    copyright            : (C) 2002 by Gary E.Sherman
    email                : sherman at mrcc dot com
       Romans 3:23=>Romans 6:23=>Romans 5:8=>Romans 10:9,10=>Romans 12
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <qframe.h>
#include <qcolordialog.h>
#include <qpushbutton.h>
#include "qgsmaplayer.h"
#include "qgssymbol.h"
#include "qgslayerproperties.h"

QgsLayerProperties::QgsLayerProperties(QgsMapLayer * lyr):layer(lyr)
{
	// populate the property sheet based on the layer properties
	sym = layer->symbol();

	btnSetColor->setPaletteBackgroundColor(sym->color());
	
	btnSetFillColor->setPaletteBackgroundColor(sym->fillColor());
	setCaption("Layer Properties - " + lyr->name());
}

QgsLayerProperties::~QgsLayerProperties()
{
}
void QgsLayerProperties::selectFillColor()
{

	QColor fc = QColorDialog::getColor(sym->fillColor(), this);
	if (fc.isValid()) {
		
		btnSetFillColor->setPaletteBackgroundColor(fc);
		sym->setFillColor(fc);
	}
}
void QgsLayerProperties::selectOutlineColor()
{
	QColor oc = QColorDialog::getColor(sym->color(), this);
	if (oc.isValid()) {
		
		btnSetColor->setPaletteBackgroundColor(oc);
		sym->setColor(oc);
	}
}
