#ifndef QGSGPXFEATUREITERATOR_H
#define QGSGPXFEATUREITERATOR_H


#include "qgsvectordataprovider.h"

#include "gpsdata.h"

class QgsGPXProvider;


class QgsGPXFeatureIterator : public QgsVectorDataProviderIterator
{
public:
  QgsGPXFeatureIterator( QgsGPXProvider* p,
                         QgsAttributeList fetchAttributes,
                         QgsRectangle rect,
                         bool fetchGeometry,
                         bool useIntersect );

  ~QgsGPXFeatureIterator();

  //! fetch next feature, return true on success
  virtual bool nextFeature(QgsFeature& f);

  //! reset the iterator to the starting position
  virtual bool rewind();

  //! end of iterating: free the resources / lock
  virtual bool close();

protected:

  bool nextWaypoint(QgsFeature& feature);
  bool nextRoute(QgsFeature& feature);
  bool nextTrack(QgsFeature& feature);

  /**
   * Check to see if the point is withn the selection
   * rectangle
   * @param x X value of point
   * @param y Y value of point
   * @return True if point is within the rectangle
   */
  bool boundsCheck( double x, double y );

  bool boundsCheckRect( double xmin, double ymin, double xmax, double ymax );

  //! Current waypoint iterator
  QgsGPSData::WaypointIterator mWptIter;
  //! Current route iterator
  QgsGPSData::RouteIterator mRteIter;
  //! Current track iterator
  QgsGPSData::TrackIterator mTrkIter;

  QgsGPXProvider* P;
};

#endif // QGSGPXFEATUREITERATOR_H
