/***************************************************************************
      qgsdelimitedtextprovider.h  -  Data provider for delimted text
                             -------------------
    begin                : 2004-02-27
    copyright            : (C) 2004 by Gary E.Sherman
    email                : sherman at mrcc.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/* $Id$ */

#include "qgsvectordataprovider.h"

#include <QStringList>
#include <QMutex>

class QgsFeature;
class QgsField;
class QFile;
class QTextStream;


/**
\class QgsDelimitedTextProvider
\brief Data provider for delimited text files.
*
* The provider needs to know both the path to the text file and
* the delimiter to use. Since the means to add a layer is farily
* rigid, we must provide this information encoded in a form that
* the provider can decipher and use.
* The uri must contain the path and delimiter in this format:
* /full/path/too/delimited.txt?delimiter=<delimiter>
*
* Example uri = "/home/foo/delim.txt?delimiter=|"
*/
class QgsDelimitedTextProvider : public QgsVectorDataProvider
{
    Q_OBJECT

  public:

    QgsDelimitedTextProvider( QString uri = QString() );

    virtual ~QgsDelimitedTextProvider();

    /* Implementation of functions from QgsVectorDataProvider */

    /**
     * Returns the permanent storage type for this layer as a friendly name.
     */
    virtual QString storageType() const;

    /**
     * Start iterating over features of the vector data provider.
     * For new code, consider using this method instead of select/nextFeature combo.
     * @param fetchAttributes list of attributes which should be fetched
     * @param rect spatial filter
     * @param fetchGeometry true if the feature geometry should be fetched
     * @param useIntersect true if an accurate intersection test should be used,
     *                     false if a test based on bounding box is sufficient
     * @return iterator instance for retrieval of features
     * @note Added in v1.6
     */
    virtual QgsFeatureIterator getFeatures( QgsAttributeList fetchAttributes = QgsAttributeList(),
                                            QgsRectangle rect = QgsRectangle(),
                                            bool fetchGeometry = true,
                                            bool useIntersect = false );

    /**
     * Get feature type.
     * @return int representing the feature type
     */
    virtual QGis::WkbType geometryType() const;

    /**
     * Number of features in the layer
     * @return long containing number of features
     */
    virtual long featureCount() const;

    /**
     * Number of attribute fields for a feature in the layer
     */
    virtual uint fieldCount() const;

    /**
     * Return a map of indexes with field names for this layer
     * @return map of fields
     */
    virtual const QgsFieldMap & fields() const;

    /** Returns a bitmask containing the supported capabilities
        Note, some capabilities may change depending on whether
        a spatial filter is active on this provider, so it may
        be prudent to check this value per intended operation.
     */
    virtual int capabilities() const;


    /* Implementation of functions from QgsDataProvider */

    /** return a provider name

        Essentially just returns the provider key.  Should be used to build file
        dialogs so that providers can be shown with their supported types. Thus
        if more than one provider supports a given format, the user is able to
        select a specific provider to open that file.

        @note

        Instead of being pure virtual, might be better to generalize this
        behavior and presume that none of the sub-classes are going to do
        anything strange with regards to their name or description?
     */
    QString name() const;

    /** return description

        Return a terse string describing what the provider is.

        @note

        Instead of being pure virtual, might be better to generalize this
        behavior and presume that none of the sub-classes are going to do
        anything strange with regards to their name or description?
     */
    QString description() const;

    /**
     * Return the extent for this data layer
     */
    virtual QgsRectangle extent();

    /**
     * Returns true if this is a valid delimited file
     */
    bool isValid();

    virtual QgsCoordinateReferenceSystem crs();

    /* new functions */


  private:

    //! Fields
    QgsFieldMap mAttributeFields;

    QVector<QgsField> mAttributeVector;

    QgsAttributeList mAttributesToFetch;

    QString mFileName;
    QString mDelimiter;
    QRegExp mDelimiterRegexp;
    QString mDelimiterType;

    int mXFieldIndex;
    int mYFieldIndex;

    //! Layer extent
    QgsRectangle mExtent;

    //! Text file
    QFile *mFile;

    QTextStream *mStream;

    bool mValid;

    long mNumberFeatures;

    //! Storage for any lines in the file that couldn't be loaded
    QStringList mInvalidLines;
    //! Only want to show the invalid lines once to the user
    bool mShowInvalidLines;

    QStringList splitLine( QString line );

    void showInvalidLinesErrors();

    friend class QgsDelimitedTextFeatureIterator;

    QMutex mStreamMutex;
};
