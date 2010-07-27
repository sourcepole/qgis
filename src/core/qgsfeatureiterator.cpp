
#include "qgsfeatureiterator.h"


QgsVectorDataProviderIterator::QgsVectorDataProviderIterator(QgsAttributeList fetchAttributes,
                                                             QgsRectangle rect,
                                                             bool fetchGeometry,
                                                             bool useIntersect )
 : mFetchAttributes(fetchAttributes),
   mFetchGeometry(fetchGeometry),
   mRect(rect),
   mUseIntersect(useIntersect),
   refs(0),
   mClosed(false)
{
}

QgsVectorDataProviderIterator::~QgsVectorDataProviderIterator()
{
}

void QgsVectorDataProviderIterator::ref()
{
  refs++;
}
void QgsVectorDataProviderIterator::deref()
{
  refs--;
  if (!refs)
    delete this;
}


QgsFeatureIterator& QgsFeatureIterator::operator=(const QgsFeatureIterator& other)
{
  if (this != &other)
  {
    if (provIter)
      provIter->deref();
    provIter = other.provIter;
    if (provIter)
      provIter->ref();
  }
  return *this;
}
