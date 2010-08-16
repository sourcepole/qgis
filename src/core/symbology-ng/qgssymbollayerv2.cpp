
#include "qgssymbollayerv2.h"

#include "qgsrenderer.h"
#include "qgsrendercontext.h"
#include "qgsclipper.h"

#include <QSize>
#include <QPainter>
#include <QPointF>
#include <QPolygonF>



QgsMarkerSymbolLayerV2::QgsMarkerSymbolLayerV2( bool locked )
    : QgsSymbolLayerV2( QgsSymbolV2::Marker, locked )
{
}

QgsLineSymbolLayerV2::QgsLineSymbolLayerV2( bool locked )
    : QgsSymbolLayerV2( QgsSymbolV2::Line, locked )
{
}

QgsFillSymbolLayerV2::QgsFillSymbolLayerV2( bool locked )
    : QgsSymbolLayerV2( QgsSymbolV2::Fill, locked )
{
}

void QgsMarkerSymbolLayerV2::drawPreviewIcon( QgsSymbolV2RenderContext& context, QSize size )
{
  startRender( context );
  renderPoint( QPointF( size.width() / 2, size.height() / 2 ), context );
  stopRender( context );
}

void QgsLineSymbolLayerV2::drawPreviewIcon( QgsSymbolV2RenderContext& context, QSize size )
{
  QPolygonF points;
  // we're adding 0.5 to get rid of blurred preview:
  // drawing antialiased lines of width 1 at (x,0)-(x,100) creates 2px line
  points << QPointF( 0, size.height() / 2 + 0.5 ) << QPointF( size.width(), size.height() / 2 + 0.5 );

  startRender( context );
  renderPolyline( points, context );
  stopRender( context );
}

void QgsLineSymbolLayerV2::renderPolyline( const QPolygonF& points, QgsSymbolV2RenderContext& context )
{
  renderPolyline( points.constData(), points.size(), context );
}


void QgsFillSymbolLayerV2::drawPreviewIcon( QgsSymbolV2RenderContext& context, QSize size )
{
  QPolygonF poly = QRectF( QPointF( 0, 0 ), QPointF( size.width() - 1, size.height() - 1 ) );
  startRender( context );
  renderPolygon( poly, NULL, context );
  stopRender( context );
}

void QgsFillSymbolLayerV2::renderPolygon( const QPolygonF& points, QList<QPolygonF>* rings, QgsSymbolV2RenderContext& context )
{
  renderPolygon( points.constData(), points.size(), rings, context );
}

void QgsFillSymbolLayerV2::_renderPolygon( QPainter* p, const QPolygonF& points, const QList<QPolygonF>* rings )
{
  _renderPolygon( p, points.constData(), points.size(), rings );
}

void QgsFillSymbolLayerV2::_renderPolygon( QPainter* p, const QPointF* points, int numPoints, const QList<QPolygonF>* rings )
{
  if ( !p )
  {
    return;
  }

  if ( rings == NULL )
  {
    // simple polygon without holes
    p->drawPolygon( points, numPoints );
  }
  else
  {
    // polygon with holes must be drawn using painter path
    QPainterPath path;
    QPolygonF outer_ring( numPoints );
    memcpy( outer_ring.data(), points, sizeof(QPointF) * numPoints );

    // clip polygon rings! Qt (as of version 4.6) has a slow algorithm for
    // clipping painter paths: we will clip the rings by ourselves, so
    // the clipping in Qt will not be triggered.
    QgsClipper::trimFeature( outer_ring, false );

    path.addPolygon( outer_ring );

    QList<QPolygonF>::const_iterator it = rings->constBegin();
    for ( ; it != rings->constEnd(); ++it )
    {
      QPolygonF ring = *it;
      QgsClipper::trimFeature( ring, false );
      path.addPolygon( ring );
    }

    p->drawPath( path );
  }
}
