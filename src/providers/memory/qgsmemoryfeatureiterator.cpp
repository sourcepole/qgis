#include "qgsmemoryfeatureiterator.h"
#include "qgsmemoryprovider.h"

#include "qgsgeometry.h"
#include "qgslogger.h"
#include "qgsspatialindex.h"

// used from provider:
// - mSpatialIndex
// - mFeatures

QgsMemoryFeatureIterator::QgsMemoryFeatureIterator( QgsMemoryProvider* p,
                                                    QgsAttributeList fetchAttributes,
                                                    QgsRectangle rect,
                                                    bool fetchGeometry,
                                                    bool useIntersect )
 : QgsVectorDataProviderIterator(fetchAttributes, rect, fetchGeometry, useIntersect),
   P(p),
   mRectGeom( NULL )
{

  P->mDataMutex.lock();

  mRectGeom = QgsGeometry::fromRect( rect );

  // if there's spatial index, use it!
  // (but don't use it when selection rect is not specified)
  if ( P->mSpatialIndex && !mRect.isEmpty() )
  {
    mSelectUsingSpatialIndex = true;
    mSelectSI_Features = P->mSpatialIndex->intersects( rect );
    QgsDebugMsg( "Features returned by spatial index: " + QString::number( mSelectSI_Features.count() ) );
  }
  else
  {
    mSelectUsingSpatialIndex = false;
    mSelectSI_Features.clear();
  }

  rewind();
}

QgsMemoryFeatureIterator::~QgsMemoryFeatureIterator()
{
  close();
}

bool QgsMemoryFeatureIterator::nextFeature(QgsFeature& feature)
{
  if (mClosed)
    return false;

  feature.setValid( false );
  bool hasFeature = false;

  // option 1: using spatial index
  if ( mSelectUsingSpatialIndex )
  {
    while ( mSelectSI_Iterator != mSelectSI_Features.end() )
    {
      // do exact check in case we're doing intersection
      if ( mUseIntersect )
      {
        if ( P->mFeatures[*mSelectSI_Iterator].geometry()->intersects( mRectGeom ) )
          hasFeature = true;
      }
      else
        hasFeature = true;

      if ( hasFeature )
        break;

      mSelectSI_Iterator++;
    }

    // copy feature
    if ( hasFeature )
    {
      feature = P->mFeatures[*mSelectSI_Iterator];
      mSelectSI_Iterator++;
    }
    return hasFeature;
  }

  // option 2: not using spatial index
  while ( mSelectIterator != P->mFeatures.end() )
  {
    if ( mRect.isEmpty() )
    {
      // selection rect empty => using all features
      hasFeature = true;
    }
    else
    {
      if ( mUseIntersect )
      {
        // using exact test when checking for intersection
        if ( mSelectIterator->geometry()->intersects( mRectGeom ) )
          hasFeature = true;
      }
      else
      {
        // check just bounding box against rect when not using intersection
        if ( mSelectIterator->geometry()->boundingBox().intersects( mRect ) )
          hasFeature = true;
      }
    }

    if ( hasFeature )
      break;

    mSelectIterator++;
  }

  // copy feature
  if ( hasFeature )
  {
    feature = mSelectIterator.value();
    mSelectIterator++;
    feature.setValid( true );
  }

  return hasFeature;
}

bool QgsMemoryFeatureIterator::rewind()
{
  if (mClosed)
    return false;

  if ( mSelectUsingSpatialIndex )
    mSelectSI_Iterator = mSelectSI_Features.begin();
  else
    mSelectIterator = P->mFeatures.begin();

  return true;
}

bool QgsMemoryFeatureIterator::close()
{
  if (mClosed)
    return false;

  delete mRectGeom;
  mRectGeom = NULL;

  P->mDataMutex.unlock();

  return true;
}
