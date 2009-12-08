
#include "qgsmarkersymbollayerv2.h"
#include "qgssymbollayerv2utils.h"

#include "qgsrendercontext.h"
#include "qgsapplication.h"
#include "qgslogger.h"
#include "qgsproject.h"

#include <QPainter>
#include <QSvgRenderer>
#include <QFileInfo>
#include <QDir>

#include <cmath>

// MSVC compiler doesn't have defined M_PI in math.h
#ifndef M_PI
#define M_PI          3.14159265358979323846
#endif

#define DEG2RAD(x)    ((x)*M_PI/180)


QgsSimpleMarkerSymbolLayerV2::QgsSimpleMarkerSymbolLayerV2( QString name, QColor color, QColor borderColor, double size, double angle )
{
  mName = name;
  mColor = color;
  mBorderColor = borderColor;
  mSize = size;
  mAngle = angle;
  mOffset = QPointF( 0, 0 );
}

QgsSymbolLayerV2* QgsSimpleMarkerSymbolLayerV2::create( const QgsStringMap& props )
{
  QString name = DEFAULT_SIMPLEMARKER_NAME;
  QColor color = DEFAULT_SIMPLEMARKER_COLOR;
  QColor borderColor = DEFAULT_SIMPLEMARKER_BORDERCOLOR;
  double size = DEFAULT_SIMPLEMARKER_SIZE;
  double angle = DEFAULT_SIMPLEMARKER_ANGLE;

  if ( props.contains( "name" ) )
    name = props["name"];
  if ( props.contains( "color" ) )
    color = QgsSymbolLayerV2Utils::decodeColor( props["color"] );
  if ( props.contains( "color_border" ) )
    borderColor = QgsSymbolLayerV2Utils::decodeColor( props["color_border"] );
  if ( props.contains( "size" ) )
    size = props["size"].toDouble();
  if ( props.contains( "angle" ) )
    angle = props["angle"].toDouble();

  QgsSimpleMarkerSymbolLayerV2* m = new QgsSimpleMarkerSymbolLayerV2( name, color, borderColor, size, angle );
  if ( props.contains( "offset" ) )
    m->setOffset( QgsSymbolLayerV2Utils::decodePoint( props["offset"] ) );
  return m;
}


QString QgsSimpleMarkerSymbolLayerV2::layerType() const
{
  return "SimpleMarker";
}

void QgsSimpleMarkerSymbolLayerV2::startRender( QgsRenderContext& context )
{
  mBrush = QBrush( mColor );
  mPen = QPen( mBorderColor );

  mPolygon.clear();

  double half = mSize / 2.0;

  if ( mName == "rectangle" )
  {
    mPolygon = QPolygonF( QRectF( QPointF( -half, -half ), QPointF( half, half ) ) );
  }
  else if ( mName == "diamond" )
  {
    mPolygon << QPointF( -half, 0 ) << QPointF( 0, half )
    << QPointF( half, 0 ) << QPointF( 0, -half );
  }
  else if ( mName == "pentagon" )
  {
    mPolygon << QPointF( half * sin( DEG2RAD( 288.0 ) ), - half * cos( DEG2RAD( 288.0 ) ) )
    << QPointF( half * sin( DEG2RAD( 216.0 ) ), - half * cos( DEG2RAD( 216.0 ) ) )
    << QPointF( half * sin( DEG2RAD( 144.0 ) ), - half * cos( DEG2RAD( 144.0 ) ) )
    << QPointF( half * sin( DEG2RAD( 72.0 ) ), - half * cos( DEG2RAD( 72.0 ) ) )
    << QPointF( 0, - half );
  }
  else if ( mName == "triangle" )
  {
    mPolygon << QPointF( -half, half ) << QPointF( half, half ) << QPointF( 0, -half );
  }
  else if ( mName == "equilateral_triangle" )
  {
    mPolygon << QPointF( half * sin( DEG2RAD( 240.0 ) ), - half * cos( DEG2RAD( 240.0 ) ) )
    << QPointF( half * sin( DEG2RAD( 120.0 ) ), - half * cos( DEG2RAD( 120.0 ) ) )
    << QPointF( 0, -half );
  }
  else if ( mName == "star" )
  {
    double sixth = half / 6;

    mPolygon << QPointF( 0, -half )
    << QPointF( -sixth, -sixth )
    << QPointF( -half, -sixth )
    << QPointF( -sixth, 0 )
    << QPointF( -half, half )
    << QPointF( 0, + sixth )
    << QPointF( half, half )
    << QPointF( + sixth, 0 )
    << QPointF( half, -sixth )
    << QPointF( + sixth, -sixth );
  }
  else if ( mName == "regular_star" )
  {
    double r = half;
    double inner_r = r * cos( DEG2RAD( 72.0 ) ) / cos( DEG2RAD( 36.0 ) );

    mPolygon << QPointF( inner_r * sin( DEG2RAD( 324.0 ) ), - inner_r * cos( DEG2RAD( 324.0 ) ) )  // 324
    << QPointF( r * sin( DEG2RAD( 288.0 ) ) , - r * cos( DEG2RAD( 288 ) ) )    // 288
    << QPointF( inner_r * sin( DEG2RAD( 252.0 ) ), - inner_r * cos( DEG2RAD( 252.0 ) ) )   // 252
    << QPointF( r * sin( DEG2RAD( 216.0 ) ) , - r * cos( DEG2RAD( 216.0 ) ) )   // 216
    << QPointF( 0, inner_r )         // 180
    << QPointF( r * sin( DEG2RAD( 144.0 ) ) , - r * cos( DEG2RAD( 144.0 ) ) )   // 144
    << QPointF( inner_r * sin( DEG2RAD( 108.0 ) ), - inner_r * cos( DEG2RAD( 108.0 ) ) )   // 108
    << QPointF( r * sin( DEG2RAD( 72.0 ) ) , - r * cos( DEG2RAD( 72.0 ) ) )    //  72
    << QPointF( inner_r * sin( DEG2RAD( 36.0 ) ), - inner_r * cos( DEG2RAD( 36.0 ) ) )   //  36
    << QPointF( 0, -half );          //   0
  }
  else if ( mName == "arrow" )
  {
    double eight = half / 4;
    double quarter = half / 2;

    mPolygon << QPointF( 0,        -half )
    << QPointF( quarter,  -quarter )
    << QPointF( eight,    -quarter )
    << QPointF( eight,      half )
    << QPointF( -eight,     half )
    << QPointF( -eight,    -quarter )
    << QPointF( -quarter,  -quarter );
  }
  else
  {
    // some markers can't be drawn as a polygon (circle, cross)
  }

  // rotate if needed
  if ( mAngle != 0 )
    mPolygon = QMatrix().rotate( mAngle ).map( mPolygon );

  // cache the marker
  // TODO: use caching only when drawing to screen (not printer)
  // TODO: decide whether to use QImage or QPixmap - based on the render context

  // calculate necessary image size for the cache
  int pw = (( mPen.width() == 0 ? 1 : mPen.width() ) + 1 ) / 2 * 2; // make even (round up); handle cosmetic pen
  int imageSize = (( int ) mSize + pw ) / 2 * 2 + 1; //  make image width, height odd; account for pen width

  double center = (( double ) imageSize / 2 ) + 0.5; // add 1/2 pixel for proper rounding when the figure's coordinates are added

  mCache = QImage( QSize( imageSize, imageSize ), QImage::Format_ARGB32_Premultiplied );
  mCache.fill( 0 );

  QPainter p;
  p.begin( &mCache );
  p.setRenderHint( QPainter::Antialiasing );
  p.setBrush( mBrush );
  p.setPen( mPen );
  p.translate( QPointF( center, center ) );
  drawMarker( &p );
  p.end();
}

void QgsSimpleMarkerSymbolLayerV2::stopRender( QgsRenderContext& context )
{
}

void QgsSimpleMarkerSymbolLayerV2::renderPoint( const QPointF& point, QgsRenderContext& context )
{
  QPainter* p = context.painter();
  //p->setBrush(mBrush);
  //p->setPen(mPen);

  //p->save();
  //p->translate(point);

  //drawMarker(p);
  double s = mCache.width();
  //if (mCache.isValid())
  p->drawImage( point + QPointF( -s / 2.0, -s / 2.0 ) + mOffset, mCache );

  //p->restore();
}


QgsStringMap QgsSimpleMarkerSymbolLayerV2::properties() const
{
  QgsStringMap map;
  map["name"] = mName;
  map["color"] = QgsSymbolLayerV2Utils::encodeColor( mColor );
  map["color_border"] = QgsSymbolLayerV2Utils::encodeColor( mBorderColor );
  map["size"] = QString::number( mSize );
  map["angle"] = QString::number( mAngle );
  map["offset"] = QgsSymbolLayerV2Utils::encodePoint( mOffset );
  return map;
}

QgsSymbolLayerV2* QgsSimpleMarkerSymbolLayerV2::clone() const
{
  QgsSimpleMarkerSymbolLayerV2* m = new QgsSimpleMarkerSymbolLayerV2( mName, mColor, mBorderColor, mSize, mAngle );
  m->setOffset( mOffset );
  return m;
}

void QgsSimpleMarkerSymbolLayerV2::drawMarker( QPainter* p )
{
  if ( mPolygon.count() != 0 )
  {
    p->drawPolygon( mPolygon );
  }
  else
  {
    double half = mSize / 2.0;
    // TODO: rotate

    if ( mName == "circle" )
    {
      p->drawEllipse( QRectF( -half, -half, half*2, half*2 ) ); // x,y,w,h
    }
    else if ( mName == "cross" )
    {
      p->drawLine( QPointF( -half, 0 ), QPointF( half, 0 ) ); // horizontal
      p->drawLine( QPointF( 0, -half ), QPointF( 0, half ) ); // vertical
    }
    else if ( mName == "cross2" )
    {
      p->drawLine( QPointF( -half, -half ), QPointF( half,  half ) );
      p->drawLine( QPointF( -half,  half ), QPointF( half, -half ) );
    }
  }

}


//////////


QgsSvgMarkerSymbolLayerV2::QgsSvgMarkerSymbolLayerV2( QString name, double size, double angle )
{
  mPath = symbolNameToPath( name );
  mSize = size;
  mAngle = angle;
  mOffset = QPointF( 0, 0 );
}


QgsSymbolLayerV2* QgsSvgMarkerSymbolLayerV2::create( const QgsStringMap& props )
{
  QString name = DEFAULT_SVGMARKER_NAME;
  double size = DEFAULT_SVGMARKER_SIZE;
  double angle = DEFAULT_SVGMARKER_ANGLE;

  if ( props.contains( "name" ) )
    name = props["name"];
  if ( props.contains( "size" ) )
    size = props["size"].toDouble();
  if ( props.contains( "angle" ) )
    angle = props["angle"].toDouble();

  QgsSvgMarkerSymbolLayerV2* m = new QgsSvgMarkerSymbolLayerV2( name, size, angle );
  if ( props.contains( "offset" ) )
    m->setOffset( QgsSymbolLayerV2Utils::decodePoint( props["offset"] ) );
  return m;
}


QString QgsSvgMarkerSymbolLayerV2::layerType() const
{
  return "SvgMarker";
}

void QgsSvgMarkerSymbolLayerV2::startRender( QgsRenderContext& context )
{
  QRectF rect( QPointF( -mSize / 2.0, -mSize / 2.0 ), QSizeF( mSize, mSize ) );
  QSvgRenderer renderer( mPath );
  QPainter painter( &mPicture );
  renderer.render( &painter, rect );
}

void QgsSvgMarkerSymbolLayerV2::stopRender( QgsRenderContext& context )
{
}


void QgsSvgMarkerSymbolLayerV2::renderPoint( const QPointF& point, QgsRenderContext& context )
{
  QPainter* p = context.painter();
  p->save();
  p->translate( point + mOffset );

  if ( mAngle != 0 )
    p->rotate( mAngle );

  p->drawPicture( 0, 0, mPicture );

  if ( mAngle != 0 )
    p->rotate( -mAngle );

  p->restore();
}


QgsStringMap QgsSvgMarkerSymbolLayerV2::properties() const
{
  QgsStringMap map;
  map["name"] = symbolPathToName( mPath );
  map["size"] = QString::number( mSize );
  map["angle"] = QString::number( mAngle );
  map["offset"] = QgsSymbolLayerV2Utils::encodePoint( mOffset );
  return map;
}

QgsSymbolLayerV2* QgsSvgMarkerSymbolLayerV2::clone() const
{
  QgsSvgMarkerSymbolLayerV2* m = new QgsSvgMarkerSymbolLayerV2( mPath, mSize, mAngle );
  m->setOffset( mOffset );
  return m;
}


QStringList QgsSvgMarkerSymbolLayerV2::listSvgFiles()
{
  // copied from QgsMarkerCatalogue - TODO: unify
  QStringList list;
  QStringList svgPaths = QgsApplication::svgPaths();

  for ( int i = 0; i < svgPaths.size(); i++ )
  {
    QDir dir( svgPaths[i] );
    foreach( QString item, dir.entryList( QDir::Dirs | QDir::NoDotAndDotDot ) )
    {
      svgPaths.insert( i + 1, dir.path() + "/" + item );
    }

    foreach( QString item, dir.entryList( QStringList( "*.svg" ), QDir::Files ) )
    {
      // TODO test if it is correct SVG
      list.append( dir.path() + "/" + item );
    }
  }
  return list;
}

QString QgsSvgMarkerSymbolLayerV2::symbolNameToPath( QString name )
{
  // copied from QgsSymbol::setNamedPointSymbol - TODO: unify

  // we might have a full path...
  if ( QFile( name ).exists() )
    return QFileInfo( name ).canonicalFilePath();

  // SVG symbol not found - probably a relative path was used

  QStringList svgPaths = QgsApplication::svgPaths();
  for ( int i = 0; i < svgPaths.size(); i++ )
  {
    QgsDebugMsg( "SvgPath: " + svgPaths[i] );
    QFileInfo myInfo( name );
    QString myFileName = myInfo.fileName(); // foo.svg
    QString myLowestDir = myInfo.dir().dirName();
    QString myLocalPath = svgPaths[i] + "/" + myLowestDir + "/" + myFileName;

    QgsDebugMsg( "Alternative svg path: " + myLocalPath );
    if ( QFile( myLocalPath ).exists() )
    {
      QgsDebugMsg( "Svg found in alternative path" );
      return QFileInfo( myLocalPath ).canonicalFilePath();
    }
    else if ( myInfo.isRelative() )
    {
      QFileInfo pfi( QgsProject::instance()->fileName() );
      QString alternatePath = pfi.canonicalPath() + QDir::separator() + name;
      if ( pfi.exists() && QFile( alternatePath ).exists() )
      {
        QgsDebugMsg( "Svg found in alternative path" );
        return QFileInfo( alternatePath ).canonicalFilePath();
      }
      else
      {
        QgsDebugMsg( "Svg not found in project path" );
      }
    }
    else
    {
      //couldnt find the file, no happy ending :-(
      QgsDebugMsg( "Computed alternate path but no svg there either" );
    }
  }
  return QString();
}

QString QgsSvgMarkerSymbolLayerV2::symbolPathToName( QString path )
{
  // copied from QgsSymbol::writeXML

  QFileInfo fi( path );
  if ( !fi.exists() )
    return path;

  path = fi.canonicalFilePath();

  QStringList svgPaths = QgsApplication::svgPaths();

  for ( int i = 0; i < svgPaths.size(); i++ )
  {
    QString dir = QFileInfo( svgPaths[i] ).canonicalFilePath();

    if ( !dir.isEmpty() && path.startsWith( dir ) )
    {
      path = path.mid( dir.size() );
      break;
    }
  }

  return path;
}
