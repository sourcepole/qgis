/***************************************************************************
 *   Copyright (C) 2005 by Tim Sutton   *
 *   tim@linfiniti.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef _CLIMATEFILEREADER_H_
#define _CLIMATEFILEREADER_H_

#include <QFile>
#include <QString>
#include <QVector>
#include <QMap>
#include <QTextStream>
#include <fstream>
#include <iostream>
//other includes
#include <gdal_priv.h>

/** \ingroup library
 * \brief This class will handle opening a file containing a climate 
 * matrix and iterating through the file in a columnwise / rowwise manner.
 */
class CDP_LIB_EXPORT ClimateFileReader : public QObject
{
  Q_OBJECT;

public:

  //
  // Enumerators
  //
  /**
   * This enum defines the different types of files that can be read in.
   */
  enum FileType { GDAL,
    HADLEY_SRES,
    HADLEY_IS92,
    HADLEY_SRES_MEAN,
    IPCC_OBSERVED,
    VALDES,
    ECHAM4,
    CSIRO_MK2,
    NCAR_CSM_PCM,
    GFDL_R30,
    CGCM2,
    CCSR_AGCM_OGCM,
    CRU_CL1_MONTHLY };


  //
  //   Public methods
  //

  /** Default constructor */
  ClimateFileReader();

  /** Constructor (default ctor is in private section to stop progrmmers using it!)*/
  ClimateFileReader(QString theFileName,FileType theFileType);

  /** Destructor  */
  ~ClimateFileReader();

  /** Setup the file reader 
   * @param theFileNameString the name of the file to be read
   * @param theFileType the file format to be read
   * @return bool true on success
   */
  bool initialise(QString theFileNameString, FileType theFileType);

  /**
   *Get the next available element from the file matrix.
   *The cell index will be advanced by one.
   * @return float - the value at the element at the next cell.
   */
  float getElement();

  /**
   * Accessor for long mCurrentElementNo.
   * Calculated as (currentRowLong * rows) + mCurrentColumn.
   * @return long - the current position in the current block.
   */
  const long currentElementNo();

  /**
   * Accessor property of long currentRowLong.
   * @return long - the current row position in the current block.
   */
  const long currentRow();

  /**
   * Accessor property of long mCurrentColumn.
   * @return long - the current row position in the current block.
   */
  const long currentCol();


  /**
   * Accessor for the int mFileHeaderLines.
   * @return int - the number of header lines before each block
   */
  const int headerLineCount();

  /**
   * Mutator for int mActiveBlockNo.
   * It will move the file pointer too the start of the data block indicated
   * by the start month.
   * This is really only applicable for file formats that include
   * muliple months / years data in a single file such as Hadley SRES data.
   * @param theNewVal - an unsigned int representing the new start block.
   * @return bool - flag indicating success or failure
   */
  bool setActiveBlock( const unsigned int theBlockNo);

  /**
   * Accessor of int mActiveBlockNo.
   * This is really only applicable for file formats that include
   * muliple months / years data in a single file such as Hadley SRES data.
   * @return bool - flag indicating success or failure
   */
  const int activeBlock();


  /**
   * Accessor for bool endOfMatrixFlag.
   * @return bool - Current state of endOfMatrixFlag
   */
  const bool isAtMatrixEnd();


  /**
   * Accessor for QString Filename.
   * @return QString - the current filename
   */
  const QString  filename();



  /**
   * Read property of FileTypeEnum fileType.
   * @note The return type is ClimateFileReader::FileTypeEnum because the calling
   * class does not have the enum in its name space so we need to.
   * explicitly specifiy the namespace.
   * @return ClimateFileReader::FileTypeEnum - the file format of the current file.
   */
  const ClimateFileReader::FileType getFileType();

  /** Read property of long mYDim. */
  const long yDim();
  /** Read property of long mXDim. */
  const long xDim();
  /** Read property of int mBlockHeaderLines. */
  const int blockHeaderLineCount();
  /** Return the various metadata stored for the open file. */
  QString getClimateFileReaderInfo();
  /** Read property of blockStartPos. */
  const int blockStartPos();
  /** Move the internal file pointer to the start of the file header. */
  bool moveToHeader();
  /** The number of blocks available in this file.*/
  const int blockCount();
  /** Find out how many blocks (useful in multiblock formats such as SRES) are in this file. */
  int getNumberOfBlocks();
  /**
   * A helper function to see if the block markers are correct.
   * The value of the first element in each block
   * will be printed to console on std out.
   * @return void - No return.
   */
  void printFirstCellInEachBlock();
  /**
   * A helper function to see if the block markers are correct.
   * The value of the last element in each block
   * will be printed to console on std out.
   * @return void - No return.
   */

  void printLastCellInEachBlock();
  /**
   * A helper function to see if the block markers are correct.
   * The value of each block marker file seek position will be printed.
   * @return void - No return.
   */
  void printBlockMarkers();
  /** A helper function to show a whole block.
   * @note This will likely be removed!
   */
  void printBlock(int theBlock);
  /** Get a world file based on the file reader. This is useful when you 
      are writing out a new file and want it to have the same positional info
      and dimensions as the original input file.
    */
   const QString  getWorldFile();

  /** Get an ascii grid header file based on the file reader. This is useful when you 
      are writing out a new file and want it to have the same positional info
      and dimensions as the original input file.
    */
   const QString  getAsciiHeader();
signals:
  void error (QString theError);
  void message (QString theMessage);

private:

  /**
   * Mutator of FileTypeEnum fileType.
   * @note You should specify the file type BEFORE opening the file.
   * @param theNewVal - a FileTypeEnum specifying the input file type.
   * @return bool - flag indicating success or failure
   */
  bool setFileType( const FileType theNewVal);
  /**
   * Use the header info for a given file type to determine the
   * begining of the data block(s) and position the
   * blockStartPos there. This method will need to be called
   * explicitly by the client app so that when multiple
   * copies of the same file are being opened, we dont need to
   * do the same thing each time.
   * @param forceFlag - Force parsing file for block markers. By default this
   * is set to false and the file will only be parsed if an accompanying
   * .bmr file is found.
   * @return QVector <int> - a qvalue vector contining a series of
   * file offsets (ulongs) which mark the start of each data block.
   */
  QVector <int> getBlockMarkers(bool forceFlag=false);

  /** A helper method for gdal reading to get a value into the correct type.
   * @param theData a scanline of data from gdal.
   * @param theType the gdal datatype of data in this scanline
   * @param theIndex Postion inthe scanline to read from.
   * @return float containing the value we read in
   */
  double readValue ( void *theData, GDALDataType theType, int theIndex );

  //
  //   Private attributes
  //
  /** This is a maximum length for lines that shall be skipped */
  static const unsigned int mMaxLineLength=100;
  /** This is the xDim (columns) of a matrix for one month (files may contain more than one matrix). */
  unsigned int mXDim;
  /** Number of rows in matrix for one month / year */
  unsigned int mYDim;
  /** Type of file we are reading. */
  FileType mFileType;
  /** The name of the file, including full path if neccessary. */
  QString mFileName;
  /** Whether the file pointer has reached the end of the matrix */
  bool mEndOfMatrixFlag;
  /** Block in file that matrix extraction should start at. */
  int mActiveBlockNo;
  /** Number of header lines specifically at start of file. */
  int mFileHeaderLines;
  /** Current column in the matrix */
  unsigned int mCurrentColumn;
  /** Current row in matrix */
  unsigned int mCurrentRow;
  /** Position in the matrix expressed as (current row * cols) + current col */
  unsigned int mCurrentElementNo;
  /** The text stream that will be used to pull data from the file */
  std::ifstream  mTextStream;
  /** This is a buffer for holding read in lines */
  char mBuffer[mMaxLineLength];
  /** if the file type is a gdal dataset we will store the handle to the dataset here rather than in the text stream */
  GDALDataset *  mGdalDataset;

  /** Number of header lines per month data block (applicable to files containing multiple months in a single file only. */
  int mBlockHeaderLines;
  /** The offset of the start of the header. This may not be after the header in instances where files contain more the one dataset. */
  int mBlockStartPos;
  /** This is a vector  that stores the filepos for each  datablock in the file*/
  QVector <int> mBlockMarkersVector;


};

#endif
