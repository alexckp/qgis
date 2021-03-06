#include "qgsgpxfeatureiterator.h"
#include "qgsgpxprovider.h"
#include "gpsdata.h"

#include "qgsapplication.h"
#include "qgsgeometry.h"
#include "qgslogger.h"

#include <cstring>


// used from provider:
// - mFeatureType
// - data

QgsGPXFeatureIterator::QgsGPXFeatureIterator( QgsGPXProvider* p,
                                              QgsAttributeList fetchAttributes,
                                              QgsRectangle rect,
                                              bool fetchGeometry,
                                              bool useIntersect )
 : QgsVectorDataProviderIterator(fetchAttributes, rect, fetchGeometry, useIntersect ),
   P(p)
{
  P->mDataMutex.lock();

  if ( rect.isEmpty() )
  {
    mRect = P->extent();
  }
  else
  {
    mRect = rect;
  }

  rewind();
}

QgsGPXFeatureIterator::~QgsGPXFeatureIterator()
{
  close();
}

bool QgsGPXFeatureIterator::nextFeature(QgsFeature& feature)
{
  if (mClosed)
    return false;

  feature.setValid( false );

  if ( P->mFeatureType == QgsGPXProvider::WaypointType )
  {
    return nextWaypoint(feature);
  }
  else if ( P->mFeatureType == QgsGPXProvider::RouteType )
  {
    return nextRoute(feature);
  }
  else if ( P->mFeatureType == QgsGPXProvider::TrackType )
  {
    return nextTrack(feature);
  }

  return false;
}

bool QgsGPXFeatureIterator::nextWaypoint(QgsFeature& feature)
{
  // go through the list of waypoints and return the first one that is in
  // the bounds rectangle
  for ( ; mWptIter != P->data->waypointsEnd(); ++mWptIter )
  {
    const QgsWaypoint* wpt;
    wpt = &( *mWptIter );
    if ( ! boundsCheck( wpt->lon, wpt->lat ) )
      continue;

    feature.setFeatureId( wpt->id );
    feature.setValid( true );

    // some wkb voodoo
    if ( mFetchGeometry )
    {
      char* geo = new char[21];
      std::memset( geo, 0, 21 );
      geo[0] = QgsApplication::endian();
      geo[geo[0] == QgsApplication::NDR ? 1 : 4] = QGis::WKBPoint;
      std::memcpy( geo + 5, &wpt->lon, sizeof( double ) );
      std::memcpy( geo + 13, &wpt->lat, sizeof( double ) );
      feature.setGeometryAndOwnership(( unsigned char * )geo, 1+4+8+8 );
    }

    QVariant* attrs = feature.resizeAttributeVector( P->fieldCount() );

    // add attributes if they are wanted
    for ( QgsAttributeList::const_iterator iter = mFetchAttributes.begin(); iter != mFetchAttributes.end(); ++iter )
    {
      if ( !setCommonAttribute( wpt, attrs, *iter ) )
      {
        if ( *iter == QgsGPXProvider::EleAttr && wpt->ele != -std::numeric_limits<double>::max() )
          attrs[ QgsGPXProvider::EleAttr ] = wpt->ele;
        else if ( *iter == QgsGPXProvider::SymAttr )
          attrs[ QgsGPXProvider::SymAttr ] = wpt->sym;
      }
    }

    ++mWptIter;
    return true;
  }

  return false;
}


bool QgsGPXFeatureIterator::nextRoute(QgsFeature& feature)
{
  // go through the routes and return the first one that is in the bounds
  // rectangle
  for ( ; mRteIter != P->data->routesEnd(); ++mRteIter )
  {
    const QgsRoute* rte;
    rte = &( *mRteIter );

    if ( rte->points.size() == 0 )
      continue;

    if ( !boundsCheckRect( rte->xMin, rte->yMin, rte->xMax, rte->yMax ) )
      continue;

    // some wkb voodoo
    int nPoints = rte->points.size();
    char* geo = new char[9 + 16 * nPoints];
    std::memset( geo, 0, 9 + 16 * nPoints );
    geo[0] = QgsApplication::endian();
    geo[geo[0] == QgsApplication::NDR ? 1 : 4] = QGis::WKBLineString;
    std::memcpy( geo + 5, &nPoints, 4 );
    for ( uint i = 0; i < rte->points.size(); ++i )
    {
      std::memcpy( geo + 9 + 16 * i, &rte->points[i].lon, sizeof( double ) );
      std::memcpy( geo + 9 + 16 * i + 8, &rte->points[i].lat, sizeof( double ) );
    }

    //create QgsGeometry and use it for intersection test
    //if geometry is to be fetched, it is attached to the feature, otherwise we delete it
    QgsGeometry* theGeometry = new QgsGeometry();
    theGeometry->fromWkb(( unsigned char * )geo, 9 + 16 * nPoints );
    bool intersection = theGeometry->intersects( mRect );//use geos for precise intersection test

    if ( !intersection )
    {
      delete theGeometry;
      continue;
    }

    if ( mFetchGeometry )
    {
      feature.setGeometry( theGeometry );
    }
    else
    {
      delete theGeometry;
    }
    feature.setFeatureId( rte->id );
    feature.setValid( true );

    QVariant* attrs = feature.resizeAttributeVector( P->fieldCount() );

    // add attributes if they are wanted
    for ( QgsAttributeList::const_iterator iter = mFetchAttributes.begin(); iter != mFetchAttributes.end(); ++iter )
    {
      if ( !setCommonAttribute( rte, attrs, *iter ) )
      {
        if ( *iter == QgsGPXProvider::NumAttr && rte->number != std::numeric_limits<int>::max() )
          attrs[ QgsGPXProvider::NumAttr ] = rte->number;
      }
    }

    ++mRteIter;
    return true;
  }
  return false;
}

bool QgsGPXFeatureIterator::nextTrack(QgsFeature& feature)
{
  // go through the tracks and return the first one that is in the bounds
  // rectangle
  for ( ; mTrkIter != P->data->tracksEnd(); ++mTrkIter )
  {
    const QgsTrack* trk;
    trk = &( *mTrkIter );

    QgsDebugMsg( QString( "GPX feature track segments: %1" ).arg( trk->segments.size() ) );
    if ( trk->segments.size() == 0 )
      continue;

    // A track consists of several segments. Add all those segments into one.
    int totalPoints = 0;;
    for ( std::vector<QgsTrackSegment>::size_type i = 0; i < trk->segments.size(); i ++ )
    {
      totalPoints += trk->segments[i].points.size();
    }
    if ( totalPoints == 0 )
      continue;

    QgsDebugMsg( "GPX feature track total points: " + QString::number( totalPoints ) );

    if ( !boundsCheckRect( trk->xMin, trk->yMin, trk->xMax, trk->yMax ) )
      continue;

    // some wkb voodoo
    char* geo = new char[9 + 16 * totalPoints];
    if ( !geo )
    {
      QgsDebugMsg( "Too large track!!!" );
      return false;
    }
    std::memset( geo, 0, 9 + 16 * totalPoints );
    geo[0] = QgsApplication::endian();
    geo[geo[0] == QgsApplication::NDR ? 1 : 4] = QGis::WKBLineString;
    std::memcpy( geo + 5, &totalPoints, 4 );

    int thisPoint = 0;
    for ( std::vector<QgsTrackSegment>::size_type k = 0; k < trk->segments.size(); k++ )
    {
      int nPoints = trk->segments[k].points.size();
      for ( int i = 0; i < nPoints; ++i )
      {
        std::memcpy( geo + 9 + 16 * thisPoint,     &trk->segments[k].points[i].lon, sizeof( double ) );
        std::memcpy( geo + 9 + 16 * thisPoint + 8, &trk->segments[k].points[i].lat, sizeof( double ) );
        thisPoint++;
      }
    }

    //create QgsGeometry and use it for intersection test
    //if geometry is to be fetched, it is attached to the feature, otherwise we delete it
    QgsGeometry* theGeometry = new QgsGeometry();
    theGeometry->fromWkb(( unsigned char * )geo, 9 + 16 * totalPoints );
    bool intersection = theGeometry->intersects( mRect );//use geos for precise intersection test

    if ( !intersection ) //no intersection, delete geometry and move on
    {
      delete theGeometry;
      continue;
    }

    if ( mFetchGeometry )
    {
      feature.setGeometry( theGeometry );
    }
    else
    {
      delete theGeometry;
    }
    feature.setFeatureId( trk->id );
    feature.setValid( true );

    QVariant* attrs = feature.resizeAttributeVector( P->fieldCount() );

    // add attributes if they are wanted
    for ( QgsAttributeList::const_iterator iter = mFetchAttributes.begin(); iter != mFetchAttributes.end(); ++iter )
    {
      if ( !setCommonAttribute( trk, attrs, *iter ) )
      {
        if ( *iter == QgsGPXProvider::NumAttr && trk->number != std::numeric_limits<int>::max() )
          attrs[ QgsGPXProvider::NumAttr ] = trk->number;
      }
    }

    ++mTrkIter;
    return true;
  }
  return false;
}

bool QgsGPXFeatureIterator::setCommonAttribute( const QgsGPSObject* obj, QVariant* attrs, int index )
{
  switch ( index )
  {
    case QgsGPXProvider::NameAttr:
      attrs[ QgsGPXProvider::NameAttr ] = obj->name;
      return true;
    case QgsGPXProvider::CmtAttr:
      attrs[ QgsGPXProvider::CmtAttr ] = obj->cmt;
      return true;
    case QgsGPXProvider::DscAttr:
      attrs[ QgsGPXProvider::DscAttr ] = obj->desc;
      return true;
    case QgsGPXProvider::SrcAttr:
      attrs[ QgsGPXProvider::SrcAttr ] = obj->src;
      return true;
    case QgsGPXProvider::URLAttr:
      attrs[ QgsGPXProvider::URLAttr ] = obj->url;
      return true;
    case QgsGPXProvider::URLNameAttr:
      attrs[ QgsGPXProvider::URLNameAttr ] = obj->urlname;
      return true;
  }
  return false;
}

bool QgsGPXFeatureIterator::boundsCheck( double x, double y )
{
  return ( x <= mRect.xMaximum() ) && ( x >= mRect.xMinimum() ) &&
         ( y <= mRect.yMaximum() ) && ( y >= mRect.yMinimum() );
}

bool QgsGPXFeatureIterator::boundsCheckRect( double xmin, double ymin, double xmax, double ymax )
{
  return ( xmax >= mRect.xMinimum() ) && ( xmin <= mRect.xMaximum() ) &&
         ( ymax >= mRect.yMinimum() ) && ( ymin <= mRect.yMaximum() );
}



bool QgsGPXFeatureIterator::rewind()
{
  if (mClosed)
    return false;

  if ( P->mFeatureType == QgsGPXProvider::WaypointType )
    mWptIter = P->data->waypointsBegin();
  else if ( P->mFeatureType == QgsGPXProvider::RouteType )
    mRteIter = P->data->routesBegin();
  else if ( P->mFeatureType == QgsGPXProvider::TrackType )
    mTrkIter = P->data->tracksBegin();

  return true;
}

bool QgsGPXFeatureIterator::close()
{
  if (mClosed)
    return false;

  P->mDataMutex.unlock();
  mClosed = true;
  return true;
}
