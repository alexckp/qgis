/***************************************************************************
                          qgsclipper.cpp  -  a class that clips line
                          segments and polygons
                             -------------------
    begin                : March 2004
    copyright            : (C) 2005 by Gavin Macaulay
    email                :
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

#include "qgsclipper.h"

// Where has all the code gone?

// It's been inlined, so is in the qgsclipper.h file.

// But the static members must be initialised outside the class! (or GCC 4 dies)

const double QgsClipper::maxX =  30000;
const double QgsClipper::minX = -30000;
const double QgsClipper::maxY =  30000;
const double QgsClipper::minY = -30000;

const double QgsClipper::SMALL_NUM = 1e-12;






