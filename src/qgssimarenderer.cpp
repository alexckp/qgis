/***************************************************************************
                          qgssimarenderer.cpp 
 Single marker renderer
                             -------------------
    begin                : March 2004
    copyright            : (C) 2004 by Marco Hugentobler
    email                : mhugent@geo.unizh.ch
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 /* $Id$ */

#include "qgssimarenderer.h"
#include "qgsmarkersymbol.h"
#include <qpainter.h>

void QgsSiMaRenderer::initializeSymbology(QgsVectorLayer* layer, QgsDlgVectorLayerProperties*)
{
    
}

void QgsSiMaRenderer::renderFeature(QPainter* p, QgsFeature* f, QPicture* pic, double* scalefactor)
{
    p->setPen(mItem.getSymbol()->pen());
    p->setBrush(mItem.getSymbol()->brush());

    QgsMarkerSymbol* ms=(QgsMarkerSymbol*)mItem.getSymbol();
    if(ms)
    {
	pic=ms->picture();
	(*scalefactor)=ms->scaleFactor();
    }
}
