#include "qgsvectorlayeriterator.h"

#include "qgsvectorlayer.h"
#include "qgsvectorlayereditbuffer.h"

#include "qgsgeometry.h"
#include "qgslogger.h"

// used from QgsVectorLayer:
// - mDataProvider
// - mEditable
// - mDeletedFeatureIds
// - mAddedFeatures
// - mChangedGeometries
// - mUpdatedFields
// - mAddedAttributeIds
// - updateFeatureAttributes()

QgsVectorLayerIterator::QgsVectorLayerIterator( QgsVectorLayer* l,
                                                QgsAttributeList fetchAttributes,
                                                QgsRectangle rect,
                                                bool fetchGeometry,
                                                bool useIntersect )
: QgsVectorDataProviderIterator(fetchAttributes, rect, fetchGeometry, useIntersect),
  L(l)
{

  if ( !L->mDataProvider )
  {
    mClosed = true;
    return;
  }

  QgsVectorLayerEditBuffer* b = L->mEditBuffer;

  if ( b )
  {
    mFetchConsidered = b->mDeletedFeatureIds;
    mFetchAddedFeaturesIt = b->mAddedFeatures.begin();
    mFetchChangedGeomIt = b->mChangedGeometries.begin();
  }

  //look in the normal features of the provider
  if ( mFetchAttributes.size() > 0 )
  {
    if ( b )
    {
      // fetch only available field from provider
      mFetchProvAttributes.clear();
      for ( QgsAttributeList::iterator it = mFetchAttributes.begin(); it != mFetchAttributes.end(); it++ )
      {
        if ( !b->mUpdatedFields.contains( *it ) || b->mAddedAttributeIds.contains( *it ) )
          continue;

        mFetchProvAttributes << *it;
      }

      mProvIter = L->mDataProvider->getFeatures( mFetchProvAttributes, rect, fetchGeometry, useIntersect );
    }
    else
      mProvIter = L->mDataProvider->getFeatures( mFetchAttributes, rect, fetchGeometry, useIntersect );
  }
  else
  {
    mProvIter = L->mDataProvider->getFeatures( QgsAttributeList(), rect, fetchGeometry, useIntersect );
  }

}

QgsVectorLayerIterator::~QgsVectorLayerIterator()
{
  close();
}


bool QgsVectorLayerIterator::nextFeature(QgsFeature& f)
{
  if ( mClosed )
    return false;

  if ( L->mEditBuffer )
  {
    QgsVectorLayerEditBuffer* b = L->mEditBuffer;
    if ( !mRect.isEmpty() )
    {
      // check if changed geometries are in rectangle
      for ( ; mFetchChangedGeomIt != b->mChangedGeometries.end(); mFetchChangedGeomIt++ )
      {
        int fid = mFetchChangedGeomIt.key();

        if ( mFetchConsidered.contains( fid ) )
          // skip deleted features
          continue;

        mFetchConsidered << fid;

        if ( !mFetchChangedGeomIt->intersects( mRect ) )
          // skip changed geometries not in rectangle and don't check again
          continue;

        f.setFeatureId( fid );
        f.setValid( true );

        if ( mFetchGeometry )
          f.setGeometry( mFetchChangedGeomIt.value() );

        if ( mFetchAttributes.size() > 0 )
        {
          if ( fid < 0 )
          {
            // fid<0 => in mAddedFeatures
            bool found = false;

            for ( QgsFeatureList::iterator it = b->mAddedFeatures.begin(); it != b->mAddedFeatures.end(); it++ )
            {
              if ( fid != it->id() )
              {
                found = true;
                f.setAttributeMap( it->attributeMap() );
                break;
              }
            }

            if ( !found )
              QgsDebugMsg( QString( "No attributes for the added feature %1 found" ).arg( f.id() ) );
          }
          else
          {
            // retrieve attributes from provider
            QgsFeature tmp;
            // TODO: do not fetch here using featureAtId - do it when iterating in provider
            L->mDataProvider->featureAtId( fid, tmp, false, mFetchProvAttributes );
            b->updateFeatureAttributes( tmp, mFetchAttributes );
            f.setAttributeMap( tmp.attributeMap() );
          }
        }

        // return complete feature
        mFetchChangedGeomIt++;
        return true;
      }

      // no more changed geometries
    }

    for ( ; mFetchAddedFeaturesIt != b->mAddedFeatures.end(); mFetchAddedFeaturesIt++ )
    {
      int fid = mFetchAddedFeaturesIt->id();

      if ( mFetchConsidered.contains( fid ) )
        // must have changed geometry outside rectangle
        continue;

      if ( !mRect.isEmpty() &&
           mFetchAddedFeaturesIt->geometry() &&
           !mFetchAddedFeaturesIt->geometry()->intersects( mRect ) )
        // skip added features not in rectangle
        continue;

      f.setFeatureId( fid );
      f.setValid( true );

      if ( mFetchGeometry )
        f.setGeometry( *mFetchAddedFeaturesIt->geometry() );

      if ( mFetchAttributes.size() > 0 )
      {
        f.setAttributeMap( mFetchAddedFeaturesIt->attributeMap() );
        b->updateFeatureAttributes( f, mFetchAttributes );
      }

      mFetchAddedFeaturesIt++;
      return true;
    }

    // no more added features
  }

  while ( mProvIter.nextFeature( f ) )
  {
    if ( mFetchConsidered.contains( f.id() ) )
      continue;

    if ( L->mEditBuffer )
      L->mEditBuffer->updateFeatureAttributes( f, mFetchAttributes );

    // found it
    return true;
  }

  close();
  return false;
}

bool QgsVectorLayerIterator::rewind()
{
  if (mClosed)
    return false;

  // TODO: rewind editing stuff
  mProvIter.rewind();
  return false;
}

bool QgsVectorLayerIterator::close()
{
  if (mClosed)
    return false;

  mProvIter.close();
  mClosed = true;
  return true;
}
