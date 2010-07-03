#ifndef QGSPOSTGRESFEATUREITERATOR_H
#define QGSPOSTGRESFEATUREITERATOR_H

#include "qgsvectordataprovider.h"

#include <queue>

class QgsPostgresProvider;

class QgsPostgresFeatureIterator : public QgsVectorDataProviderIterator
{
public:
  QgsPostgresFeatureIterator( QgsPostgresProvider* p,
                              QgsAttributeList fetchAttributes,
                              QgsRectangle rect,
                              bool fetchGeometry,
                              bool useIntersect );

  ~QgsPostgresFeatureIterator();

  //! fetch next feature, return true on success
  virtual bool nextFeature(QgsFeature& f);

  //! reset the iterator to the starting position
  virtual bool rewind();

  //! end of iterating: free the resources / lock
  virtual bool close();

protected:
  QgsPostgresProvider* P;

  int mFetched; // number of retrieved features

  /**
   * Feature queue that GetNextFeature will retrieve from
   * before the next fetch from PostgreSQL
   */
  std::queue<QgsFeature> mFeatureQueue;

  /**
   * Maximal size of the feature queue
   */
  int mFeatureQueueSize;

};

#endif // QGSPOSTGRESFEATUREITERATOR_H
