#ifndef QGSDELIMITEDTEXTFEATUREITERATOR_H
#define QGSDELIMITEDTEXTFEATUREITERATOR_H

#include "qgsvectordataprovider.h"

class QgsDelimitedTextProvider;

class QgsDelimitedTextFeatureIterator : public QgsVectorDataProviderIterator
{
public:
  QgsDelimitedTextFeatureIterator( QgsDelimitedTextProvider* p,
                                   QgsAttributeList fetchAttributes,
                                   QgsRectangle rect,
                                   bool fetchGeometry,
                                   bool useIntersect );

  ~QgsDelimitedTextFeatureIterator();

  //! fetch next feature, return true on success
  virtual bool nextFeature(QgsFeature& f);

  //! reset the iterator to the starting position
  virtual bool rewind();

  //! end of iterating: free the resources / lock
  virtual bool close();


  /**
   * Check to see if the point is withn the selection
   * rectangle
   * @param x X value of point
   * @param y Y value of point
   * @return True if point is within the rectangle
  */
  bool boundsCheck( double x, double y );


protected:
  QgsDelimitedTextProvider* P;

  //! Current feature id
  long mFid;

};


#endif // QGSDELIMITEDTEXTFEATUREITERATOR_H
