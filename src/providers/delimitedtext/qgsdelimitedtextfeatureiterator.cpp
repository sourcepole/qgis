#include "qgsdelimitedtextfeatureiterator.h"

#include "qgsdelimitedtextprovider.h"

#include "qgsapplication.h"
#include "qgslogger.h"

#include <QTextStream>

// used from provider:
// - mStream
// - attributeFields
// - mXFieldIndex, mYFieldIndex
// - mInvalidLines, mShowInvalidLines
// - splitLine()

QgsDelimitedTextFeatureIterator::QgsDelimitedTextFeatureIterator( QgsDelimitedTextProvider* p,
                                                                  QgsAttributeList fetchAttributes,
                                                                  QgsRectangle rect,
                                                                  bool fetchGeometry,
                                                                  bool useIntersect )
: QgsVectorDataProviderIterator( fetchAttributes, rect, fetchGeometry, useIntersect ),
  P(p)
{
  P->mStreamMutex.lock();
  rewind();
}

QgsDelimitedTextFeatureIterator::~QgsDelimitedTextFeatureIterator()
{
  close();
}

bool QgsDelimitedTextFeatureIterator::boundsCheck( double x, double y )
{
  // no selection rectangle => always in the bounds
  if ( mRect.isEmpty() )
    return true;

  return ( x <= mRect.xMaximum() ) && ( x >= mRect.xMinimum() ) &&
         ( y <= mRect.yMaximum() ) && ( y >= mRect.yMinimum() );
}


bool QgsDelimitedTextFeatureIterator::nextFeature(QgsFeature& feature)
{
  if (mClosed)
    return false;

  // before we do anything else, assume that there's something wrong with
  // the feature
  feature.setValid( false );
  while ( ! P->mStream->atEnd() )
  {
    double x = 0.0;
    double y = 0.0;
    QString line = P->mStream->readLine(); // Default local 8 bit encoding

    // lex the tokens from the current data line
    QStringList tokens = P->splitLine( line );

    bool xOk = false;
    bool yOk = false;

    // Skip indexing malformed lines.
    if ( P->attributeFields.size() == tokens.size() )
    {
      x = tokens[P->mXFieldIndex].toDouble( &xOk );
      y = tokens[P->mYFieldIndex].toDouble( &yOk );
    }

    if ( !( xOk && yOk ) )
    {
      // Accumulate any lines that weren't ok, to report on them
      // later, and look at the next line in the file, but only if
      // we need to.
      QgsDebugMsg( "Malformed line : " + line );
      if ( P->mShowInvalidLines )
        P->mInvalidLines << line;

      continue;
    }

    // Give every valid line in the file an id, even if it's not
    // in the current extent or bounds.
    ++mFid;             // increment to next feature ID

    // skip the feature if it's out of current bounds
    if ( ! boundsCheck( x, y ) )
      continue;

    // at this point, one way or another, the current feature values
    // are valid
    feature.setValid( true );

    feature.setFeatureId( mFid );

    QByteArray  buffer;
    QDataStream s( &buffer, static_cast<QIODevice::OpenMode>( QIODevice::WriteOnly ) ); // open on buffers's data

    switch ( QgsApplication::endian() )
    {
      case QgsApplication::NDR :
        // we're on a little-endian platform, so tell the data
        // stream to use that
        s.setByteOrder( QDataStream::LittleEndian );
        s << ( quint8 )1; // 1 is for little-endian
        break;
      case QgsApplication::XDR :
        // don't change byte order since QDataStream is big endian by default
        s << ( quint8 )0; // 0 is for big-endian
        break;
      default :
        QgsDebugMsg( "unknown endian" );
        //delete [] geometry;
        return false;
    }

    s << ( quint32 )QGis::WKBPoint;
    s << x;
    s << y;

    unsigned char* geometry = new unsigned char[buffer.size()];
    memcpy( geometry, buffer.data(), buffer.size() );

    feature.setGeometryAndOwnership( geometry, buffer.size() );

    for ( QgsAttributeList::const_iterator i = mFetchAttributes.begin();
          i != mFetchAttributes.end();
          ++i )
    {
      QVariant val;
      switch ( P->attributeFields[*i].type() )
      {
        case QVariant::Int:
          if( !tokens[*i].isEmpty() )
            val = QVariant( tokens[*i].toInt() );
          else
            val = QVariant( P->attributeFields[*i].type() );
          break;
        case QVariant::Double:
          if( !tokens[*i].isEmpty() )
            val = QVariant( tokens[*i].toDouble() );
          else
            val = QVariant( P->attributeFields[*i].type() );
          break;
        default:
          val = QVariant( tokens[*i] );
          break;
      }
      feature.addAttribute( *i, val );
    }

    // We have a good line, so return
    return true;

  } // ! textStream EOF

  // End of the file. If there are any lines that couldn't be
  // loaded, display them now.

  P->showInvalidLinesErrors();

  return false;
}

bool QgsDelimitedTextFeatureIterator::rewind()
{
  if (mClosed)
    return false;

  // Reset feature id to 0
  mFid = 0;

  // Skip ahead one line since first record is always assumed to be
  // the header record
  P->mStream->seek( 0 );
  P->mStream->readLine();

  return true;
}

bool QgsDelimitedTextFeatureIterator::close()
{
  if (mClosed)
    return false;

  P->mStreamMutex.unlock();
  mClosed = true;
  return true;
}
