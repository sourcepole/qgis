
#include "qgsoverlayobjectpositionmanager.h"

#include "qgsvectorlayer.h"
#include "qgsvectoroverlay.h"

void QgsOverlayObjectPositionManager::addOverlaysForLayer( QgsVectorLayer* vl, QgsRenderContext& context )
{
  QList<QgsVectorOverlay*> thisLayerOverlayList;
  vl->vectorOverlays( thisLayerOverlayList );

  QList<QgsVectorOverlay*>::iterator overlayIt = thisLayerOverlayList.begin();
  for ( ; overlayIt != thisLayerOverlayList.end(); ++overlayIt )
  {
    if (( *overlayIt )->displayFlag() )
    {
      ( *overlayIt )->createOverlayObjects( context );
      allOverlayList.push_back( *overlayIt );
    }
  }

  addLayer( vl, thisLayerOverlayList );
}

void QgsOverlayObjectPositionManager::drawOverlays( QgsRenderContext& context, QGis::UnitType unitType )
{
  if ( allOverlayList.isEmpty() )
    return;

  findObjectPositions( context, unitType );

  // draw all the overlays
  QList<QgsVectorOverlay*>::iterator allOverlayIt = allOverlayList.begin();
  for ( ; allOverlayIt != allOverlayList.end(); ++allOverlayIt )
  {
    ( *allOverlayIt )->drawOverlayObjects( context );
  }

  removeLayers();

  allOverlayList.clear();
}
