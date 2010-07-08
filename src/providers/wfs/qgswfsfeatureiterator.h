#ifndef QGSWFSFEATUREITERATOR_H
#define QGSWFSFEATUREITERATOR_H

#include "qgsvectordataprovider.h"

class QgsWFSProvider;

class QgsWFSFeatureIterator : public QgsVectorDataProviderIterator
{
public:
  QgsWFSFeatureIterator( QgsWFSProvider* p,
                         QgsAttributeList fetchAttributes,
                         QgsRectangle rect,
                         bool fetchGeometry,
                         bool useIntersect );

  ~QgsWFSFeatureIterator();

  //! fetch next feature, return true on success
  virtual bool nextFeature(QgsFeature& f);

  //! reset the iterator to the starting position
  virtual bool rewind();

  //! end of iterating: free the resources / lock
  virtual bool close();

protected:
  QgsWFSProvider* P;

  /**Iterator on the feature vector for use in rewind(), nextFeature(), etc...*/
  QList<int>::iterator mFeatureIterator;

  /**Vector where the ids of the selected features are inserted*/
  QList<int> mSelectedFeatures;
};

#endif // QGSWFSFEATUREITERATOR_H
