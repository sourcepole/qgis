#ifndef QGSMEMORYFEATUREITERATOR_H
#define QGSMEMORYFEATUREITERATOR_H

#include "qgsvectordataprovider.h"

class QgsMemoryProvider;

typedef QMap<int, QgsFeature> QgsFeatureMap;


class QgsMemoryFeatureIterator : public QgsVectorDataProviderIterator
{
public:
  QgsMemoryFeatureIterator( QgsMemoryProvider* p,
                            QgsAttributeList fetchAttributes,
                            QgsRectangle rect,
                            bool fetchGeometry,
                            bool useIntersect );

  ~QgsMemoryFeatureIterator();

  //! fetch next feature, return true on success
  virtual bool nextFeature(QgsFeature& f);

  //! reset the iterator to the starting position
  virtual bool rewind();

  //! end of iterating: free the resources / lock
  virtual bool close();

protected:
  QgsMemoryProvider* P;

  QgsGeometry* mRectGeom;

  QgsFeatureMap::iterator mSelectIterator;
  bool mSelectUsingSpatialIndex;
  QList<int> mSelectSI_Features;
  QList<int>::iterator mSelectSI_Iterator;
};


#endif // QGSMEMORYFEATUREITERATOR_H
