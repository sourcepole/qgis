/***************************************************************************
    qgsmaprender.h  -  class for rendering map layer set
    ----------------------
    begin                : January 2006
    copyright            : (C) 2006 by Martin Dobias
    email                : wonder.sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#ifndef QGSMAPRENDER_H
#define QGSMAPRENDER_H

#include <QSize>
#include <QStringList>
#include <QFuture>
#include <QFutureWatcher>
#include <QTime>
#include <QImage>
#include <QFlags>

#include "qgis.h"
#include "qgsrectangle.h"
#include "qgsrendercontext.h"
#include "qgscoordinatereferencesystem.h"

class QDomDocument;
class QDomNode;
class QPainter;

class QgsMapToPixel;
class QgsMapLayer;
class QgsMapRenderer;
class QgsDistanceArea;
class QgsOverlayObjectPositionManager;
class QgsVectorLayer;
class QgsFeature;


typedef struct ThreadedRenderContext
{
  QgsMapRenderer* mr; // renderer that governs the rendering
  QgsMapLayer* ml; // source map layer
  QImage* img; // destination image
  QgsRenderContext ctx; // private render context
  bool cached; // whether the image is retrieved from cache (= rendering not necessary)
} ThreadedRenderContext;


/** Labeling engine interface.
 * \note Added in QGIS v1.4
 */
class QgsLabelingEngineInterface
{
  public:
    virtual ~QgsLabelingEngineInterface() {}

    //! called when we're going to start with rendering
    virtual void init( QgsMapRenderer* mp ) = 0;
    //! called to find out whether the layer is used for labeling
    virtual bool willUseLayer( QgsVectorLayer* layer ) = 0;
    //! called when starting rendering of a layer
    virtual int prepareLayer( QgsVectorLayer* layer, int& attrIndex, QgsRenderContext& ctx ) = 0;
    //! called for every feature
    virtual void registerFeature( QgsVectorLayer* layer, QgsFeature& feat, const QgsRenderContext& context = QgsRenderContext() ) = 0;
    //! called when the map is drawn and labels should be placed
    virtual void drawLabeling( QgsRenderContext& context ) = 0;
    //! called when we're done with rendering
    virtual void exit() = 0;

    //! called when passing engine among map renderers
    virtual QgsLabelingEngineInterface* clone() = 0;
};



class CORE_EXPORT QgsMapRendererCache
{
  public:

    QgsMapRendererCache();

    //! invalidate the cache contents
    void clear();

    //! initialize cache: set new parameters and erase cache if parameters have changed
    //! @return flag whether the extent and other factors are the same as last time
    bool init(QgsRectangle extent, double scale, double scaleFactor, double rasterScaleFactor);

    //! set cached image for the specified layer ID
    void setCacheImage(QString layerId, const QImage& img);

    //! get cached image for the specified layer ID. Returns null image if it is not cached.
    QImage cacheImage(QString layerId);

  protected:
    QgsRectangle mExtent;
    double mScale;
    double mScaleFactor, mRasterScaleFactor;
    QMap<QString, QImage> mCachedImages;
};



/** \ingroup core
 * A non GUI class for rendering a map layer set onto a QPainter.
 */

class CORE_EXPORT QgsMapRenderer : public QObject
{
    Q_OBJECT

  public:

    /**Output units for pen width and point marker width/height*/
    enum OutputUnits
    {
      Millimeters,
      Pixels
      //MAP_UNITS probably supported in future versions
    };

    //! constructor
    QgsMapRenderer();

    //! destructor
    ~QgsMapRenderer();

    //! starts rendering
    void render( QPainter* painter );

    //! sets extent and checks whether suitable (returns false if not)
    bool setExtent( const QgsRectangle& extent );

    //! returns current extent
    QgsRectangle extent() const;

    const QgsMapToPixel* coordinateTransform() { return &( mRenderContext.mapToPixel() ); }

    double scale() const;
    /**Sets scale for scale based visibility. Normally, the scale is calculated automatically. This
     function is only used to force a preview scale (e.g. for print composer)*/
    void setScale( double scale );
    double mapUnitsPerPixel() const;

    int width() const;
    int height() const;

    //! Recalculate the map scale
    void updateScale();

    //! Return the measuring object
    QgsDistanceArea* distanceArea() { return mDistArea; }
    QGis::UnitType mapUnits() const;
    void setMapUnits( QGis::UnitType u );

    //! sets whether map image will be for overview
    void enableOverviewMode( bool isOverview = true );

    void setOutputSize( QSize size, int dpi );

    //!accessor for output dpi
    int outputDpi();
    //!accessor for output size
    QSize outputSize();

    //! transform extent in layer's CRS to extent in output CRS
    QgsRectangle layerExtentToOutputExtent( QgsMapLayer* theLayer, QgsRectangle extent );

    //! transform coordinates from layer's CRS to output CRS
    QgsPoint layerToMapCoordinates( QgsMapLayer* theLayer, QgsPoint point );

    //! transform coordinates from output CRS to layer's CRS
    QgsPoint mapToLayerCoordinates( QgsMapLayer* theLayer, QgsPoint point );

    //! transform rect's coordinates from output CRS to layer's CRS
    QgsRectangle mapToLayerCoordinates( QgsMapLayer* theLayer, QgsRectangle rect );

    //! sets whether to use projections for this layer set
    void setProjectionsEnabled( bool enabled );

    //! returns true if projections are enabled for this layer set
    bool hasCrsTransformEnabled();

    //! sets destination spatial reference system
    void setDestinationSrs( const QgsCoordinateReferenceSystem& srs );

    //! returns CRS ID of destination spatial reference system
    const QgsCoordinateReferenceSystem& destinationSrs();

    void setOutputUnits( OutputUnits u );

    OutputUnits outputUnits() const;

    //! returns current extent of layer set
    QgsRectangle fullExtent();

    //! returns current layer set
    QStringList& layerSet();

    //! change current layer set
    void setLayerSet( const QStringList& layers );

    //! @deprecated does nothing. Just use fullExtent()
    void updateFullExtent();

    //! read settings
    bool readXML( QDomNode & theNode );

    //! write settings
    bool writeXML( QDomNode & theNode, QDomDocument & theDoc );

    //! Accessor for render context
    QgsRenderContext* rendererContext() {return &mRenderContext;}

    //! Labeling engine (NULL if there's no custom engine)
    //! \note Added in QGIS v1.4
    QgsLabelingEngineInterface* labelingEngine();

    //! Set labeling engine. Previous engine (if any) is deleted.
    //! Takes ownership of the engine.
    //! Added in QGIS v1.4
    void setLabelingEngine( QgsLabelingEngineInterface* iface );

    enum RenderHint {
        Antialiasing = 0x01,
        ForceVectorOutput = 0x02,
        DrawEditingInformation = 0x04,
        NoLabeling = 0x08,
        IgnoreScaleBasedVisibility = 0x10,
        MultipleThreads = 0x20
        // ??? Caching = 0x10,
    };

    Q_DECLARE_FLAGS(RenderHints, RenderHint)

    void setRenderHint( RenderHint hint, bool on = true );
    void setRenderHints( RenderHints hints, bool on = true );
    bool testRenderHint( RenderHint hint ) const;

    //! Enable or disable rendering in multiple threads on multiprocessor computers
    //! Added in QGIS v1.6
    void setThreadingEnabled( bool use );

    //! Determine whether we are using threaded rendering
    //! Added in QGIS v1.6
    bool isThreadingEnabled() const { return mThreadingEnabled; }

    //! Enable or disable caching of rendered layers
    //! Added in QGIS v1.6
    void setCachingEnabled( bool enabled );

    //! Determine whether the rendered layers are cached
    //! Added in QGIS v1.6
    bool isCachingEnabled() const { return mCache != NULL; }

    //! Schedule a redraw of the layers.
    //! This function returns immediately after starting the asynchronous rendering process.
    //! Any previous rendering operations must be finished/canceled first.
    //! Added in QGIS v1.6
    void startThreadedRendering();

    //! Cancel pending asynchronous rendering. Waits until the rendering is canceled.
    //! Does nothing if no asynchronous rendering is running.
    //! Added in QGIS v1.6
    void cancelThreadedRendering();

    //! Return the intermediate output of the asynchronous rendering.
    //! An invalid image is returned when the rendering has finished.
    //! (to obtain the final output image, use finishedThreadedRendering signal)
    //! Added in QGIS v1.6
    QImage threadedRenderingOutput();

    //! Tell whether the rendering takes place
    //! Added in QGIS v1.6
    bool isDrawing() const { return mDrawing; }

    //! if caching is enabled, invalidate the renderer cache
    //! Added in QGIS v1.6
    void clearCache();

  signals:

    //! emitted when asynchronous rendering is finished (or canceled).
    //! The passed image is the result of the rendering process.
    //! Added in QGIS v1.6
    void finishedThreadedRendering(QImage i);

    void drawingProgress( int current, int total );

    void hasCrsTransformEnabled( bool flag );

    void destinationSrsChanged();

    //! @note not used anymore
    void updateMap();

    void mapUnitsChanged();

    //! emitted when layer's draw() returned false
    void drawError( QgsMapLayer* );

  public slots:

    //! called by signal from layer current being drawn
    //! @note currently does nothing
    void onDrawingProgress( int current, int total );

    void futureFinished();

    void futureProgress(int value);

    void clearLayerCache();

    //! called to map layer registry
    void handleLayerRemoval(QString layerId);

  protected:

    //! adjust extent to fit the pixmap size
    void adjustExtentToSize();

    /** Convenience function to project an extent into the layer source
     * CRS, but also split it into two extents if it crosses
     * the +/- 180 degree line. Modifies the given extent to be in the
     * source CRS coordinates, and if it was split, returns true, and
     * also sets the contents of the r2 parameter
     */
    bool splitLayersExtent( QgsMapLayer* layer, QgsRectangle& extent, QgsRectangle& r2 );

    //! initialize the rendering process
    void initRendering( QPainter* painter, double deviceDpi );

    //! finalize the rendering process (labeling, overlays)
    void finishRendering();

    //! render the whole layer set
    void renderLayers();

    //! render one layer
    void renderLayerNoThreading( QgsMapLayer* ml );

    //! schedule rendering of a layer
    void renderLayerThreading( QgsMapLayer* ml );

    //! invoke rendering of the layer with given context
    bool renderLayer( QgsMapLayer* ml, QgsRenderContext& ctx );

    friend void _renderLayerThreading( ThreadedRenderContext& tctx );

    //! render labels for vector layers (not using PAL)
    void renderLabels();

    /**Creates an overlay object position manager subclass according to the current settings
    @note this method was added in version 1.1*/
    QgsOverlayObjectPositionManager* overlayManagerFromSettings();

  protected:

    typedef struct
    {
      //! map units per pixel
      double mapUnitsPerPixel;

      //! Map scale at its current zool level
      double scale;

      //! Scene DPI
      double dpi;

      //! map unit type
      QGis::UnitType mapUnits;

      //! current extent to be drawn
      QgsRectangle extent;

      QSize size;

      //! detemines whether on the fly projection support is enabled
      bool projectionsEnabled;

      //! destination spatial reference system of the projection
      QgsCoordinateReferenceSystem destCRS;

      //! stores array of layers to be rendered (identified by string)
      QStringList layerSet;

      //! Output units
      OutputUnits outputUnits;

      //! Labeling engine (NULL by default)
      QgsLabelingEngineInterface* labelingEngine;

      RenderHints hints;

    } Parameters;

    //! parameters for the next rendering process
    Parameters next;

    //! parameters for the current rendering process
    Parameters curr;

    //! indicates drawing in progress
    bool mDrawing;

    //! tool for measuring
    QgsDistanceArea* mDistArea;

    //! Multithreaded rendering
    bool mThreadingEnabled;

    //! Render caching
    QgsMapRendererCache* mCache;


    QFuture<void> mFuture;
    QFutureWatcher<void> mFW;

    // variables valid only while rendering:
    QTime mRenderTime;
    QgsOverlayObjectPositionManager* mOverlayManager;
    //!Encapsulates context of rendering
    QgsRenderContext mRenderContext;
    QList<ThreadedRenderContext> mThreadedJobs;
};


Q_DECLARE_OPERATORS_FOR_FLAGS(QgsMapRenderer::RenderHints)

#endif

