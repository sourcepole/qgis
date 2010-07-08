#ifndef QGSOSMFEATUREITERATOR_H
#define QGSOSMFEATUREITERATOR_H

#include "qgsvectordataprovider.h"

#include <sqlite3.h>

class QgsOSMDataProvider;

class QgsOSMFeatureIterator : public QgsVectorDataProviderIterator
{
public:
  QgsOSMFeatureIterator( QgsOSMDataProvider* p,
                         QgsAttributeList fetchAttributes,
                         QgsRectangle rect,
                         bool fetchGeometry,
                         bool useIntersect );

  ~QgsOSMFeatureIterator();

  //! fetch next feature, return true on success
  virtual bool nextFeature(QgsFeature& f);

  //! reset the iterator to the starting position
  virtual bool rewind();

  //! end of iterating: free the resources / lock
  virtual bool close();

protected:
  QgsOSMDataProvider* P;

  //! geometry object of area from which features should be fetched after calling of select() function
  QgsGeometry* mSelectionRectangleGeom;

  //! pointer to main sqlite3 database statement object; this statement serves to select OSM data
  sqlite3_stmt *mDatabaseStmt;

};

#endif // QGSOSMFEATUREITERATOR_H
