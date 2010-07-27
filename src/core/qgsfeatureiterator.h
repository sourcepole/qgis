#ifndef QGSFEATUREITERATOR_H
#define QGSFEATUREITERATOR_H

#include <QList>

#include "qgsrectangle.h"

typedef QList<int> QgsAttributeList;

class QgsFeature;

/** \ingroup core
 * Internal feature iterator to be implemented within data providers
 */
class QgsVectorDataProviderIterator
{
public:
  //! base class constructor - stores the iteration parameters
  QgsVectorDataProviderIterator(QgsAttributeList fetchAttributes = QgsAttributeList(),
                                QgsRectangle rect = QgsRectangle(),
                                bool fetchGeometry = true,
                                bool useIntersect = false );

  //! destructor makes sure that the iterator is closed properly
  virtual ~QgsVectorDataProviderIterator();

  //! fetch next feature, return true on success
  virtual bool nextFeature(QgsFeature& f) = 0;
  //! reset the iterator to the starting position
  virtual bool rewind() = 0;
  //! end of iterating: free the resources / lock
  virtual bool close() = 0;

protected:
  QgsAttributeList mFetchAttributes;
  bool mFetchGeometry;
  QgsRectangle mRect;
  bool mUseIntersect;

  bool mClosed;

  // reference counting (to allow seamless copying of QgsFeatureIterator instances)
  int refs;
  void ref(); // add reference
  void deref(); // remove reference, delete if refs == 0
  friend class QgsFeatureIterator;
};


/**
 * \ingroup core
 * Wrapper for iterator of features from vector data provider or vector layer
 */
class QgsFeatureIterator
{
public:
  //! construct invalid iterator
  QgsFeatureIterator();
  //! construct an iterator for iterating an instance of vector data provider
  QgsFeatureIterator(QgsVectorDataProviderIterator* iter);
  //! construct an iterator for iterating an instance of vector layer (data provider + uncommitted data)
  //QgsFeatureIterator(VectorLayerIterator* layerIter);
  //! copy constructor copies the provider iterator, increases ref.count
  QgsFeatureIterator(const QgsFeatureIterator& fi);
  //! destructor deletes the provider iterator if it has no more references
  ~QgsFeatureIterator();

  QgsFeatureIterator& operator=(const QgsFeatureIterator& other);

  bool nextFeature(QgsFeature& f);
  bool rewind();
  bool close();

  //! find out whether the iterator is still valid or closed already
  bool isClosed();

  friend bool operator== (const QgsFeatureIterator &fi1, const QgsFeatureIterator &fi2);
  friend bool operator!= (const QgsFeatureIterator &fi1, const QgsFeatureIterator &fi2);

protected:
  QgsVectorDataProviderIterator* provIter;
};

////////

inline QgsFeatureIterator::QgsFeatureIterator()
  : provIter(NULL)
{
}

inline QgsFeatureIterator::QgsFeatureIterator(QgsVectorDataProviderIterator* iter)
  : provIter(iter)
{
  if (iter)
    iter->ref();
}

inline QgsFeatureIterator::QgsFeatureIterator(const QgsFeatureIterator& fi)
  : provIter(fi.provIter)
{
  if (provIter)
    provIter->ref();
}

inline QgsFeatureIterator::~QgsFeatureIterator()
{
  if (provIter)
    provIter->deref();
}

inline bool QgsFeatureIterator::nextFeature(QgsFeature& f)
{
  return provIter ? provIter->nextFeature(f) : false;
}

inline bool QgsFeatureIterator::rewind()
{
  return provIter ? provIter->rewind() : false;
}

inline bool QgsFeatureIterator::close()
{
  return provIter ? provIter->close() : false;
}

inline bool QgsFeatureIterator::isClosed()
{
  return provIter ? provIter->mClosed : true;
}


inline bool operator== (const QgsFeatureIterator &fi1, const QgsFeatureIterator &fi2)
{
  return (fi1.provIter == fi2.provIter);
}
inline bool operator!= (const QgsFeatureIterator &fi1, const QgsFeatureIterator &fi2)
{
  return !(fi1 == fi2);
}


#endif // QGSFEATUREITERATOR_H
