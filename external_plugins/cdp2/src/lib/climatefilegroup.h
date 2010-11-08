/***************************************************************************
                          filegroup.h  -  description
                             -------------------
    begin                : Sat May 10 2003
    copyright            : (C) 2003 by Tim Sutton
    email                : tim@linfiniti.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CLIMATEFILEGROUP_H
#define CLIMATEFILEGROUP_H


#include "climatefilereader.h"
#include <QVector>
#include <QObject>

/** \ingroup library
  * \brief A file group manages a collection of ClimateFileReader objects and is used
  * to step through several filereaders simultaneously.
  * @author Tim Sutton
  */

class CDP_LIB_EXPORT  ClimateFileGroup : public QObject
{
Q_OBJECT;
public:
    /** Constructor for filegroup. Initialises the filereader vector. */
    ClimateFileGroup();
    /** Desctructor - closes each file in the filereader vector and then destroys the vector. */
    ~ClimateFileGroup();
    /** Close all files in the file group */
    void close();
    /** Add a new file reader object to the filegroup and position the fpos_t at the start of the data block  */
    bool add(ClimateFileReader *theReader) ;
    /** Get the next element from each fileReader and return the result as a vector. */
    QVector<float> getElementVector();
    /**
    * Used to find out how meny files are in the file group 
    * @return unsingned int representing the number of files int eh group.
    */
    int fileCount();
    /**
    * Accessor for the mElementCount property. This property tells us
    * how many cells in any one data block in a filereader exist.
    * @return int - The number of cells in a block of the first filereader.
    */
    int elementCount();
        /**
    * Accessor for the mXDim property. This property tells us
    * how many cells in any one data block in the x dimension in a filereader exist.
    * @return int - The number of cells in a block of the first filereader.
    */
    int xDim() {return mXDim;};
        /**
    * Accessor for the mXDim property. This property tells us
    * how many cells in any one data block in the y dimension in a filereader exist.
    * @return int - The number of cells in a block of the first filereader.
    */
    int yDim() {return mYDim;};
    /** Read property of bool mEndOfMatrixFlag. */
    const bool isAtMatrixEnd();
    /** Move to the start of the active data block */
    bool rewind();

signals:
    void error(QString);
    void message (QString);

private:
    /** Type specification for pointer vector for holding file readers. */
    typedef QVector <ClimateFileReader *> ClimateFileReaderVector;
    /**This is the container for all the file readers in this group. */
    ClimateFileReaderVector  mReaderVector;
    /**
    * The number of cells (xdim * ydim) in the block in the first file in the
    * file group. It is assumed that all files in the group have the same block
    * dimensions.
    */
    int mElementCount;
   /**
    * The number of cells in the x dimension in a block in the first file in the
    * file group. It is assumed that all files in the group have the same block
    * dimensions.
    */
    int mXDim;
   /**
    * The number of cells in the y dimension in a block in the first file in the
    * file group. It is assumed that all files in the group have the same block
    * dimensions.
    */
    int mYDim;
    /**
    * A flag to show whether the end of the matrix has been reached.
    * @note the first fileReader in the fileGroup is used to determine this.
    */
    bool mEndOfMatrixFlag;
    
    
};

#endif
