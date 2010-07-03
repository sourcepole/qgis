#ifndef QGSVECTORLAYERITERATOR_H
#define QGSVECTORLAYERITERATOR_H

#include "qgsvectordataprovider.h"

class QgsVectorLayer;

class QgsVectorLayerIterator : public QgsVectorDataProviderIterator
{
public:

  QgsVectorLayerIterator( QgsVectorLayer* l,
                          QgsAttributeList fetchAttributes,
                          QgsRectangle rect,
                          bool fetchGeometry,
                          bool useIntersect );

  ~QgsVectorLayerIterator();

  //! fetch next feature, return true on success
  virtual bool nextFeature(QgsFeature& f);

  //! reset the iterator to the starting position
  virtual bool rewind();

  //! end of iterating: free the resources / lock
  virtual bool close();

protected:
  QgsVectorLayer* L;

  QgsAttributeList mFetchProvAttributes;
  QgsFeatureIterator mProvIter;

  QSet<int> mFetchConsidered;
  QgsGeometryMap::iterator mFetchChangedGeomIt;
  QgsFeatureList::iterator mFetchAddedFeaturesIt;

};

#endif // QGSVECTORLAYERITERATOR_H
