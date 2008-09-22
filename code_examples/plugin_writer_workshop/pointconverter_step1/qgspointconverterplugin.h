#ifndef QGSPOINTCONVERTERPLUGIN_H
#define QGSPOINTCONVERTERPLUGIN_H

#include "qgisplugin.h"
#include <QObject>

/**A plugin that converts vector layers to delimited text point files.
 The vertices of polygon/line type layers are converted to point features*/
class QgsPointConverterPlugin: public QObject, public QgisPlugin
{
  public:
  QgsPointConverterPlugin(QgisApp* app, QgisIface* iface);
  ~QgsPointConverterPlugin();
  void initGui();
  void unload();
};

#endif
