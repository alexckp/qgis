/***************************************************************************
                              qgswfsprovider.h
                              -------------------
  begin                : July 25, 2006
  copyright            : (C) 2006 by Marco Hugentobler
  email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSWFSPROVIDER_H
#define QGSWFSPROVIDER_H

#include <QDomElement>
#include "qgis.h"
#include "qgsrectangle.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsvectordataprovider.h"

#include <QMutex>

class QgsRectangle;
class QgsSpatialIndex;

/**A provider reading features from a WFS server*/
class QgsWFSProvider: public QgsVectorDataProvider
{
    Q_OBJECT
  public:

    enum REQUEST_ENCODING
    {
      GET,
      POST,
      SOAP,/*Note that this goes also through HTTP POST but additionally uses soap envelope and friends*/
      FILE  //reads from a file on disk
    };

    QgsWFSProvider( const QString& uri );
    ~QgsWFSProvider();

    /* Inherited from QgsVectorDataProvider */

    /**
     * Start iterating over features of the vector data provider.
     * For new code, consider using this method instead of select/nextFeature combo.
     * @param fetchAttributes list of attributes which should be fetched
     * @param rect spatial filter
     * @param fetchGeometry true if the feature geometry should be fetched
     * @param useIntersect true if an accurate intersection test should be used,
     *                     false if a test based on bounding box is sufficient
     * @return iterator instance for retrieval of features
     * @note Added in v1.6
     */
    virtual QgsFeatureIterator getFeatures( QgsAttributeList fetchAttributes = QgsAttributeList(),
                                            QgsRectangle rect = QgsRectangle(),
                                            bool fetchGeometry = true,
                                            bool useIntersect = false );

    QGis::WkbType geometryType() const;
    long featureCount() const;
    uint fieldCount() const;
    const QgsFieldMap & fields() const;

    virtual QgsCoordinateReferenceSystem crs();

    /* Inherited from QgsDataProvider */

    QgsRectangle extent();
    bool isValid();
    QString name() const;
    QString description() const;

    /* new functions */

    /**Sets the encoding type in which the provider makes requests and interprets
     results. Posibilities are GET, POST, SOAP*/
    void setEncoding( QgsWFSProvider::REQUEST_ENCODING e ) {mEncoding = e;}

    /**Makes a GetFeatures, receives the features from the wfs server (as GML), converts them to QgsFeature and
       stores them in a vector*/
    int getFeature( const QString& uri );


  signals:
    void dataReadProgressMessage( QString message );

  private slots:
    /**Receives the progress signals from QgsWFSData::dataReadProgress, generates a string
     and emits the dataReadProgressMessage signal*/
    void handleWFSProgressMessage( int done, int total );


  protected:
    QgsFieldMap mFieldMap;
    QgsFieldVector mFields;
    /**The encoding used for request/response. Can be GET, POST or SOAP*/
    REQUEST_ENCODING mEncoding;
    /**Bounding box for the layer*/
    QgsRectangle mExtent;
    /**A spatial index for fast access to a feature subset*/
    QgsSpatialIndex *mSpatialIndex;
    /**Vector where the features are inserted*/
    QList<QgsFeature*> mFeatures;
    /**Geometry type of the features in this layer*/
    mutable QGis::WkbType mWKBType;
    /**Source CRS*/
    QgsCoordinateReferenceSystem mSourceCRS;
    int mFeatureCount;
    /**Flag if provider is valid*/
    bool mValid;

    friend class QgsWFSFeatureIterator;

    QMutex mDataMutex;


    /**Collects information about the field types. Is called internally from QgsWFSProvider::getFeature. The method delegates the work to request specific ones and gives back the name of the geometry attribute and the thematic attributes with their types*/
    int describeFeatureType( const QString& uri, QString& geometryAttribute, QgsFieldVector& fields );

    //encoding specific methods of getFeature
    int getFeatureGET( const QString& uri, const QString& geometryAttribute );
    int getFeaturePOST( const QString& uri, const QString& geometryAttribute );
    int getFeatureSOAP( const QString& uri, const QString& geometryAttribute );
    int getFeatureFILE( const QString& uri, const QString& geometryAttribute );
    //encoding specific methods of describeFeatureType
    int describeFeatureTypeGET( const QString& uri, QString& geometryAttribute, QgsFieldVector& fields );
    int describeFeatureTypePOST( const QString& uri, QString& geometryAttribute, QgsFieldVector& fields );
    int describeFeatureTypeSOAP( const QString& uri, QString& geometryAttribute, QgsFieldVector& fields );
    int describeFeatureTypeFile( const QString& uri, QString& geometryAttribute, QgsFieldVector& fields );

    /**Reads the name of the geometry attribute, the thematic attributes and their types from a dom document. Returns 0 in case of success*/
    int readAttributesFromSchema( QDomDocument& schemaDoc, QString& geometryAttribute, QgsFieldVector& fields ) const;
    /**This method tries to guess the geometry attribute and the other attribute names from the .gml file if no schema is present. Returns 0 in case of success*/
    int guessAttributesFromFile( const QString& uri, QString& geometryAttribute, std::list<QString>& thematicAttributes ) const;

    //GML2 specific methods
    int getExtentFromGML2( QgsRectangle* extent, const QDomElement& wfsCollectionElement ) const;

    int getFeaturesFromGML2( const QDomElement& wfsCollectionElement, const QString& geometryAttribute );

    int getWkbFromGML2( const QDomNode& geometryElement, unsigned char** wkb, int* wkbSize, QGis::WkbType* type ) const;
    /**Creates WKB from a <Point> element*/
    int getWkbFromGML2Point( const QDomElement& geometryElement, unsigned char** wkb, int* wkbSize, QGis::WkbType* type ) const;
    /**Creates WKB from a <Polygon> element*/
    int getWkbFromGML2Polygon( const QDomElement& geometryElement, unsigned char** wkb, int* wkbSize, QGis::WkbType* type ) const;
    /**Creates WKB from a <LineString> element*/
    int getWkbFromGML2LineString( const QDomElement& geometryElement, unsigned char** wkb, int* wkbSize, QGis::WkbType* type ) const;
    /**Creates WKB from a <MultiPoint> element*/
    int getWkbFromGML2MultiPoint( const QDomElement& geometryElement, unsigned char** wkb, int* wkbSize, QGis::WkbType* type ) const;
    /**Creates WKB from a <MultiLineString> element*/
    int getWkbFromGML2MultiLineString( const QDomElement& geometryElement, unsigned char** wkb, int* wkbSize, QGis::WkbType* type ) const;
    /**Creates WKB from a <MultiPolygon> element*/
    int getWkbFromGML2MultiPolygon( const QDomElement& geometryElement, unsigned char** wkb, int* wkbSize, QGis::WkbType* type ) const;
    /**Reads the <gml:coordinates> element and extracts the coordinates as points
       @param coords list where the found coordinates are appended
       @param elem the <gml:coordinates> element
       @return 0 in case of success*/
    int readGML2Coordinates( std::list<QgsPoint>& coords, const QDomElement elem ) const;
    /**Tries to create a QgsCoordinateReferenceSystem object and assign it to mSourceCRS. Returns 0 in case of success*/
    int setCRSFromGML2( const QDomElement& wfsCollectionElement );
};

#endif
