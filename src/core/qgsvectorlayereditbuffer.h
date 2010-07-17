#ifndef QGSVECTORLAYEREDITBUFFER_H
#define QGSVECTORLAYEREDITBUFFER_H

#include "qgsfeature.h"
#include "qgsfield.h"

class QgsPoint;
class QgsRectangle;
class QgsUndoCommand;
class QgsVectorLayer;
struct QgsSnappingResult;

#include <QObject>
#include <QSet>

// TODO: copied from qgsvectorlayer.h
typedef QList<int> QgsAttributeList;
typedef QSet<int> QgsFeatureIds;
typedef QSet<int> QgsAttributeIds;

/**
 \ingroup core
 Editing buffer for vector layers that records temporarily added/modified/deleted features
 and updates of fields.
 */
class CORE_EXPORT QgsVectorLayerEditBuffer : public QObject
{
  Q_OBJECT

protected:
    /** constructor is protected: an instance can be created only from QgsVectorLayer (it is a friend class) */
    QgsVectorLayerEditBuffer( QgsVectorLayer* vl );

public:

    ~QgsVectorLayerEditBuffer();

    /** Sets whether some features are modified or not */
    void setModified( bool modified = true, bool onlyGeometryWasModified = false );

    /** Returns true if there are any pending changes in the buffer */
    bool isModified();


    //////////////////////////////////
    // vertex editing

    /** Insert a new vertex before the given vertex number,
     *  in the given ring, item (first number is index 0), and feature
     *  Not meaningful for Point geometries
     */
    bool insertVertex( double x, double y, int atFeatureId, int beforeVertex );

    /** Moves the vertex at the given position number,
     *  ring and item (first number is index 0), and feature
     *  to the given coordinates
     */
    bool moveVertex( double x, double y, int atFeatureId, int atVertex );

    /** Deletes a vertex from a feature
     */
    bool deleteVertex( int atFeatureId, int atVertex );

    //////////////////////////////////
    // misc advanced editing functions

    /** Adds a ring to polygon/multipolygon features
     @return
       0 in case of success,
       1 problem with feature type,
       2 ring not closed,
       3 ring not valid,
       4 ring crosses existing rings,
       5 no feature found where ring can be inserted*/
    int addRing( const QList<QgsPoint>& ring );

    /** Adds a new island polygon to a multipolygon feature
     @return
       0 in case of success,
       1 if selected feature is not multipolygon,
       2 if ring is not a valid geometry,
       3 if new polygon ring not disjoint with existing rings,
       4 if no feature was selected,
       5 if several features are selected,
       6 if selected geometry not found*/
    int addIsland( const QList<QgsPoint>& ring );

    /** Translates feature by dx, dy
       @param featureId id of the feature to translate
       @param dx translation of x-coordinate
       @param dy translation of y-coordinate
       @return 0 in case of success*/
    int translateFeature( int featureId, double dx, double dy );

    /** Splits features cut by the given line
       @param splitLine line that splits the layer features
       @param topologicalEditing true if topological editing is enabled
       @return 0 in case of success, 4 if there is a selection but no feature split*/
    int splitFeatures( const QList<QgsPoint>& splitLine, bool topologicalEditing = false );

    //////////////////////////////////
    // topological editing stuff

    /** Adds topological points for every vertex of the geometry
    @param geom the geometry where each vertex is added to segments of other features
    Note: geom is not going to be modified by the function
    @return 0 in case of success*/
    int addTopologicalPoints( QgsGeometry* geom );

    /** Adds a vertex to segments which intersect point p but don't
     already have a vertex there. If a feature already has a vertex at position p,
     no additional vertex is inserted. This method is useful for topological
     editing.
    @param p position of the vertex
    @return 0 in case of success*/
    int addTopologicalPoints( const QgsPoint& p );

    /** Inserts vertices to the snapped segments.
    This is useful for topological editing if snap to segment is enabled.
    @param snapResults results collected from the snapping operation
    @return 0 in case of success*/
    int insertSegmentVerticesForSnap( const QList<QgsSnappingResult>& snapResults );

    //////////////////////////////////
    // modified fields, counts, ...

    /** returns field list in the to-be-committed state */
    const QgsFieldMap &pendingFields() const;

    /** returns list of attributes */
    QgsAttributeList pendingAllAttributesList();

    /** returns feature count after commit */
    int pendingFeatureCount();

    /** layer extent, including added features, modified geometries, excluding deleted features */
    QgsRectangle layerExtent();

    //////////////////////////////////
    // basic feature editing routines

    /** change feature's geometry
      @note added in version 1.2 */
    bool changeGeometry( int fid, QgsGeometry* geom );

    /** changed an attribute value (but does not commit it) */
    bool changeAttributeValue( int fid, int field, QVariant value, bool emitSignal = true );

    /** add an attribute field (but does not commit it)
        returns true if the field was added
      @note added in version 1.2 */
    bool addAttribute( const QgsField &field );

    /** delete an attribute field (but does not commit it) */
    bool deleteAttribute( int attr );

    /** Adds a feature
        @param lastFeatureInBatch  If True, will also go to the effort of e.g. updating the extents.
        @return                    True in case of success and False in case of error
     */
    bool addFeature( QgsFeature& f, bool alsoUpdateExtent = true );

    /** Insert a copy of the given features into the layer  (but does not commit it) */
    bool addFeatures( QgsFeatureList features );

    /** delete a feature from the layer (but does not commit it) */
    bool deleteFeature( int fid );

    /**
      Attempts to commit any changes to disk.  Returns the result of the attempt.
      If a commit fails, the in-memory changes are left alone.

      This allows editing to continue if the commit failed on e.g. a
      disallowed value in a Postgres database - the user can re-edit and try
      again.

      The commits occur in distinct stages,
      (add attributes, add features, change attribute values, change
      geometries, delete features, delete attributes)
      so if a stage fails, it's difficult to roll back cleanly.
      Therefore any error message also includes which stage failed so
      that the user has some chance of repairing the damage cleanly.
     */
    bool commitChanges( QStringList& commitErrors );

    /** Stop editing and discard the edits */
    bool rollBack();

    //////////////////////////////////
    // update features retrieved from provider

    bool featureAtId( int featureId, QgsFeature& f, bool fetchGeometries, bool fetchAttributes );

    /**Update feature with uncommited attribute updates*/
    void updateFeatureAttributes( QgsFeature &f, const QgsAttributeList& fetchAttributes );

    /**Update feature with uncommited geometry updates*/
    void updateFeatureGeometry( QgsFeature &f );


    //////////////////////////////////
    // undo stack commands

    /**
     * Create edit command for undo/redo operations
     * @param text text which is to be displayed in undo window
     */
    void beginEditCommand( QString text );

    /** Finish edit command and add it to undo/redo stack */
    void endEditCommand();

    /** Destroy active command and reverts all changes in it */
    void destroyEditCommand();

    /** Execute undo operation. To be called only from QgsVectorLayerUndoCommand. */
    void undoEditCommand( QgsUndoCommand* cmd );

    /** Execute redo operation. To be called only from QgsVectorLayerUndoCommand. */
    void redoEditCommand( QgsUndoCommand* cmd );

    //////////////////////////////////
    // recording of commands for undo

    /** Record changed geometry, store in active command (if any) */
    void editGeometryChange( int featureId, QgsGeometry& geometry );

    /** Record added feature, store in active command (if any) */
    void editFeatureAdd( QgsFeature& feature );

    /** Record deleted feature, store in active command (if any) */
    void editFeatureDelete( int featureId );

    /** Record changed attribute, store in active command (if any) */
    void editAttributeChange( int featureId, int field, QVariant value );

signals:

    /** This signal is emitted when modifications has been done on layer */
    void layerModified( bool onlyGeometry );

    void editingStarted();
    void editingStopped();
    void attributeAdded( int idx );
    void attributeDeleted( int idx );
    void featureDeleted( int fid );

    void attributeValueChanged( int fid, int idx, const QVariant & );


protected:

    /**Little helper function that gives bounding box from a list of points.
    @return 0 in case of success*/
    int boundingBoxFromPointList( const QList<QgsPoint>& list, double& xmin, double& ymin, double& xmax, double& ymax ) const;

protected:

    /** Flag indicating whether the layer has been modified since the last commit */
    bool mModified;

    /** Deleted feature IDs which are not commited.  Note a feature can be added and then deleted
        again before the change is committed - in that case the added feature would be removed
        from mAddedFeatures only and *not* entered here.
     */
    QgsFeatureIds mDeletedFeatureIds;

    /** New features which are not commited.  Note a feature can be added and then changed,
        therefore the details here can be overridden by mChangedAttributeValues and mChangedGeometries.
     */
    QgsFeatureList mAddedFeatures;

    /** Changed attributes values which are not commited */
    QgsChangedAttributesMap mChangedAttributeValues;

    /** deleted attributes fields which are not commited */
    QgsAttributeIds mDeletedAttributeIds;

    /** added attributes fields which are not commited */
    QgsAttributeIds mAddedAttributeIds;

    /** Changed geometries which are not commited. */
    QgsGeometryMap mChangedGeometries;

    /** field map to commit */
    QgsFieldMap mUpdatedFields;


    /** max field index */
    int mMaxUpdatedIndex;

    QgsUndoCommand * mActiveCommand;


    friend class QgsVectorLayer;
    friend class QgsVectorLayerIterator;

    QgsVectorLayer* L;
  };

#endif // QGSVECTORLAYEREDITBUFFER_H
