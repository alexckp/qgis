/***************************************************************************
    globe_plugin.cpp

    Globe Plugin
    a QGIS plugin
     --------------------------------------
    Date                 : 08-Jul-2010
    Copyright            : (C) 2010 by Sourcepole
    Email                : info at sourcepole.ch
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "globe_plugin.h"
#include "globe_plugin_gui.h"

#include <qgisinterface.h>
#include <qgisgui.h>

#include <QAction>
#include <QToolBar>

#include <osgGA/TrackballManipulator>
#include <osgDB/ReadFile>

#include <osg/Notify>
#include <osgGA/StateSetManipulator>
#include <osgGA/GUIEventHandler>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgEarth/Map>
#include <osgEarth/MapNode>
#include <osgEarthUtil/EarthManipulator>
#include <osgEarthUtil/Viewpoint>

// some preset viewpoints.
static osgEarthUtil::Viewpoint VPs[] = {
    osgEarthUtil::Viewpoint( "Africa",        osg::Vec3d(    0.0,   0.0, 0.0 ), 0.0, -90.0, 10e6 ),
    osgEarthUtil::Viewpoint( "California",    osg::Vec3d( -121.0,  34.0, 0.0 ), 0.0, -90.0, 6e6 ),
    osgEarthUtil::Viewpoint( "Europe",        osg::Vec3d(    0.0,  45.0, 0.0 ), 0.0, -90.0, 4e6 ),
    osgEarthUtil::Viewpoint( "Washington DC", osg::Vec3d(  -77.0,  38.0, 0.0 ), 0.0, -90.0, 1e6 ),
    osgEarthUtil::Viewpoint( "Australia",     osg::Vec3d(  135.0, -20.0, 0.0 ), 0.0, -90.0, 2e6 )
};

// a simple handler that demonstrates the "viewpoint" functionality in 
// osgEarthUtil::EarthManipulator. Press a number key to fly to a viewpoint.
struct FlyToViewpointHandler : public osgGA::GUIEventHandler 
{
    FlyToViewpointHandler( osgEarthUtil::EarthManipulator* manip ) : _manip(manip) { }

    bool handle( const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa )
    {
        if ( ea.getEventType() == ea.KEYDOWN && ea.getKey() >= '1' && ea.getKey() <= '5' )
        {
            _manip->setViewpoint( VPs[ea.getKey()-'1'], 4.0 );
        }
        return false;
    }

    osg::observer_ptr<osgEarthUtil::EarthManipulator> _manip;
};

//static const char * const sIdent = "$Id: plugin.cpp 9327 2008-09-14 11:18:44Z jef $";
static const QString sName = QObject::tr( "Globe" );
static const QString sDescription = QObject::tr( "Overlay data on a 3D globe" );
static const QString sPluginVersion = QObject::tr( "Version 0.1" );
static const QgisPlugin::PLUGINTYPE sPluginType = QgisPlugin::UI;

GlobePlugin::GlobePlugin( QgisInterface* theQgisInterface )
  : QgisPlugin( sName, sDescription, sPluginVersion, sPluginType ),
    mQGisIface( theQgisInterface ),
    mQActionPointer( NULL ),
    viewer()
{
}

GlobePlugin::~GlobePlugin()
{
}

void GlobePlugin::initGui()
{
  // Create the action for tool
  mQActionPointer = new QAction( QIcon( ":/globe/globe_plugin.png" ), tr( "Globe" ), this );
  // Set the what's this text
  mQActionPointer->setWhatsThis( tr( "Overlay data on a 3D globe" ) );
  // Connect the action to the run
  connect( mQActionPointer, SIGNAL( triggered() ), this, SLOT( run() ) );
  // Add the icon to the toolbar
  mQGisIface->addToolBarIcon( mQActionPointer );
  mQGisIface->addPluginToMenu( tr( "&Globe" ), mQActionPointer );
}

void GlobePlugin::run()
{
  // install the programmable manipulator.
  osgEarthUtil::EarthManipulator* manip = new osgEarthUtil::EarthManipulator();
  viewer.setCameraManipulator( manip );

  // The "Map" is the data model object that we will be visualizing. It will be
  // geocentric by default, but you can specify a projected map in the constructor.
  osgEarth::Map* map = new osgEarth::Map();

  // Add an image layer to the map.
  {
    osgEarth::Config conf;
    conf.add( "url", "http://demo.pelicanmapping.com/rmweb/data/bluemarble-tms/tms.xml" );
    osgEarth::MapLayer* layer = new osgEarth::MapLayer( "NASA", osgEarth::MapLayer::TYPE_IMAGE, "tms", conf );
    map->addMapLayer( layer );
  }

  // Add a heightfield layer to the map. You can add any number of heightfields and
  // osgEarth will composite them automatically.
  {
    osgEarth::Config conf;
    conf.add( "url", "http://demo.pelicanmapping.com/rmweb/data/srtm30_plus_tms/tms.xml" );
    osgEarth::MapLayer* layer = new osgEarth::MapLayer( "SRTM", osgEarth::MapLayer::TYPE_HEIGHTFIELD, "tms", conf );
    map->addMapLayer( layer );
  }

  // The MapNode will render the Map object in the scene graph.
  osgEarth::MapNode* mapNode = new osgEarth::MapNode( map );

  // Set a home viewpoint
  if ( mapNode && mapNode->getMap()->isGeocentric() )
  {
    manip->setHomeViewpoint( 
      osgEarthUtil::Viewpoint( osg::Vec3d( -90, 0, 0 ), 0.0, -90.0, 4e7 ),
      1.0 );
  }

  viewer.setSceneData( mapNode );

  manip->getSettings()->bindMouseDoubleClick(
      osgEarthUtil::EarthManipulator::ACTION_GOTO,
      osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON );

  // add our fly-to handler
  viewer.addEventHandler(new FlyToViewpointHandler( manip ));

  // add some stock OSG handlers:
  viewer.addEventHandler(new osgViewer::StatsHandler());
  viewer.addEventHandler(new osgViewer::WindowSizeHandler());
  viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));

  viewer.show();
}

void GlobePlugin::unload()
{
  // remove the GUI
  mQGisIface->removePluginMenu( "&Globe", mQActionPointer );
  mQGisIface->removeToolBarIcon( mQActionPointer );
  delete mQActionPointer;
}

void GlobePlugin::help()
{
}

/**
 * Required extern functions needed  for every plugin
 * These functions can be called prior to creating an instance
 * of the plugin class
 */
// Class factory to return a new instance of the plugin class
QGISEXTERN QgisPlugin * classFactory( QgisInterface * theQgisInterfacePointer )
{
  return new GlobePlugin( theQgisInterfacePointer );
}
// Return the name of the plugin - note that we do not user class members as
// the class may not yet be insantiated when this method is called.
QGISEXTERN QString name()
{
  return sName;
}

// Return the description
QGISEXTERN QString description()
{
  return sDescription;
}

// Return the type (either UI or MapLayer plugin)
QGISEXTERN int type()
{
  return sPluginType;
}

// Return the version number for the plugin
QGISEXTERN QString version()
{
  return sPluginVersion;
}

// Delete ourself
QGISEXTERN void unload( QgisPlugin * thePluginPointer )
{
  delete thePluginPointer;
}
