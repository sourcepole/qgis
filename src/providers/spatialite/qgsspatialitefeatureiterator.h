#ifndef QGSSPATIALITEFEATUREITERATOR_H
#define QGSSPATIALITEFEATUREITERATOR_H

#include "qgsvectordataprovider.h"

class QgsSpatiaLiteProvider;

extern "C"
{
#include <sqlite3.h>
}

class QgsSpatiaLiteFeatureIterator : public QgsVectorDataProviderIterator
{
public:
  QgsSpatiaLiteFeatureIterator( QgsSpatiaLiteProvider* p,
                                QgsAttributeList fetchAttributes,
                                QgsRectangle rect,
                                bool fetchGeometry,
                                bool useIntersect );

  ~QgsSpatiaLiteFeatureIterator();

  //! fetch next feature, return true on success
  virtual bool nextFeature(QgsFeature& f);

  //! reset the iterator to the starting position
  virtual bool rewind();

  //! end of iterating: free the resources / lock
  virtual bool close();

protected:
  QgsSpatiaLiteProvider* P;

  /**
    * SQLite statement handle
   */
  sqlite3_stmt *sqliteStatement;
};

#endif // QGSSPATIALITEFEATUREITERATOR_H
