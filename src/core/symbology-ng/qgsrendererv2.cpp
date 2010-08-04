
#include "qgsrendererv2.h"
#include "qgssymbolv2.h"
#include "qgssymbollayerv2utils.h"

#include "qgssinglesymbolrendererv2.h" // for default renderer

#include "qgsrendererv2registry.h"

#include "qgsrendercontext.h"
#include "qgsgeometry.h"
#include "qgsfeature.h"
#include "qgslogger.h"
#include "qgsvectorlayer.h"

#include <QDomElement>
#include <QDomDocument>
#include <QPolygonF>



unsigned char* QgsFeatureRendererV2::_getPoint( QPointF& pt, QgsRenderContext& context, unsigned char* wkb )
{
  wkb++; // jump over endian info
  unsigned int wkbType = *(( int* ) wkb );
  wkb += sizeof( unsigned int );

  double x = *(( double * ) wkb ); wkb += sizeof( double );
  double y = *(( double * ) wkb ); wkb += sizeof( double );

  if ( wkbType == QGis::WKBPolygon25D )
    wkb += sizeof( double );

  if ( context.coordinateTransform() )
  {
    double z = 0; // dummy variable for coordiante transform
    context.coordinateTransform()->transformInPlace( x, y, z );
  }

  context.mapToPixel().transformInPlace( x, y );

  pt = QPointF( x, y );
  return wkb;
}

unsigned char* QgsFeatureRendererV2::getLineString( QgsRenderContext& context, unsigned char* wkb )
{
  wkb++; // jump over endian info
  unsigned int wkbType = *(( int* ) wkb );
  wkb += sizeof( unsigned int );
  unsigned int nPoints = *(( int* ) wkb );
  wkb += sizeof( unsigned int );

  bool hasZValue = ( wkbType == QGis::WKBLineString25D );
  double x, y;

  if ( nPoints > mTmpPtsCapacity )
  {
    delete mTmpPts;
    mTmpPts = new QPointF[nPoints];
    mTmpPtsCapacity = nPoints;
  }

  mTmpPtsCount = nPoints;

  const QgsCoordinateTransform* ct = context.coordinateTransform();
  const QgsMapToPixel& mtp = context.mapToPixel();
  double z = 0; // dummy variable for coordiante transform

  for ( unsigned int i = 0; i < nPoints; ++i )
  {
    x = *(( double * ) wkb );
    wkb += sizeof( double );
    y = *(( double * ) wkb );
    wkb += sizeof( double );

    if ( hasZValue ) // ignore Z value
      wkb += sizeof( double );

    // TODO: maybe to the transform at once (faster?)
    if ( ct )
      ct->transformInPlace( x, y, z );
    mtp.transformInPlace( x, y );

    mTmpPts[i] = QPointF( x, y );
  }

  return wkb;
}

static unsigned char* getLinearRing( QPointF* data, int nPoints, QgsRenderContext& context, unsigned char* wkb, bool hasZValue )
{
  double x, y;
  double z = 0; // dummy variable for coordiante transform
  const QgsCoordinateTransform* ct = context.coordinateTransform();
  const QgsMapToPixel& mtp = context.mapToPixel();

  // Extract the points from the WKB and store in a pair of vectors.
  for ( unsigned int jdx = 0; jdx < nPoints; jdx++ )
  {
    x = *(( double * ) wkb ); wkb += sizeof( double );
    y = *(( double * ) wkb ); wkb += sizeof( double );

    // TODO: maybe to the transform at once (faster?)
    if ( ct )
      ct->transformInPlace( x, y, z );
    mtp.transformInPlace( x, y );

    data[jdx] = QPointF( x, y );

    if ( hasZValue )
      wkb += sizeof( double );
  }

  return wkb;
}

unsigned char* QgsFeatureRendererV2::getPolygon( QList<QPolygonF>& holes, QgsRenderContext& context, unsigned char* wkb )
{
  wkb++; // jump over endian info
  unsigned int wkbType = *(( int* ) wkb );
  wkb += sizeof( unsigned int ); // jump over wkb type
  unsigned int numRings = *(( int* ) wkb );
  wkb += sizeof( unsigned int );

  if ( numRings == 0 )  // sanity check for zero rings in polygon
    return wkb;

  bool hasZValue = ( wkbType == QGis::WKBPolygon25D );
  holes.clear();

  for ( unsigned int idx = 0; idx < numRings; idx++ )
  {
    unsigned int nPoints = *(( int* )wkb );
    wkb += sizeof( unsigned int );

    if ( nPoints == 0 )
      continue;

    QPointF* data;
    if ( idx == 0 )
    {
      // first ring: use temporary points array
      if ( nPoints > mTmpPtsCapacity )
      {
        delete mTmpPts;
        mTmpPts = new QPointF[nPoints];
        mTmpPtsCapacity = nPoints;
      }
      mTmpPtsCount = nPoints;

      data = mTmpPts;
    }
    else
    {
      holes.append( QPolygonF( nPoints ) );
      data = holes.last().data();
    }

    wkb = getLinearRing(data, nPoints, context, wkb, hasZValue);
  }

  return wkb;
}


QgsFeatureRendererV2::QgsFeatureRendererV2( QString type )
    : mType( type ), mUsingSymbolLevels( false ),
    mCurrentVertexMarkerType( QgsVectorLayer::Cross ),
    mCurrentVertexMarkerSize( 3 )
{
  mTmpPtsCapacity = 0;
  mTmpPts = NULL;
  mTmpPtsCount = 0;
}

QgsFeatureRendererV2::~QgsFeatureRendererV2()
{
  delete mTmpPts;
}

QgsFeatureRendererV2* QgsFeatureRendererV2::defaultRenderer( QGis::GeometryType geomType )
{
  return new QgsSingleSymbolRendererV2( QgsSymbolV2::defaultSymbol( geomType ) );
}


void QgsFeatureRendererV2::renderFeature( QgsFeature& feature, QgsRenderContext& context, int layer, bool selected, bool drawVertexMarker )
{
  QgsSymbolV2* symbol = symbolForFeature( feature );
  if ( symbol == NULL )
    return;

  QgsSymbolV2::SymbolType symbolType = symbol->type();

  QgsGeometry* geom = feature.geometry();
  switch ( geom->wkbType() )
  {
    case QGis::WKBPoint:
    case QGis::WKBPoint25D:
    {
      if ( symbolType != QgsSymbolV2::Marker )
      {
        QgsDebugMsg( "point can be drawn only with marker symbol!" );
        break;
      }
      QPointF pt;
      _getPoint( pt, context, geom->asWkb() );
      (( QgsMarkerSymbolV2* )symbol )->renderPoint( pt, context, layer, selected );

      //if ( drawVertexMarker )
      //  renderVertexMarker( pt, context );
    }
    break;

    case QGis::WKBLineString:
    case QGis::WKBLineString25D:
    {
      if ( symbolType != QgsSymbolV2::Line )
      {
        QgsDebugMsg( "linestring can be drawn only with line symbol!" );
        break;
      }
      getLineString( context, geom->asWkb() );
      (( QgsLineSymbolV2* )symbol )->renderPolyline( mTmpPts, mTmpPtsCount, context, layer, selected );

      if ( drawVertexMarker )
        renderVertexMarkerPolyline( mTmpPts, mTmpPtsCount, context );
    }
    break;

    case QGis::WKBPolygon:
    case QGis::WKBPolygon25D:
    {
      if ( symbolType != QgsSymbolV2::Fill )
      {
        QgsDebugMsg( "polygon can be drawn only with fill symbol!" );
        break;
      }

      getPolygon( mTmpHoles, context, geom->asWkb() );
      (( QgsFillSymbolV2* )symbol )->renderPolygon( mTmpPts, mTmpPtsCount, ( mTmpHoles.count() ? &mTmpHoles : NULL ), context, layer, selected );

      if ( drawVertexMarker )
        renderVertexMarkerPolygon( mTmpPts, mTmpPtsCount, ( mTmpHoles.count() ? &mTmpHoles : NULL ), context );
    }
    break;

    case QGis::WKBMultiPoint:
    case QGis::WKBMultiPoint25D:
    {
      if ( symbolType != QgsSymbolV2::Marker )
      {
        QgsDebugMsg( "multi-point can be drawn only with marker symbol!" );
        break;
      }

      unsigned char* wkb = geom->asWkb();
      unsigned int num = *(( int* )( wkb + 5 ) );
      unsigned char* ptr = wkb + 9;
      QPointF pt;

      for ( unsigned int i = 0; i < num; ++i )
      {
        ptr = _getPoint( pt, context, ptr );
        (( QgsMarkerSymbolV2* )symbol )->renderPoint( pt, context, layer, selected );

        //if ( drawVertexMarker )
        //  renderVertexMarker( pt, context );
      }
    }
    break;

    case QGis::WKBMultiLineString:
    case QGis::WKBMultiLineString25D:
    {
      if ( symbolType != QgsSymbolV2::Line )
      {
        QgsDebugMsg( "multi-linestring can be drawn only with line symbol!" );
        break;
      }

      unsigned char* wkb = geom->asWkb();
      unsigned int num = *(( int* )( wkb + 5 ) );
      unsigned char* ptr = wkb + 9;

      for ( unsigned int i = 0; i < num; ++i )
      {
        ptr = getLineString( context, ptr );
        (( QgsLineSymbolV2* )symbol )->renderPolyline( mTmpPts, mTmpPtsCount, context, layer, selected );

        if ( drawVertexMarker )
          renderVertexMarkerPolyline( mTmpPts, mTmpPtsCount, context );
      }
    }
    break;

    case QGis::WKBMultiPolygon:
    case QGis::WKBMultiPolygon25D:
    {
      if ( symbolType != QgsSymbolV2::Fill )
      {
        QgsDebugMsg( "multi-polygon can be drawn only with fill symbol!" );
        break;
      }

      unsigned char* wkb = geom->asWkb();
      unsigned int num = *(( int* )( wkb + 5 ) );
      unsigned char* ptr = wkb + 9;

      for ( unsigned int i = 0; i < num; ++i )
      {
        ptr = getPolygon( mTmpHoles, context, ptr );
        (( QgsFillSymbolV2* )symbol )->renderPolygon( mTmpPts, mTmpPtsCount, ( mTmpHoles.count() ? &mTmpHoles : NULL ), context, layer, selected );

        if ( drawVertexMarker )
          renderVertexMarkerPolygon( mTmpPts, mTmpPtsCount, ( mTmpHoles.count() ? &mTmpHoles : NULL ), context );
      }
    }
    break;

    default:
      QgsDebugMsg( "unsupported wkb type for rendering" );
  }
}

QString QgsFeatureRendererV2::dump()
{
  return "UNKNOWN RENDERER\n";
}


QgsFeatureRendererV2* QgsFeatureRendererV2::load( QDomElement& element )
{
  // <renderer-v2 type=""> ... </renderer-v2>

  if ( element.isNull() )
    return NULL;

  // load renderer
  QString rendererType = element.attribute( "type" );

  QgsRendererV2AbstractMetadata* m = QgsRendererV2Registry::instance()->rendererMetadata( rendererType );
  if ( m == NULL )
    return NULL;

  QgsFeatureRendererV2* r = m->createRenderer( element );
  if ( r )
    r->setUsingSymbolLevels( element.attribute( "symbollevels", "0" ).toInt() );

  return r;
}

QDomElement QgsFeatureRendererV2::save( QDomDocument& doc )
{
  // create empty renderer element
  return doc.createElement( RENDERER_TAG_NAME );
}

QgsLegendSymbologyList QgsFeatureRendererV2::legendSymbologyItems( QSize iconSize )
{
  // empty list by default
  return QgsLegendSymbologyList();
}

QgsLegendSymbolList QgsFeatureRendererV2::legendSymbolItems()
{
  return QgsLegendSymbolList();
}

void QgsFeatureRendererV2::setVertexMarkerAppearance( int type, int size )
{
  mCurrentVertexMarkerType = type;
  mCurrentVertexMarkerSize = size;
}

void QgsFeatureRendererV2::renderVertexMarker( const QPointF& pt, QgsRenderContext& context )
{
  QgsVectorLayer::drawVertexMarker( pt.x(), pt.y(), *context.painter(),
                                    ( QgsVectorLayer::VertexMarkerType ) mCurrentVertexMarkerType,
                                    mCurrentVertexMarkerSize );
}

void QgsFeatureRendererV2::renderVertexMarkerPolyline( const QPolygonF& pts, QgsRenderContext& context )
{
  renderVertexMarkerPolyline( pts.constData(), pts.size(), context );
}

void QgsFeatureRendererV2::renderVertexMarkerPolyline( const QPointF* pts, int numPoints, QgsRenderContext& context )
{
  for (int i = 0; i < numPoints; i++)
    renderVertexMarker( pts[i], context );
}

void QgsFeatureRendererV2::renderVertexMarkerPolygon( const QPolygonF& pts, const QList<QPolygonF>* rings, QgsRenderContext& context )
{
  renderVertexMarkerPolygon( pts.constData(), pts.size(), rings, context );
}

void QgsFeatureRendererV2::renderVertexMarkerPolygon( const QPointF* pts, int numPoints, const QList<QPolygonF>* rings, QgsRenderContext& context )
{
  for (int i = 0; i < numPoints; i++)
    renderVertexMarker( pts[i], context );

  if ( rings )
  {
    foreach( const QPolygonF& ring, *rings )
    {
      foreach( const QPointF& pt, ring )
        renderVertexMarker( pt, context );
    }
  }
}
