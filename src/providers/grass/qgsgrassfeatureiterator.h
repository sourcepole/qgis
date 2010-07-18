#ifndef QGSGRASSFEATUREITERATOR_H
#define QGSGRASSFEATUREITERATOR_H

#include "qgsvectordataprovider.h"

class QgsGrassProvider;

struct ilist;

class QgsGrassFeatureIterator : public QgsVectorDataProviderIterator
{
public:
  QgsGrassFeatureIterator( QgsGrassProvider* p,
                           QgsAttributeList fetchAttributes,
                           QgsRectangle rect,
                           bool fetchGeometry,
                           bool useIntersect );

  ~QgsGrassFeatureIterator();

  //! fetch next feature, return true on success
  virtual bool nextFeature(QgsFeature& f);

  //! reset the iterator to the starting position
  virtual bool rewind();

  //! end of iterating: free the resources / lock
  virtual bool close();

protected:
  QgsGrassProvider* P;

  struct ilist     *mList;
  struct line_pnts *mPoints; // points structure
  struct line_cats *mCats;   // cats structure

  int    mNextCidx;          // !UPDATE! Next index in cidxFieldIndex to be read, used to find nextFeature

  // selection: array of size nlines or nareas + 1, set to 1 - selected or 0 - not selected, 2 - read
  // Code 2 means that the line was already read in this cycle, all 2 must be reset to 1
  // if getFirstFeature() or select() is calles.
  // Distinction between 1 and 2 is used if attribute table exists, in that case attributes are
  // read from the table and geometry is append and selection set to 2.
  // In the end the selection array is scanned for 1 (attributes missing), and the geometry
  // is returned without attributes
  char    *mSelection;           // !UPDATE!
  int     mSelectionSize;        // !UPDATE! Size of selection array

  /*! Allocate sellection array for given map id. The array is large enough for lines or areas
   *  (bigger from num lines and num areas)
   *  Possible old selection array is not released.
   */
  void allocateSelection();

  void resetSelection( bool sel ); // reset selection

};

#endif // QGSGRASSFEATUREITERATOR_H
