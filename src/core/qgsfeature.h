/***************************************************************************
                      qgsfeature.h - Spatial Feature Class
                     --------------------------------------
Date                 : 09-Sep-2003
Copyright            : (C) 2003 by Gary E.Sherman
email                : sherman at mrcc.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#ifndef QGSFEATURE_H
#define QGSFEATURE_H

#include <QMap>
#include <QString>
#include <QVariant>
#include <QList>
#include <QVector>

#include <QSharedDataPointer>

class QgsGeometry;
class QgsRectangle;
class QgsFeature;


typedef QVector<QVariant> QgsAttributeVector;

// key = field index, value = field value
typedef QMap<int, QVariant> QgsAttributeMap;

// key = feature id, value = changed attributes
typedef QMap<int, QgsAttributeMap> QgsChangedAttributesMap;

// key = feature id, value = changed geometry
typedef QMap<int, QgsGeometry> QgsGeometryMap;

// key = field index, value = field name
typedef QMap<int, QString> QgsFieldNameMap;

typedef QList<QgsFeature> QgsFeatureList;


#include <QSharedData>

class QgsFeatureData : public QSharedData
{
  public:

    QgsFeatureData( int id );
    QgsFeatureData( const QgsFeatureData & rhs );

    ~QgsFeatureData();

    void setGeometry( QgsGeometry* geom );

    void clearAttributes();
    void setAttributes( const QgsAttributeMap& attributes );
    void setAttributes( const QgsAttributeVector& attributes );
    void addAttribute( int field, QVariant attr );
    void deleteAttribute( int field );
    void changeAttribute( int field, QVariant attr );

    void convertVectorToMap() const;
    void convertMapToVector();

    //! feature id
    int mFid;

    /** map of attributes accessed by field index */
    mutable QgsAttributeMap mAttributes;
    mutable bool mAttributeMapDirty;

    /** vector of attributes */
    QgsAttributeVector mAttributeVector;

    /** pointer to geometry */
    QgsGeometry *mGeometry;

    /** Indicator if the mGeometry is owned by this QgsFeature.
        If so, this QgsFeature takes responsibility for the mGeometry's destruction.
     */
    bool mOwnsGeometry;

    //! Flag to indicate if this feature is valid
    bool mValid;
};



/** \ingroup core
 * The feature class encapsulates a single feature including its id,
 * geometry and a list of field/values attributes.
 *
 * @author Gary E.Sherman
 */
class CORE_EXPORT QgsFeature
{
  public:
    //! Constructor
    QgsFeature( int id = 0, QString typeName = QString() );

    /** copy ctor needed due to internal pointer */
    QgsFeature( const QgsFeature & rhs );

    //! Destructor
    ~QgsFeature();


    /**
     * Get the feature id for this feature
     * @return Feature id
     */
    int id() const;

    /**
     * Set the feature id for this feature
     * @param id Feature id
     */
    void setFeatureId( int id );

    /**
     * Get the attributes for this feature.
     * @return A QMap containing the field name/value mapping
     */
    const QgsAttributeMap& attributeMap() const;

    /**Sets all the attributes in one go*/
    void setAttributeMap( const QgsAttributeMap& attributeMap );

    /** Clear attribute map
     * added in 1.5
     */
    void clearAttributeMap();

    /**
     * Add an attribute to the map
     */
    void addAttribute( int field, QVariant attr );

    /**Deletes an attribute and its value*/
    void deleteAttribute( int field );

    /**Changes an existing attribute value
       @param field index of the field
       @param attr attribute name and value to be set */
    void changeAttribute( int field, QVariant attr );

    QVariant* resizeAttributeVector( int fieldCount );
    void setAttributeVector( const QgsAttributeVector& attrList );
    const QgsAttributeVector& attributeVector() const;

    /**
     * Return the validity of this feature. This is normally set by
     * the provider to indicate some problem that makes the feature
     * invalid or to indicate a null feature.
     */
    bool isValid() const;

    /**
     * Set the validity of the feature.
     */
    void setValid( bool validity );

    /** @deprecated not used anymore */
    QString typeName() const;
    /** @deprecated not used anymore */
    void setTypeName( QString typeName );
    /** @deprecated not used anymore */
    bool isDirty() const;
    /** @deprecated not used anymore */
    void clean();

    /**
     * Get the geometry object associated with this feature
     */
    QgsGeometry *geometry();

    /**
     * Get the geometry object associated with this feature
     * The caller assumes responsibility for the QgsGeometry*'s destruction.
     */
    QgsGeometry *geometryAndOwnership();

    /** Set this feature's geometry from another QgsGeometry object (deep copy)
     */
    void setGeometry( const QgsGeometry& geom );

    /** Set this feature's geometry (takes geometry ownership)
     */
    void setGeometry( QgsGeometry* geom );

    /**
     * Set this feature's geometry from WKB
     *
     * This feature assumes responsibility for destroying geom.
     */
    void setGeometryAndOwnership( unsigned char * geom, size_t length );

  private:

    QSharedDataPointer<QgsFeatureData> d;

}; // class QgsFeature


#endif
