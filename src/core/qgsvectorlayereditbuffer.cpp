#include "qgsvectorlayereditbuffer.h"

#include "qgsgeometry.h"
#include "qgslogger.h"
#include "qgsvectorlayer.h"
#include "qgsvectorlayerundocommand.h"

#include <limits>

// used from QgsVectorLayer:
// undoStack()
// crs()
// mCachedGeometries, deleteCachedGeometries()
// getFeatures()
// triggerRepaint()
// updateExtents()
// mSelectedFeatureIds, selectedFeatures()
// mAttributeAliasMap


QgsVectorLayerEditBuffer::QgsVectorLayerEditBuffer(QgsVectorLayer* vl)
  : mModified( false ),
  mMaxUpdatedIndex( -1 ),
  mActiveCommand( NULL ),
  L(vl)
{
  // connect our signals to signals of the vector layer
  connect(this, SIGNAL(layerModified(bool)), L, SIGNAL(layerModified(bool)));
  connect(this, SIGNAL(editingStarted()), L, SIGNAL(editingStarted()));
  connect(this, SIGNAL(editingStopped()), L, SIGNAL(editingStopped()));
  connect(this, SIGNAL(attributeAdded(int)), L, SIGNAL(attributeAdded(int)));
  connect(this, SIGNAL(attributeDeleted(int)), L, SIGNAL(attributeDeleted(int)));
  connect(this, SIGNAL(featureDeleted(int)), L, SIGNAL(featureDeleted(int)));
  connect(this, SIGNAL(attributeValueChanged(int,int,QVariant)), L, SIGNAL(attributeValueChanged(int,int,QVariant)));

  mUpdatedFields = L->mDataProvider->fields();

  mMaxUpdatedIndex = -1;

  for ( QgsFieldMap::const_iterator it = mUpdatedFields.begin(); it != mUpdatedFields.end(); it++ )
    if ( it.key() > mMaxUpdatedIndex )
      mMaxUpdatedIndex = it.key();
}

QgsVectorLayerEditBuffer::~QgsVectorLayerEditBuffer()
{
  mUpdatedFields.clear();
  mMaxUpdatedIndex = -1;
  L->undoStack()->clear();
  emit editingStopped();

  setModified( false );
}

bool QgsVectorLayerEditBuffer::isModified()
{
  return mModified;
}

void QgsVectorLayerEditBuffer::setModified( bool modified, bool onlyGeometry )
{
  mModified = modified;
  emit layerModified( onlyGeometry );
}


void QgsVectorLayerEditBuffer::updateFeatureAttributes( QgsFeature &f, const QgsAttributeList& fetchAttributes )
{
  if ( mChangedAttributeValues.contains( f.id() ) )
  {
    const QgsAttributeMap &map = mChangedAttributeValues[f.id()];
    for ( QgsAttributeMap::const_iterator it = map.begin(); it != map.end(); it++ )
      f.changeAttribute( it.key(), it.value() );
  }

  // remove all attributes that will disappear
  QgsAttributeMap map = f.attributeMap();
  for ( QgsAttributeMap::const_iterator it = map.begin(); it != map.end(); it++ )
    if ( !mUpdatedFields.contains( it.key() ) )
      f.deleteAttribute( it.key() );

  // null/add all attributes that were added, but don't exist in the feature yet
  for ( QgsFieldMap::const_iterator it = mUpdatedFields.begin(); it != mUpdatedFields.end(); it++ )
    if ( !map.contains( it.key() ) && fetchAttributes.contains( it.key() ) )
      f.changeAttribute( it.key(), QVariant( QString::null ) );
}

void QgsVectorLayerEditBuffer::updateFeatureGeometry( QgsFeature &f )
{
  if ( mChangedGeometries.contains( f.id() ) )
    f.setGeometry( mChangedGeometries[f.id()] );
}

const QgsFieldMap &QgsVectorLayerEditBuffer::pendingFields() const
{
   return mUpdatedFields;
}

QgsAttributeList QgsVectorLayerEditBuffer::pendingAllAttributesList()
{
  return mUpdatedFields.keys();
}

int QgsVectorLayerEditBuffer::pendingFeatureCount()
{
  return L->mDataProvider->featureCount()
         + mAddedFeatures.size()
         - mDeletedFeatureIds.size();
}


///////

void QgsVectorLayerEditBuffer::editGeometryChange( int featureId, QgsGeometry& geometry )
{
  if ( mActiveCommand != NULL )
  {
    mActiveCommand->storeGeometryChange( featureId, mChangedGeometries[ featureId ], geometry );
  }
  mChangedGeometries[ featureId ] = geometry;
}


void QgsVectorLayerEditBuffer::editFeatureAdd( QgsFeature& feature )
{
  if ( mActiveCommand != NULL )
  {
    mActiveCommand->storeFeatureAdd( feature );
  }
  mAddedFeatures.append( feature );
}

void QgsVectorLayerEditBuffer::editFeatureDelete( int featureId )
{
  if ( mActiveCommand != NULL )
  {
    mActiveCommand->storeFeatureDelete( featureId );
  }
  mDeletedFeatureIds.insert( featureId );
}

void QgsVectorLayerEditBuffer::editAttributeChange( int featureId, int field, QVariant value )
{
  if ( mActiveCommand != NULL )
  {
    QVariant original;
    bool isFirstChange = true;
    if ( featureId < 0 )
    {
      // work with added feature
      for ( int i = 0; i < mAddedFeatures.size(); i++ )
      {
        if ( mAddedFeatures[i].id() == featureId && mAddedFeatures[i].attributeMap().contains( field ) )
        {
          original = mAddedFeatures[i].attributeMap()[field];
          isFirstChange = false;
          break;
        }
      }
    }
    else
    {
      if ( mChangedAttributeValues.contains( featureId ) && mChangedAttributeValues[featureId].contains( field ) )
      {
        original = mChangedAttributeValues[featureId][field];
        isFirstChange = false;
      }
    }
    mActiveCommand->storeAttributeChange( featureId, field, original, value, isFirstChange );
  }

  if ( featureId >= 0 )
  {
    // changed attribute of existing feature
    if ( !mChangedAttributeValues.contains( featureId ) )
    {
      mChangedAttributeValues.insert( featureId, QgsAttributeMap() );
    }

    mChangedAttributeValues[featureId].insert( field, value );
  }
  else
  {
    // updated added feature
    for ( int i = 0; i < mAddedFeatures.size(); i++ )
    {
      if ( mAddedFeatures[i].id() == featureId )
      {
        mAddedFeatures[i].changeAttribute( field, value );
        break;
      }
    }
  }
}

////////

bool QgsVectorLayerEditBuffer::addFeature( QgsFeature& f, bool alsoUpdateExtent )
{
  static int addedIdLowWaterMark = -1;

  if ( !( L->mDataProvider->capabilities() & QgsVectorDataProvider::AddFeatures ) )
  {
    return false;
  }

  //assign a temporary id to the feature (use negative numbers)
  addedIdLowWaterMark--;

  QgsDebugMsg( "Assigned feature id " + QString::number( addedIdLowWaterMark ) );

  // Force a feature ID (to keep other functions in QGIS happy,
  // providers will use their own new feature ID when we commit the new feature)
  // and add to the known added features.
  f.setFeatureId( addedIdLowWaterMark );
  editFeatureAdd( f );
  L->mCachedGeometries[f.id()] = *f.geometry();

  setModified( true );

  if ( alsoUpdateExtent )
  {
    L->updateExtents();
  }

  return true;
}


bool QgsVectorLayerEditBuffer::addFeatures( QgsFeatureList features )
{
  if ( !( L->mDataProvider->capabilities() & QgsVectorDataProvider::AddFeatures ) )
  {
    return false;
  }

  for ( QgsFeatureList::iterator iter = features.begin(); iter != features.end(); ++iter )
  {
    addFeature( *iter, false );
  }

  L->updateExtents();

  return true;
}


bool QgsVectorLayerEditBuffer::insertVertex( double x, double y, int atFeatureId, int beforeVertex )
{
  QgsGeometry geometry;
  if ( !mChangedGeometries.contains( atFeatureId ) )
  {
    // first time this geometry has changed since last commit
    if ( !L->mCachedGeometries.contains( atFeatureId ) )
    {
      return false;
    }
    geometry = L->mCachedGeometries[atFeatureId];
    //mChangedGeometries[atFeatureId] = L->mCachedGeometries[atFeatureId];
  }
  else
  {
    geometry = mChangedGeometries[atFeatureId];
  }
  geometry.insertVertex( x, y, beforeVertex );
  L->mCachedGeometries[atFeatureId] = geometry;
  editGeometryChange( atFeatureId, geometry );

  setModified( true, true ); // only geometry was changed

  return true;
}


bool QgsVectorLayerEditBuffer::moveVertex( double x, double y, int atFeatureId, int atVertex )
{
  QgsGeometry geometry;
  if ( !mChangedGeometries.contains( atFeatureId ) )
  {
    // first time this geometry has changed since last commit
    if ( !L->mCachedGeometries.contains( atFeatureId ) )
    {
      return false;
    }
    geometry = L->mCachedGeometries[atFeatureId];
    //mChangedGeometries[atFeatureId] = L->mCachedGeometries[atFeatureId];
  }
  else
  {
    geometry = mChangedGeometries[atFeatureId];
  }

  geometry.moveVertex( x, y, atVertex );
  L->mCachedGeometries[atFeatureId] = geometry;
  editGeometryChange( atFeatureId, geometry );

  setModified( true, true ); // only geometry was changed

  return true;
}


bool QgsVectorLayerEditBuffer::deleteVertex( int atFeatureId, int atVertex )
{
  QgsGeometry geometry;
  if ( !mChangedGeometries.contains( atFeatureId ) )
  {
    // first time this geometry has changed since last commit
    if ( !L->mCachedGeometries.contains( atFeatureId ) )
    {
      return false;
    }
    geometry = L->mCachedGeometries[atFeatureId];
  }
  else
  {
    geometry = mChangedGeometries[atFeatureId];
  }

  if ( !geometry.deleteVertex( atVertex ) )
  {
    return false;
  }
  L->mCachedGeometries[atFeatureId] = geometry;
  editGeometryChange( atFeatureId, geometry );

  setModified( true, true ); // only geometry was changed

  return true;
}


////////

int QgsVectorLayerEditBuffer::addRing( const QList<QgsPoint>& ring )
{
  int addRingReturnCode = 5; //default: return code for 'ring not inserted'
  double xMin, yMin, xMax, yMax;
  QgsRectangle bBox;

  if ( boundingBoxFromPointList( ring, xMin, yMin, xMax, yMax ) == 0 )
  {
    bBox.setXMinimum( xMin ); bBox.setYMinimum( yMin );
    bBox.setXMaximum( xMax ); bBox.setYMaximum( yMax );
  }
  else
  {
    return 3; //ring not valid
  }

  QgsFeatureIterator fi = L->getFeatures( QgsAttributeList(), bBox, true, true );

  QgsFeature f;
  while ( fi.nextFeature( f ) )
  {
    addRingReturnCode = f.geometry()->addRing( ring );
    if ( addRingReturnCode == 0 )
    {
      editGeometryChange( f.id(), *f.geometry() );

      setModified( true, true );
      break;
    }
  }

  return addRingReturnCode;
}

int QgsVectorLayerEditBuffer::addIsland( const QList<QgsPoint>& ring )
{
  //number of selected features must be 1

  if ( L->mSelectedFeatureIds.size() < 1 )
  {
    QgsDebugMsg( "Number of selected features <1" );
    return 4;
  }
  else if ( L->mSelectedFeatureIds.size() > 1 )
  {
    QgsDebugMsg( "Number of selected features >1" );
    return 5;
  }

  int selectedFeatureId = *L->mSelectedFeatureIds.constBegin();

  //look if geometry of selected feature already contains geometry changes
  QgsGeometryMap::iterator changedIt = mChangedGeometries.find( selectedFeatureId );
  if ( changedIt != mChangedGeometries.end() )
  {
    QgsGeometry geom = *changedIt;
    int returnValue = geom.addIsland( ring );
    editGeometryChange( selectedFeatureId, geom );
    L->mCachedGeometries[selectedFeatureId] = geom;
    return returnValue;
  }

  //look if id of selected feature belongs to an added feature
#if 0
  for ( QgsFeatureList::iterator addedIt = mAddedFeatures.begin(); addedIt != mAddedFeatures.end(); ++addedIt )
  {
    if ( addedIt->id() == selectedFeatureId )
    {
      return addedIt->geometry()->addIsland( ring );
      L->mCachedGeometries[selectedFeatureId] = *addedIt->geometry();
    }
  }
#endif

  //is the feature contained in the view extent (L->mCachedGeometries) ?
  QgsGeometryMap::iterator cachedIt = L->mCachedGeometries.find( selectedFeatureId );
  if ( cachedIt != L->mCachedGeometries.end() )
  {
    int errorCode = cachedIt->addIsland( ring );
    if ( errorCode == 0 )
    {
      editGeometryChange( selectedFeatureId, *cachedIt );
      L->mCachedGeometries[selectedFeatureId] = *cachedIt;
      setModified( true, true );
    }
    return errorCode;
  }
  else //maybe the selected feature has been moved outside the visible area and therefore is not contained in mCachedGeometries
  {
    QgsFeature f;
    QgsGeometry* fGeom = 0;
    if ( featureAtId( selectedFeatureId, f, true, false ) )
    {
      fGeom = f.geometryAndOwnership();
      if ( fGeom )
      {
        int errorCode = fGeom->addIsland( ring );
        editGeometryChange( selectedFeatureId, *fGeom );
        setModified( true, true );
        delete fGeom;
        return errorCode;
      }
    }
  }

  return 6; //geometry not found
}

int QgsVectorLayerEditBuffer::translateFeature( int featureId, double dx, double dy )
{
  //look if geometry of selected feature already contains geometry changes
  QgsGeometryMap::iterator changedIt = mChangedGeometries.find( featureId );
  if ( changedIt != mChangedGeometries.end() )
  {
    QgsGeometry geom = *changedIt;
    int errorCode = geom.translate( dx, dy );
    editGeometryChange( featureId, geom );
    return errorCode;
  }

  //look if id of selected feature belongs to an added feature
#if 0
  for ( QgsFeatureList::iterator addedIt = mAddedFeatures.begin(); addedIt != mAddedFeatures.end(); ++addedIt )
  {
    if ( addedIt->id() == featureId )
    {
      return addedIt->geometry()->translate( dx, dy );
    }
  }
#endif

  //else look in mCachedGeometries to make access faster
  QgsGeometryMap::iterator cachedIt = L->mCachedGeometries.find( featureId );
  if ( cachedIt != L->mCachedGeometries.end() )
  {
    int errorCode = cachedIt->translate( dx, dy );
    if ( errorCode == 0 )
    {
      editGeometryChange( featureId, *cachedIt );
      setModified( true, true );
    }
    return errorCode;
  }

  //else get the geometry from provider (may be slow)
  QgsFeature f;
  if ( L->mDataProvider->featureAtId( featureId, f, true ) )
  {
    if ( f.geometry() )
    {
      QgsGeometry translateGeom( *( f.geometry() ) );
      int errorCode = translateGeom.translate( dx, dy );
      if ( errorCode == 0 )
      {
        editGeometryChange( featureId, translateGeom );
        setModified( true, true );
      }
      return errorCode;
    }
  }
  return 1; //geometry not found
}

int QgsVectorLayerEditBuffer::splitFeatures( const QList<QgsPoint>& splitLine, bool topologicalEditing )
{
  QgsFeatureList newFeatures; //store all the newly created features
  double xMin, yMin, xMax, yMax;
  QgsRectangle bBox; //bounding box of the split line
  int returnCode = 0;
  int splitFunctionReturn; //return code of QgsGeometry::splitGeometry
  int numberOfSplitedFeatures = 0;

  QgsFeatureList featureList;

  if ( L->mSelectedFeatureIds.size() > 0 )//consider only the selected features if there is a selection
  {
    featureList = L->selectedFeatures();
  }
  else //else consider all the feature that intersect the bounding box of the split line
  {
    if ( boundingBoxFromPointList( splitLine, xMin, yMin, xMax, yMax ) == 0 )
    {
      bBox.setXMinimum( xMin ); bBox.setYMinimum( yMin );
      bBox.setXMaximum( xMax ); bBox.setYMaximum( yMax );
    }
    else
    {
      return 1;
    }

    if ( bBox.isEmpty() )
    {
      //if the bbox is a line, try to make a square out of it
      if ( bBox.width() == 0.0 && bBox.height() > 0 )
      {
        bBox.setXMinimum( bBox.xMinimum() - bBox.height() / 2 );
        bBox.setXMaximum( bBox.xMaximum() + bBox.height() / 2 );
      }
      else if ( bBox.height() == 0.0 && bBox.width() > 0 )
      {
        bBox.setYMinimum( bBox.yMinimum() - bBox.width() / 2 );
        bBox.setYMaximum( bBox.yMaximum() + bBox.width() / 2 );
      }
      else
      {
        return 2;
      }
    }

    QgsFeatureIterator fi = L->getFeatures( pendingAllAttributesList(), bBox, true, true );

    QgsFeature f;
    while ( fi.nextFeature( f ) )
      featureList << QgsFeature( f );
  }

  QgsFeatureList::iterator select_it = featureList.begin();
  for ( ; select_it != featureList.end(); ++select_it )
  {
    QList<QgsGeometry*> newGeometries;
    QList<QgsPoint> topologyTestPoints;
    QgsGeometry* newGeometry = 0;
    splitFunctionReturn = select_it->geometry()->splitGeometry( splitLine, newGeometries, topologicalEditing, topologyTestPoints );
    if ( splitFunctionReturn == 0 )
    {
      //change this geometry
      editGeometryChange( select_it->id(), *( select_it->geometry() ) );
      //update of cached geometries is necessary because we use addTopologicalPoints() later
      L->mCachedGeometries[select_it->id()] = *( select_it->geometry() );

      //insert new features
      for ( int i = 0; i < newGeometries.size(); ++i )
      {
        newGeometry = newGeometries.at( i );
        QgsFeature newFeature;
        newFeature.setGeometry( newGeometry );
        newFeature.setAttributeMap( select_it->attributeMap() );
        newFeatures.append( newFeature );
      }

      setModified( true, true );
      if ( topologicalEditing )
      {
        QList<QgsPoint>::const_iterator topol_it = topologyTestPoints.constBegin();
        for ( ; topol_it != topologyTestPoints.constEnd(); ++topol_it )
        {
          addTopologicalPoints( *topol_it );
        }
      }
      ++numberOfSplitedFeatures;
    }
    else if ( splitFunctionReturn > 1 ) //1 means no split but also no error
    {
      returnCode = 3;
    }
  }

  if ( numberOfSplitedFeatures == 0 && L->mSelectedFeatureIds.size() > 0 )
  {
    //There is a selection but no feature has been split.
    //Maybe user forgot that only the selected features are split
    returnCode = 4;
  }


  //now add the new features to this vectorlayer
  addFeatures( newFeatures );

  return returnCode;
}


////////


bool QgsVectorLayerEditBuffer::changeGeometry( int fid, QgsGeometry* geom )
{
  editGeometryChange( fid, *geom );
  L->mCachedGeometries[fid] = *geom;
  setModified( true, true );
  return true;
}


bool QgsVectorLayerEditBuffer::changeAttributeValue( int fid, int field, QVariant value, bool emitSignal )
{
  editAttributeChange( fid, field, value );
  setModified( true, false );

  if ( emitSignal )
    emit attributeValueChanged( fid, field, value );

  return true;
}

bool QgsVectorLayerEditBuffer::addAttribute( const QgsField &field )
{
  for ( QgsFieldMap::const_iterator it = mUpdatedFields.begin(); it != mUpdatedFields.end(); it++ )
  {
    if ( it.value().name() == field.name() )
      return false;
  }

  if ( !L->mDataProvider->supportedType( field ) )
    return false;

  mMaxUpdatedIndex++;

  if ( mActiveCommand != NULL )
  {
    mActiveCommand->storeAttributeAdd( mMaxUpdatedIndex, field );
  }

  mUpdatedFields.insert( mMaxUpdatedIndex, field );
  mAddedAttributeIds.insert( mMaxUpdatedIndex );

  setModified( true, false );

  emit attributeAdded( mMaxUpdatedIndex );

  return true;
}


bool QgsVectorLayerEditBuffer::deleteAttribute( int index )
{
  if ( mDeletedAttributeIds.contains( index ) )
    return false;

  if ( !mAddedAttributeIds.contains( index ) &&
       !L->mDataProvider->fields().contains( index ) )
    return false;

  if ( mActiveCommand != NULL )
  {
    mActiveCommand->storeAttributeDelete( index, mUpdatedFields[index] );
  }

  mDeletedAttributeIds.insert( index );
  mAddedAttributeIds.remove( index );
  mUpdatedFields.remove( index );
  L->mAttributeAliasMap.remove( index );

  setModified( true, false );

  emit attributeDeleted( index );

  return true;
}

bool QgsVectorLayerEditBuffer::deleteFeature( int fid )
{
  if ( mDeletedFeatureIds.contains( fid ) )
    return true;

  L->mSelectedFeatureIds.remove( fid ); // remove it from selection
  editFeatureDelete( fid );

  setModified( true, false );

  emit featureDeleted( fid );

  return true;
}


///////

void QgsVectorLayerEditBuffer::beginEditCommand( QString text )
{
  if ( mActiveCommand == NULL )
  {
    mActiveCommand = new QgsUndoCommand( L, text );
  }
}

void QgsVectorLayerEditBuffer::endEditCommand()
{
  if ( mActiveCommand != NULL )
  {
    L->undoStack()->push( mActiveCommand );
    mActiveCommand = NULL;
  }

}

void QgsVectorLayerEditBuffer::destroyEditCommand()
{
  if ( mActiveCommand != NULL )
  {
    undoEditCommand( mActiveCommand );
    delete mActiveCommand;
    mActiveCommand = NULL;
  }

}

void QgsVectorLayerEditBuffer::redoEditCommand( QgsUndoCommand* cmd )
{
  QMap<int, QgsUndoCommand::GeometryChangeEntry>& geometryChange = cmd->mGeometryChange;
  QgsFeatureIds& deletedFeatureIdChange = cmd->mDeletedFeatureIdChange;
  QgsFeatureList& addedFeatures = cmd->mAddedFeatures;
  QMap<int, QgsUndoCommand::AttributeChanges>& attributeChange = cmd->mAttributeChange;
  QgsFieldMap& addedAttributes = cmd->mAddedAttributes;
  QgsFieldMap& deletedAttributes = cmd->mDeletedAttributes;


  // geometry changes
  QMap<int, QgsUndoCommand::GeometryChangeEntry>::iterator it = geometryChange.begin();
  for ( ; it != geometryChange.end(); ++it )
  {
    if ( it.value().target == NULL )
    {
      mChangedGeometries.remove( it.key() );
    }
    else
    {
      mChangedGeometries[it.key()] = *( it.value().target );
    }
  }

  // deleted features
  QgsFeatureIds::iterator delIt = deletedFeatureIdChange.begin();
  for ( ; delIt != deletedFeatureIdChange.end(); ++delIt )
  {
    mDeletedFeatureIds.insert( *delIt );
  }

  // added features
  QgsFeatureList::iterator addIt = addedFeatures.begin();
  for ( ; addIt != addedFeatures.end(); ++addIt )
  {
    mAddedFeatures.append( *addIt );
  }

  // changed attributes
  QMap<int, QgsUndoCommand::AttributeChanges>::iterator attrFeatIt = attributeChange.begin();
  for ( ; attrFeatIt != attributeChange.end(); ++attrFeatIt )
  {
    int fid = attrFeatIt.key();
    // for every changed attribute in feature
    QMap<int, QgsUndoCommand::AttributeChangeEntry>::iterator  attrChIt = attrFeatIt.value().begin();
    for ( ; attrChIt != attrFeatIt.value().end(); ++attrChIt )
    {
      if ( fid >= 0 )
      {
        // existing feature
        if ( attrChIt.value().target.isNull() )
        {
          mChangedAttributeValues[fid].remove( attrChIt.key() );
        }
        else
        {
          mChangedAttributeValues[fid][attrChIt.key()] = attrChIt.value().target;
          QgsFeature f;
          featureAtId( fid, f, false, true );
          f.changeAttribute( attrChIt.key(), attrChIt.value().target );
        }
      }
      else
      {
        // added feature
        for ( int i = 0; i < mAddedFeatures.size(); i++ )
        {
          if ( mAddedFeatures[i].id() == fid )
          {
            mAddedFeatures[i].changeAttribute( attrChIt.key(), attrChIt.value().target );
            break;
          }
        }

      }

    }
  }

  // added attributes
  QgsFieldMap::iterator attrIt = addedAttributes.begin();
  for ( ; attrIt != addedAttributes.end(); ++attrIt )
  {
    int attrIndex = attrIt.key();
    mAddedAttributeIds.insert( attrIndex );
    mUpdatedFields.insert( attrIndex, attrIt.value() );
  }

  // deleted attributes
  QgsFieldMap::iterator dAttrIt = deletedAttributes.begin();
  for ( ; dAttrIt != deletedAttributes.end(); ++dAttrIt )
  {
    int attrIndex = dAttrIt.key();
    mDeletedAttributeIds.insert( attrIndex );
    mUpdatedFields.remove( attrIndex );
  }
  setModified( true );

  // it's not ideal to trigger refresh from here
  L->triggerRepaint();
}

void QgsVectorLayerEditBuffer::undoEditCommand( QgsUndoCommand* cmd )
{
  QMap<int, QgsUndoCommand::GeometryChangeEntry>& geometryChange = cmd->mGeometryChange;
  QgsFeatureIds& deletedFeatureIdChange = cmd->mDeletedFeatureIdChange;
  QgsFeatureList& addedFeatures = cmd->mAddedFeatures;
  QMap<int, QgsUndoCommand::AttributeChanges>& attributeChange = cmd->mAttributeChange;
  QgsFieldMap& addedAttributes = cmd->mAddedAttributes;
  QgsFieldMap& deletedAttributes = cmd->mDeletedAttributes;

  // deleted attributes
  QgsFieldMap::iterator dAttrIt = deletedAttributes.begin();
  for ( ; dAttrIt != deletedAttributes.end(); ++dAttrIt )
  {
    int attrIndex = dAttrIt.key();
    mDeletedAttributeIds.remove( attrIndex );
    mUpdatedFields.insert( attrIndex, dAttrIt.value() );
  }

  // added attributes
  QgsFieldMap::iterator attrIt = addedAttributes.begin();
  for ( ; attrIt != addedAttributes.end(); ++attrIt )
  {
    int attrIndex = attrIt.key();
    mAddedAttributeIds.remove( attrIndex );
    mUpdatedFields.remove( attrIndex );
  }

  // geometry changes
  QMap<int, QgsUndoCommand::GeometryChangeEntry>::iterator it = geometryChange.begin();
  for ( ; it != geometryChange.end(); ++it )
  {
    if ( it.value().original == NULL )
    {
      mChangedGeometries.remove( it.key() );
    }
    else
    {
      mChangedGeometries[it.key()] = *( it.value().original );
    }
  }

  // deleted features
  QgsFeatureIds::iterator delIt = deletedFeatureIdChange.begin();
  for ( ; delIt != deletedFeatureIdChange.end(); ++delIt )
  {
    mDeletedFeatureIds.remove( *delIt );
  }

  // added features
  QgsFeatureList::iterator addIt = addedFeatures.begin();
  for ( ; addIt != addedFeatures.end(); ++addIt )
  {
    QgsFeatureList::iterator addedIt = mAddedFeatures.begin();
    for ( ; addedIt != mAddedFeatures.end(); ++addedIt )
    {
      if ( addedIt->id() == addIt->id() )
      {
        mAddedFeatures.erase( addedIt );
        break; // feature was found so move to next one
      }
    }
  }

  // updated attributes
  QMap<int, QgsUndoCommand::AttributeChanges>::iterator attrFeatIt = attributeChange.begin();
  for ( ; attrFeatIt != attributeChange.end(); ++attrFeatIt )
  {
    int fid = attrFeatIt.key();
    QMap<int, QgsUndoCommand::AttributeChangeEntry>::iterator  attrChIt = attrFeatIt.value().begin();
    for ( ; attrChIt != attrFeatIt.value().end(); ++attrChIt )
    {
      if ( fid >= 0 )
      {
        if ( attrChIt.value().isFirstChange )
        {
          mChangedAttributeValues[fid].remove( attrChIt.key() );
        }
        else
        {
          mChangedAttributeValues[fid][attrChIt.key()] = attrChIt.value().original;
        }
      }
      else
      {
        // added feature TODO:
        for ( int i = 0; i < mAddedFeatures.size(); i++ )
        {
          if ( mAddedFeatures[i].id() == fid )
          {
            mAddedFeatures[i].changeAttribute( attrChIt.key(), attrChIt.value().original );
            break;
          }
        }

      }
      emit attributeValueChanged( fid, attrChIt.key(), attrChIt.value().original );
    }
  }
  setModified( true );

  // it's not ideal to trigger refresh from here
  L->triggerRepaint();
}

//////

bool QgsVectorLayerEditBuffer::featureAtId( int featureId, QgsFeature& f, bool fetchGeometries, bool fetchAttributes )
{

  if ( mDeletedFeatureIds.contains( featureId ) )
    return false;

  if ( fetchGeometries && mChangedGeometries.contains( featureId ) )
  {
    f.setFeatureId( featureId );
    f.setValid( true );
    f.setGeometry( mChangedGeometries[featureId] );

    if ( fetchAttributes )
    {
      if ( featureId < 0 )
      {
        // featureId<0 => in mAddedFeatures
        bool found = false;

        for ( QgsFeatureList::iterator it = mAddedFeatures.begin(); it != mAddedFeatures.end(); it++ )
        {
          if ( featureId != it->id() )
          {
            found = true;
            f.setAttributeMap( it->attributeMap() );
            break;
          }
        }

        if ( !found )
          QgsDebugMsg( QString( "No attributes for the added feature %1 found" ).arg( f.id() ) );
      }
      else
      {
        // retrieve attributes from provider
        QgsFeature tmp;
        L->mDataProvider->featureAtId( featureId, tmp, false, L->mDataProvider->attributeIndexes() );
        f.setAttributeMap( tmp.attributeMap() );
      }
      updateFeatureAttributes( f, pendingAllAttributesList() );
    }
    return true;
  }

  //added features
  for ( QgsFeatureList::iterator iter = mAddedFeatures.begin(); iter != mAddedFeatures.end(); ++iter )
  {
    if ( iter->id() == featureId )
    {
      f.setFeatureId( iter->id() );
      f.setValid( true );
      if ( fetchGeometries )
        f.setGeometry( *iter->geometry() );

      if ( fetchAttributes )
        f.setAttributeMap( iter->attributeMap() );

      return true;
    }
  }

  // regular features
  if ( fetchAttributes )
  {
    if ( L->mDataProvider->featureAtId( featureId, f, fetchGeometries, L->mDataProvider->attributeIndexes() ) )
    {
      updateFeatureAttributes( f, pendingAllAttributesList() );
      return true;
    }
  }
  else
  {
    if ( L->mDataProvider->featureAtId( featureId, f, fetchGeometries, QgsAttributeList() ) )
    {
      return true;
    }
  }
  return false;
}


////////

QgsRectangle QgsVectorLayerEditBuffer::layerExtent()
{
  QgsRectangle extent;
  extent.setMinimal();

  if ( mDeletedFeatureIds.isEmpty() && mChangedGeometries.isEmpty() )
  {
    // get the extent of the layer from the provider
    // but only when there are some features already
    if ( L->mDataProvider->featureCount() != 0 )
    {
      QgsRectangle r = L->mDataProvider->extent();
      extent.combineExtentWith( &r );
    }

    for ( QgsFeatureList::iterator it = mAddedFeatures.begin(); it != mAddedFeatures.end(); it++ )
    {
      QgsRectangle r = it->geometry()->boundingBox();
      extent.combineExtentWith( &r );
    }
  }
  else
  {
    QgsFeatureIterator fi = L->getFeatures( QgsAttributeList(), QgsRectangle(), true );

    QgsFeature fet;
    while ( fi.nextFeature( fet ) )
    {
      if ( fet.geometry() )
      {
        QgsRectangle bb = fet.geometry()->boundingBox();
        extent.combineExtentWith( &bb );
      }
    }
  }

  return extent;
}



////////

bool QgsVectorLayerEditBuffer::commitChanges( QStringList& commitErrors )
{
  bool success = true;

  commitErrors.clear();

  int cap = L->mDataProvider->capabilities();

  //
  // delete attributes
  //
  bool attributesChanged = false;
  if ( mDeletedAttributeIds.size() > 0 )
  {
    if (( cap & QgsVectorDataProvider::DeleteAttributes ) && L->mDataProvider->deleteAttributes( mDeletedAttributeIds ) )
    {
      commitErrors << tr( "SUCCESS: %n attribute(s) deleted.", "deleted attributes count", mDeletedAttributeIds.size() );
      mDeletedAttributeIds.clear();
      attributesChanged = true;
    }
    else
    {
      commitErrors << tr( "ERROR: %n attribute(s) not deleted.", "not deleted attributes count", mDeletedAttributeIds.size() );
      success = false;
    }
  }

  //
  // add attributes
  //
  if ( mAddedAttributeIds.size() > 0 )
  {
    QList<QgsField> addedAttributes;
    for ( QgsAttributeIds::const_iterator it = mAddedAttributeIds.begin(); it != mAddedAttributeIds.end(); it++ )
      addedAttributes << mUpdatedFields[ *it ];

    if (( cap & QgsVectorDataProvider::AddAttributes ) && L->mDataProvider->addAttributes( addedAttributes ) )
    {
      commitErrors << tr( "SUCCESS: %n attribute(s) added.", "added attributes count", mAddedAttributeIds.size() );
      mAddedAttributeIds.clear();
      attributesChanged = true;
    }
    else
    {
      commitErrors << tr( "ERROR: %n new attribute(s) not added", "not added attributes count", mAddedAttributeIds.size() );
      success = false;
    }
  }

  //
  // remap changed and attributes of added features
  //
  bool attributeChangesOk = true;
  if ( attributesChanged )
  {
    // map updates field indexes to names
    QMap<int, QString> src;
    for ( QgsFieldMap::const_iterator it = mUpdatedFields.begin(); it != mUpdatedFields.end(); it++ )
    {
      src[ it.key()] = it.value().name();
    }

    int maxAttrIdx = -1;
    const QgsFieldMap &pFields = L->mDataProvider->fields();

    // map provider table names to field indexes
    QMap<QString, int> dst;
    for ( QgsFieldMap::const_iterator it = pFields.begin(); it != pFields.end(); it++ )
    {
      dst[ it.value().name()] = it.key();
      if ( it.key() > maxAttrIdx )
        maxAttrIdx = it.key();
    }

    // if adding attributes failed add fields that are now missing
    // (otherwise we'll loose updates when doing the remapping)
    if ( mAddedAttributeIds.size() > 0 )
    {
      for ( QgsAttributeIds::const_iterator it = mAddedAttributeIds.begin(); it != mAddedAttributeIds.end(); it++ )
      {
        QString name =  mUpdatedFields[ *it ].name();
        if ( dst.contains( name ) )
        {
          // it's there => so we don't need to add it anymore
          mAddedAttributeIds.remove( *it );
          commitErrors << tr( "SUCCESS: attribute %1 was added." ).arg( name );
        }
        else
        {
          // field not there => put it behind the existing attributes
          dst[ name ] = ++maxAttrIdx;
          attributeChangesOk = false;   // don't try attribute updates - they'll fail.
          commitErrors << tr( "ERROR: attribute %1 not added" ).arg( name );
        }
      }
    }

    // map updated fields to provider fields
    QMap<int, int> remap;
    for ( QMap<int, QString>::const_iterator it = src.begin(); it != src.end(); it++ )
    {
      if ( dst.contains( it.value() ) )
      {
        remap[ it.key()] = dst[ it.value()];
      }
    }

    // remap changed attributes
    for ( QgsChangedAttributesMap::iterator fit = mChangedAttributeValues.begin(); fit != mChangedAttributeValues.end(); fit++ )
    {
      QgsAttributeMap &src = fit.value();
      QgsAttributeMap dst;

      for ( QgsAttributeMap::const_iterator it = src.begin(); it != src.end(); it++ )
      {
        if ( remap.contains( it.key() ) )
        {
          dst[ remap[it.key()] ] = it.value();
        }
      }
      src = dst;
    }

    // remap features of added attributes
    for ( QgsFeatureList::iterator fit = mAddedFeatures.begin(); fit != mAddedFeatures.end(); fit++ )
    {
      const QgsAttributeMap &src = fit->attributeMap();
      QgsAttributeMap dst;

      for ( QgsAttributeMap::const_iterator it = src.begin(); it != src.end(); it++ )
        if ( remap.contains( it.key() ) )
          dst[ remap[it.key()] ] = it.value();

      fit->setAttributeMap( dst );
    }

    QgsFieldMap attributes;

    // update private field map
    for ( QMap<int, int>::iterator it = remap.begin(); it != remap.end(); it++ )
      attributes[ it.value()] = mUpdatedFields[ it.key()];

    mUpdatedFields = attributes;
  }

  if ( attributeChangesOk )
  {
    //
    // change attributes
    //
    if ( mChangedAttributeValues.size() > 0 )
    {
      if (( cap & QgsVectorDataProvider::ChangeAttributeValues ) && L->mDataProvider->changeAttributeValues( mChangedAttributeValues ) )
      {
        commitErrors << tr( "SUCCESS: %n attribute value(s) changed.", "changed attribute values count", mChangedAttributeValues.size() );
        mChangedAttributeValues.clear();
      }
      else
      {
        commitErrors << tr( "ERROR: %n attribute value change(s) not applied.", "not changed attribute values count", mChangedAttributeValues.size() );
        success = false;
      }
    }

    //
    //  add features
    //
    if ( mAddedFeatures.size() > 0 )
    {
      for ( int i = 0; i < mAddedFeatures.size(); i++ )
      {
        QgsFeature &f = mAddedFeatures[i];

        if ( mDeletedFeatureIds.contains( f.id() ) )
        {
          mDeletedFeatureIds.remove( f.id() );

          if ( mChangedGeometries.contains( f.id() ) )
            mChangedGeometries.remove( f.id() );

          mAddedFeatures.removeAt( i-- );
          continue;
        }

        if ( mChangedGeometries.contains( f.id() ) )
        {
          f.setGeometry( mChangedGeometries.take( f.id() ) );
        }
      }

      if (( cap & QgsVectorDataProvider::AddFeatures ) && L->mDataProvider->addFeatures( mAddedFeatures ) )
      {
        commitErrors << tr( "SUCCESS: %n feature(s) added.", "added features count", mAddedFeatures.size() );
        mAddedFeatures.clear();
      }
      else
      {
        commitErrors << tr( "ERROR: %n feature(s) not added.", "not added features count", mAddedFeatures.size() );
        success = false;
      }
    }
  }

  //
  // update geometries
  //
  if ( mChangedGeometries.size() > 0 )
  {
    if (( cap & QgsVectorDataProvider::ChangeGeometries ) && L->mDataProvider->changeGeometryValues( mChangedGeometries ) )
    {
      commitErrors << tr( "SUCCESS: %n geometries were changed.", "changed geometries count", mChangedGeometries.size() );
      mChangedGeometries.clear();
    }
    else
    {
      commitErrors << tr( "ERROR: %n geometries not changed.", "not changed geometries count", mChangedGeometries.size() );
      success = false;
    }
  }

  //
  // delete features
  //
  if ( mDeletedFeatureIds.size() > 0 )
  {
    if (( cap & QgsVectorDataProvider::DeleteFeatures ) && L->mDataProvider->deleteFeatures( mDeletedFeatureIds ) )
    {
      commitErrors << tr( "SUCCESS: %n feature(s) deleted.", "deleted features count", mDeletedFeatureIds.size() );
      for ( QgsFeatureIds::const_iterator it = mDeletedFeatureIds.begin(); it != mDeletedFeatureIds.end(); it++ )
      {
        mChangedAttributeValues.remove( *it );
        mChangedGeometries.remove( *it );
      }
      mDeletedFeatureIds.clear();
    }
    else
    {
      commitErrors << tr( "ERROR: %n feature(s) not deleted.", "not deleted features count", mDeletedFeatureIds.size() );
      success = false;
    }
  }

  L->deleteCachedGeometries();

  L->mDataProvider->updateExtents();

  L->triggerRepaint();

  QgsDebugMsg( "result:\n  " + commitErrors.join( "\n  " ) );

  return success;
}

bool QgsVectorLayerEditBuffer::rollBack()
{
  if ( isModified() )
  {
    while ( mAddedAttributeIds.size() > 0 )
    {
      int idx = *mAddedAttributeIds.begin();
      mAddedAttributeIds.remove( idx );
      mUpdatedFields.remove( idx );
      emit attributeDeleted( idx );
    }

    while ( mDeletedAttributeIds.size() > 0 )
    {
      int idx = *mDeletedAttributeIds.begin();
      mDeletedAttributeIds.remove( idx );
      emit attributeAdded( idx );
    }

    // roll back changed attribute values
    mChangedAttributeValues.clear();

    // roll back changed geometries
    mChangedGeometries.clear();

    // Roll back added features
    // Delete the features themselves before deleting the references to them.
    mAddedFeatures.clear();

    // Roll back deleted features
    mDeletedFeatureIds.clear();

    // clear private field map
    mUpdatedFields.clear();
    mMaxUpdatedIndex = -1;
  }

  L->deleteCachedGeometries();

  L->undoStack()->clear();

  emit editingStopped();

  setModified( false );
  L->triggerRepaint();

  return true;
}

////

int QgsVectorLayerEditBuffer::boundingBoxFromPointList( const QList<QgsPoint>& list, double& xmin, double& ymin, double& xmax, double& ymax ) const
{
  if ( list.size() < 1 )
  {
    return 1;
  }

  xmin = std::numeric_limits<double>::max();
  xmax = -std::numeric_limits<double>::max();
  ymin = std::numeric_limits<double>::max();
  ymax = -std::numeric_limits<double>::max();

  for ( QList<QgsPoint>::const_iterator it = list.constBegin(); it != list.constEnd(); ++it )
  {
    if ( it->x() < xmin )
    {
      xmin = it->x();
    }
    if ( it->x() > xmax )
    {
      xmax = it->x();
    }
    if ( it->y() < ymin )
    {
      ymin = it->y();
    }
    if ( it->y() > ymax )
    {
      ymax = it->y();
    }
  }

  return 0;
}

/////

int QgsVectorLayerEditBuffer::addTopologicalPoints( QgsGeometry* geom )
{
  if ( !geom )
  {
    return 1;
  }

  int returnVal = 0;

  QGis::WkbType wkbType = geom->wkbType();

  switch ( wkbType )
  {
      //line
    case QGis::WKBLineString25D:
    case QGis::WKBLineString:
    {
      QgsPolyline theLine = geom->asPolyline();
      QgsPolyline::const_iterator line_it = theLine.constBegin();
      for ( ; line_it != theLine.constEnd(); ++line_it )
      {
        if ( addTopologicalPoints( *line_it ) != 0 )
        {
          returnVal = 2;
        }
      }
      break;
    }

    //multiline
    case QGis::WKBMultiLineString25D:
    case QGis::WKBMultiLineString:
    {
      QgsMultiPolyline theMultiLine = geom->asMultiPolyline();
      QgsPolyline currentPolyline;

      for ( int i = 0; i < theMultiLine.size(); ++i )
      {
        QgsPolyline::const_iterator line_it = currentPolyline.constBegin();
        for ( ; line_it != currentPolyline.constEnd(); ++line_it )
        {
          if ( addTopologicalPoints( *line_it ) != 0 )
          {
            returnVal = 2;
          }
        }
      }
      break;
    }

    //polygon
    case QGis::WKBPolygon25D:
    case QGis::WKBPolygon:
    {
      QgsPolygon thePolygon = geom->asPolygon();
      QgsPolyline currentRing;

      for ( int i = 0; i < thePolygon.size(); ++i )
      {
        currentRing = thePolygon.at( i );
        QgsPolyline::const_iterator line_it = currentRing.constBegin();
        for ( ; line_it != currentRing.constEnd(); ++line_it )
        {
          if ( addTopologicalPoints( *line_it ) != 0 )
          {
            returnVal = 2;
          }
        }
      }
      break;
    }

    //multipolygon
    case QGis::WKBMultiPolygon25D:
    case QGis::WKBMultiPolygon:
    {
      QgsMultiPolygon theMultiPolygon = geom->asMultiPolygon();
      QgsPolygon currentPolygon;
      QgsPolyline currentRing;

      for ( int i = 0; i < theMultiPolygon.size(); ++i )
      {
        currentPolygon = theMultiPolygon.at( i );
        for ( int j = 0; j < currentPolygon.size(); ++j )
        {
          currentRing = currentPolygon.at( j );
          QgsPolyline::const_iterator line_it = currentRing.constBegin();
          for ( ; line_it != currentRing.constEnd(); ++line_it )
          {
            if ( addTopologicalPoints( *line_it ) != 0 )
            {
              returnVal = 2;
            }
          }
        }
      }
      break;
    }
    default:
      break;
  }
  return returnVal;
}

int QgsVectorLayerEditBuffer::addTopologicalPoints( const QgsPoint& p )
{
  QMultiMap<double, QgsSnappingResult> snapResults; //results from the snapper object
  //we also need to snap to vertex to make sure the vertex does not already exist in this geometry
  QMultiMap<double, QgsSnappingResult> vertexSnapResults;

  QList<QgsSnappingResult> filteredSnapResults; //we filter out the results that are on existing vertices

  //work with a tolerance because coordinate projection may introduce some rounding
  double threshold =  0.0000001;
  QGis::UnitType mapUnits = L->crs().mapUnits();
  if ( mapUnits == QGis::Meters )
  {
    threshold = 0.001;
  }
  else if ( mapUnits == QGis::Feet )
  {
    threshold = 0.0001;
  }


  if ( L->snapWithContext( p, threshold, snapResults, QgsSnapper::SnapToSegment ) != 0 )
  {
    return 2;
  }

  QMultiMap<double, QgsSnappingResult>::const_iterator snap_it = snapResults.constBegin();
  QMultiMap<double, QgsSnappingResult>::const_iterator vertex_snap_it;
  for ( ; snap_it != snapResults.constEnd(); ++snap_it )
  {
    //test if p is already a vertex of this geometry. If yes, don't insert it
    bool vertexAlreadyExists = false;
    if ( L->snapWithContext( p, threshold, vertexSnapResults, QgsSnapper::SnapToVertex ) != 0 )
    {
      continue;
    }

    vertex_snap_it = vertexSnapResults.constBegin();
    for ( ; vertex_snap_it != vertexSnapResults.constEnd(); ++vertex_snap_it )
    {
      if ( snap_it.value().snappedAtGeometry == vertex_snap_it.value().snappedAtGeometry )
      {
        vertexAlreadyExists = true;
      }
    }

    if ( !vertexAlreadyExists )
    {
      filteredSnapResults.push_back( *snap_it );
    }
  }
  insertSegmentVerticesForSnap( filteredSnapResults );
  return 0;
}

int QgsVectorLayerEditBuffer::insertSegmentVerticesForSnap( const QList<QgsSnappingResult>& snapResults )
{
  int returnval = 0;
  QgsPoint layerPoint;

  QList<QgsSnappingResult>::const_iterator it = snapResults.constBegin();
  for ( ; it != snapResults.constEnd(); ++it )
  {
    if ( it->snappedVertexNr == -1 ) // segment snap
    {
      layerPoint = it->snappedVertex;
      if ( !insertVertex( layerPoint.x(), layerPoint.y(), it->snappedAtGeometry, it->afterVertexNr ) )
      {
        returnval = 3;
      }
    }
  }
  return returnval;
}

