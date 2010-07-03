#ifndef QGSOGRFEATUREITERATOR_H
#define QGSOGRFEATUREITERATOR_H

#include "qgsvectordataprovider.h"

class QgsOgrProvider;

#include <ogr_api.h>

class QgsOgrFeatureIterator : public QgsVectorDataProviderIterator
{
public:
  QgsOgrFeatureIterator( QgsOgrProvider* p,
                         QgsAttributeList fetchAttributes,
                         QgsRectangle rect,
                         bool fetchGeometry,
                         bool useIntersect );

  ~QgsOgrFeatureIterator();

  //! fetch next feature, return true on success
  virtual bool nextFeature(QgsFeature& f);

  //! reset the iterator to the starting position
  virtual bool rewind();

  //! end of iterating: free the resources / lock
  virtual bool close();

protected:
  QgsOgrProvider* P;

  //! Selection rectangle
  OGRGeometryH mSelectionRectangle;
};

#endif // QGSOGRFEATUREITERATOR_H
