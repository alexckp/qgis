/***************************************************************************
                          qgspggeoprocessing.h 
 Geoprocessing plugin for PostgreSQL/PostGIS layers
 Functions:
   Buffer
                             -------------------
    begin                : Jan 21, 2004
    copyright            : (C) 2004 by Gary E.Sherman
    email                : sherman at mrcc.com
  
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 /*  $Id$ */
#ifndef QGISQgsPgGeoprocessing_H
#define QGISQgsPgGeoprocessing_H
#include "../qgisplugin.h"
#include <qwidget.h>
#include <qmainwindow.h>

class QMessageBox;
class QToolBar;
class QMenuBar;
class QPopupMenu;
//#include "qgsworkerclass.h"
#include "../../src/qgisapp.h"

/**
* \class QgsPgGeoprocessing
* \brief PostgreSQL/PostGIS plugin for QGIS
*
*/
class QgsPgGeoprocessing : public QObject, public QgisPlugin{
Q_OBJECT
public:
/** 
* Constructor for a plugin. The QgisApp and QgisIface pointers are passed by 
* QGIS when it attempts to instantiate the plugin.
* @param qgis Pointer to the QgisApp object
* @param qI Pointer to the QgisIface object. 
*/
	QgsPgGeoprocessing(QgisApp *qgis, QgisIface *qI);
	/**
	* Virtual function to return the name of the plugin. The name will be used when presenting a list 
	* of installable plugins to the user
	*/
	virtual QString name();
	/**
	* Virtual function to return the version of the plugin. 
	*/
	virtual QString version();
	/**
	* Virtual function to return a description of the plugins functions 
	*/
	virtual QString description();
	/**
  * Return the plugin type
  */
  virtual int type();
  //! init the gui
  virtual void initGui();
  //! Destructor
	virtual ~QgsPgGeoprocessing();
public slots:
  //! buffer features in a layer
	void buffer();
  //! unload the plugin
  void unload();
private:
//! Name of the plugin
	QString pName;
  //! Version
	QString pVersion;
  //! Descrption of the plugin
	QString pDescription;
  //! Plugin type as defined in QgisPlugin::PLUGINTYPE
	int ptype;
  //! Id of the plugin's menu. Used for unloading
  int menuId;
  //! Pointer to our toolbar
  QToolBar *toolBar;
  //! Pointer to our menu
  QMenuBar *menu; 
  //! Pionter to QGIS main application object
	QgisApp *qgisMainWindow;
  //! Pointer to the QGIS interface object
	QgisIface *qI;
};

#endif
