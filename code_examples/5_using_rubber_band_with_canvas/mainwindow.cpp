/***************************************************************************
 *   Copyright (C) 2006 by Tim Sutton   *
 *   tim@linfiniti.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "mainwindow.h"
//
// QGIS Includes
//
#include <qgsapplication.h>
#include <qgsproviderregistry.h>
#include <qgssinglesymbolrenderer.h>
#include <qgsmaplayerregistry.h>
#include <qgsrasterlayer.h>
#include <qgsmapcanvas.h>
//
// Needed fr rubber band support
//
#include <qgspoint.h>
//
// QGIS Map tools
//
#include "qgsmaptoolpan.h"
#include "qgsmaptoolzoom.h"

MainWindow::MainWindow(QWidget* parent, Qt::WFlags fl)
    : QMainWindow(parent,fl)
{
  //required by Qt4 to initialise the ui
  setupUi(this);

  // Instantiate Provider Registry
#if defined(Q_WS_MAC)
  QString myPluginsDir        = "/Users/timsutton/apps/qgis.app/Contents/MacOS/lib/qgis";
#else
  QString myPluginsDir        = "/home/timlinux/apps/lib/qgis";
#endif
  QgsProviderRegistry::instance(myPluginsDir);


  // Create the Map Canvas
  mpMapCanvas= new QgsMapCanvas(0, 0);
  mpMapCanvas->enableAntiAliasing(true);
  mpMapCanvas->setCanvasColor(QColor(255, 255, 255));
  mpMapCanvas->freeze(false);
  mpMapCanvas->setVisible(true);
  mpMapCanvas->refresh();
  mpMapCanvas->show();
  
  // Lay our widgets out in the main window
  mpLayout = new QVBoxLayout(frameMap);
  mpLayout->addWidget(mpMapCanvas);

  //create the action behaviours
  connect(mpActionPan, SIGNAL(triggered()), this, SLOT(panMode()));
  connect(mpActionZoomIn, SIGNAL(triggered()), this, SLOT(zoomInMode()));
  connect(mpActionZoomOut, SIGNAL(triggered()), this, SLOT(zoomOutMode()));
  connect(mpActionAddLayer, SIGNAL(triggered()), this, SLOT(addLayer()));

  //create a little toolbar
  mpMapToolBar = addToolBar(tr("File"));
  mpMapToolBar->addAction(mpActionAddLayer);
  mpMapToolBar->addAction(mpActionZoomIn);
  mpMapToolBar->addAction(mpActionZoomOut);
  mpMapToolBar->addAction(mpActionPan);

  //create the maptools
  mpPanTool = new QgsMapToolPan(mpMapCanvas);
  mpPanTool->setAction(mpActionPan);
  mpZoomInTool = new QgsMapToolZoom(mpMapCanvas, FALSE); // false = in
  mpZoomInTool->setAction(mpActionZoomIn);
  mpZoomOutTool = new QgsMapToolZoom(mpMapCanvas, TRUE ); //true = out
  mpZoomOutTool->setAction(mpActionZoomOut);

  //create the rubber band
  bool myPolygonFlag=true;
  mpRubberBand = new QgsRubberBand(mpMapCanvas, myPolygonFlag );
  mpRubberBand->show();
}

MainWindow::~MainWindow()
{
  delete mpZoomInTool;
  delete mpZoomOutTool;
  delete mpPanTool;
  delete mpMapToolBar;
  delete mpMapCanvas;
  delete mpLayout;
  delete mpRubberBand;
}

void MainWindow::panMode()
{
  mpMapCanvas->setMapTool(mpPanTool);

}
void MainWindow::zoomInMode()
{
  mpMapCanvas->setMapTool(mpZoomInTool);
}
void MainWindow::zoomOutMode()
{
  mpMapCanvas->setMapTool(mpZoomOutTool);
}
void MainWindow::addLayer()
{
  QFileInfo myRasterFileInfo("../data/Abarema_jupunba_projection.tif");
  QgsRasterLayer * mypLayer = new QgsRasterLayer(myRasterFileInfo.filePath(), 
      myRasterFileInfo.completeBaseName());
  if (mypLayer->isValid())
  {
    qDebug("Layer is valid");
  }
  else
  {
    qDebug("Layer is NOT valid");
    return;
  }
  // render strategy for grayscale image (will be rendered as pseudocolor)
  mypLayer->setDrawingStyle( QgsRasterLayer::SingleBandPseudoColor );
  mypLayer->setColorShadingAlgorithm( QgsRasterLayer::PseudoColorShader );
  mypLayer->setContrastEnhancementAlgorithm(
    QgsContrastEnhancement::StretchToMinimumMaximum, false );
  mypLayer->setMinimumValue( mypLayer->grayBandName(), 0.0, false );
  mypLayer->setMaximumValue( mypLayer->grayBandName(), 10.0 );
  //create a layerset
  QList<QgsMapCanvasLayer> myList;
  // Add the layers to the Layer Set
  myList.append(QgsMapCanvasLayer(mypLayer, TRUE));//bool visibility
  // set the canvas to the extent of our layer
  mpMapCanvas->setExtent(mypLayer->extent());

  // Add the Vector Layer to the Layer Registry
  QgsMapLayerRegistry::instance()->addMapLayer(mypLayer, TRUE);

  // Set the Map Canvas Layer Set
  mpMapCanvas->setLayerSet(myList);
}
void MainWindow::on_mpToolShowRubberBand_clicked()
{
  QgsPoint myPoint1 = mpMapCanvas->getCoordinateTransform()->toMapCoordinates(10, 10);
  mpRubberBand->addPoint(myPoint1);
  QgsPoint myPoint2 = mpMapCanvas->getCoordinateTransform()->toMapCoordinates(20, 10);
  mpRubberBand->addPoint(myPoint2);
  QgsPoint myPoint3 = mpMapCanvas->getCoordinateTransform()->toMapCoordinates(20, 20);
  mpRubberBand->addPoint(myPoint3);
  QgsPoint myPoint4 = mpMapCanvas->getCoordinateTransform()->toMapCoordinates(10, 20);
  mpRubberBand->addPoint(myPoint4);
}
void MainWindow::on_mpToolHideRubberBand_clicked()
{
  bool myPolygonFlag=true;
  mpRubberBand->reset(myPolygonFlag);
}
