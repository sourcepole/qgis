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

  feature.setValid( false );

  //go through the loop until we find a feature in the filter
  while ( mSelectedFeatures.size() != 0 && mFeatureIterator != mSelectedFeatures.end() )
  {
    QgsFeature* origFeature = P->mFeatures[*mFeatureIterator];

    feature.setFeatureId( origFeature->id() );

    //we need geometry anyway, e.g. for intersection tests
    QgsGeometry* geometry = origFeature->geometry();
    unsigned char *geom = geometry->asWkb();
    int geomSize = geometry->wkbSize();
    unsigned char* copiedGeom = new unsigned char[geomSize];
    memcpy( copiedGeom, geom, geomSize );
    feature.setGeometryAndOwnership( copiedGeom, geomSize );

    const QgsAttributeMap& attributes = origFeature->attributeMap();
    for ( QgsAttributeList::const_iterator it = mFetchAttributes.begin(); it != mFetchAttributes.end(); ++it )
    {
      feature.addAttribute( *it, attributes[*it] );
    }
    ++mFeatureIterator;

    if ( mUseIntersect && feature.geometry() && !feature.geometry()->intersects( mRect ) )
      continue;

    feature.setValid( true );
    return true;
  }

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
