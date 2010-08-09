#include "qgswfsfeatureiterator.h"
#include "qgswfsprovider.h"

#include "qgsgeometry.h"
#include "qgsspatialindex.h"

// from provider:
// - mExtent
// - mSpatialIndex
// - mFeatures

QgsWFSFeatureIterator::QgsWFSFeatureIterator( QgsWFSProvider* p,
                       QgsAttributeList fetchAttributes,
                       QgsRectangle rect,
                       bool fetchGeometry,
                       bool useIntersect )
 : QgsVectorDataProviderIterator(fetchAttributes, rect, fetchGeometry, useIntersect),
   P(p)
{
  P->mDataMutex.lock();

  if ( mRect.isEmpty() )
  {
    mRect = P->mExtent;
  }

  mSelectedFeatures = P->mSpatialIndex->intersects( mRect );
  mFeatureIterator = mSelectedFeatures.begin();
}

QgsWFSFeatureIterator::~QgsWFSFeatureIterator()
{
  close();
}

bool QgsWFSFeatureIterator::nextFeature(QgsFeature& feature)
{
  if (mClosed)
    return false;

  //go through the loop until we find a feature in the filter
  while ( mSelectedFeatures.size() != 0 && mFeatureIterator != mSelectedFeatures.end() )
  {
    QgsFeature* origFeature = P->mFeatures[*mFeatureIterator];

    if ( mUseIntersect && feature.geometry() && !feature.geometry()->intersects( mRect ) )
    {
      ++mFeatureIterator;
      continue;
    }

    feature = *origFeature; // copy feature

    ++mFeatureIterator;
    return true;
  }

  feature.setValid( false );
  return false;
}

bool QgsWFSFeatureIterator::rewind()
{
  if (mClosed)
    return false;

  mFeatureIterator = mSelectedFeatures.begin();
  return true;
}

bool QgsWFSFeatureIterator::close()
{
  if (mClosed)
    return false;

  mSelectedFeatures.clear();

  P->mDataMutex.unlock();

  mClosed = true;
  return true;
}
