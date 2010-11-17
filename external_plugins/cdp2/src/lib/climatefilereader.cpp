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
#include "climatefilereader.h"


#include <iostream>

//qt includes
#include <QTextStream>


ClimateFileReader::ClimateFileReader() : QObject()
{}

ClimateFileReader::ClimateFileReader(QString theFileName, FileType theFileType) : QObject()
{
  initialise(theFileName, theFileType);
}



ClimateFileReader::~ClimateFileReader()
{
  if (mFileType==GDAL)
  {
    delete mGdalDataset;
  }
  else
  {
    mTextStream.close();
  }

}

bool ClimateFileReader::initialise(QString theFileName, FileType theFileType)
{

  
  mFileName=theFileName;
  if (theFileType==GDAL) //read using gdal api
  {
    GDALAllRegister();
    mGdalDataset = (GDALDataset *) GDALOpen( theFileName.toLocal8Bit(), GA_ReadOnly );
    if( mGdalDataset == NULL )
    {
      emit error ("Error opening " + theFileName + " using GDAL driver");
      return false;
    }
  }
  else //all other formats are read using a text stream
  {
    mTextStream.open (theFileName.toLocal8Bit());
    if ( !mTextStream.is_open() || !mTextStream.good())
    {
      emit error ("Error opening " + theFileName);
      return false;
    }
  }
  setFileType(theFileType);
  mCurrentColumn=0;
  mCurrentRow=0;
  mCurrentElementNo=0;
  mEndOfMatrixFlag=false;
  getBlockMarkers();
  return true;
}




const long ClimateFileReader::xDim()
{
  return mXDim;
}


/** Read property of long mYDim. */

const long ClimateFileReader::yDim()
{
  return mYDim;
}

/** Read property of FileTypeEnum mFileType. */

const ClimateFileReader::FileType ClimateFileReader::getFileType()
{
  return mFileType;
}

/** Write property of FileTypeEnum mFileType. */

bool ClimateFileReader::setFileType( const FileType theFileType)
{
  try
  {
    mFileType = theFileType;
    //Set Hadley member variables
    if ((mFileType == HADLEY_SRES) || (mFileType == HADLEY_IS92))
    {
      mXDim = 96;
      mYDim = 73;
      mFileHeaderLines = 0;
      mBlockHeaderLines = 1;
    }
    else if (mFileType == HADLEY_SRES_MEAN)
    {
      mXDim = 96;
      mYDim = 73;
      mFileHeaderLines = 0;
      mBlockHeaderLines = 5;
    }
    //Set IPCC observed member variables
    else if (mFileType == IPCC_OBSERVED)
    {
      mXDim = 720;
      mYDim = 360;
      mFileHeaderLines = 2;
      mBlockHeaderLines = 0;
    }
    //Set ECHAM4 member variables
    else if (mFileType == ECHAM4)
    {
      mXDim = 128;
      mYDim = 64;
      mFileHeaderLines = 0;
      mBlockHeaderLines = 1;
    }
    //Set CCCma member variables
    else if (mFileType == CGCM2)
    {
      mXDim = 96;
      mYDim = 48;
      mFileHeaderLines = 0;
      mBlockHeaderLines = 1;
    }
    //Set CSIRO_Mk2 member variables
    else if (mFileType == CSIRO_MK2)
    {
      mXDim = 64;
      mYDim = 56;
      mFileHeaderLines = 0;
      mBlockHeaderLines = 1;
    }
    //Set NCAR member variables
    else if (mFileType == NCAR_CSM_PCM)
    {
      mXDim = 128;
      mYDim = 64;
      mFileHeaderLines = 0;
      mBlockHeaderLines = 1;
    }
    //Set GFDL member variables
    else if (mFileType == GFDL_R30)
    {
      mXDim = 96;
      mYDim = 80;
      mFileHeaderLines = 0;
      mBlockHeaderLines = 1;
    }
    //Set CCSRC member variables
    else if (mFileType == CCSR_AGCM_OGCM)
    {
      mXDim = 64;
      mYDim = 32;
      mFileHeaderLines = 0;
      mBlockHeaderLines = 1;
    }
    //Set CRU member variables
    else if (mFileType == CRU_CL1_MONTHLY)
    {
      mXDim = 720;
      mYDim = 360;
      mFileHeaderLines = 2;
      mBlockHeaderLines = 0;
    }
    //Set ArcInfo ASCII grid and CRES member variables
    else if (mFileType == GDAL)
    {
      //qDebug("ClimateFileReader::setFileType() - setting file type to GDAL") ;
      if (!mGdalDataset)
      {
        std::cout << "Error : Cant set file type before the file has been opened! " << std::endl;
      }
      mXDim = mGdalDataset->GetRasterXSize();
      mYDim = mGdalDataset->GetRasterYSize();
      mFileHeaderLines = 0;//not used
      mBlockHeaderLines = 0;//not  used
    }
    //Set Valdes member variables
    else if (mFileType == VALDES)
    {
      /* class user will need to specify rows and cols */
      //mXDim = frmCDPWizard.txtSpecifyNumCol;
      //mYDim = frmCDPWizard.txtSpecifyNumRow;
      //columnsPerRowLong = frmCDPWizard.txtSpecifyNumDataCols;
      mFileHeaderLines = 0;
      mBlockHeaderLines = 0;
    }//end of filetype handling
    return true;
  }
  catch (...)
  {
    return false;
  }
}

/** Read property of QString Filename. */
const QString  ClimateFileReader::filename()
{
  return mFileName;
}



/** Read property of bool mEndOfMatrixFlag. */

const bool ClimateFileReader::isAtMatrixEnd()
{
  return mEndOfMatrixFlag;
}

/** Read property of int mActiveBlockNo. */
const int ClimateFileReader::activeBlock()
{
  return mActiveBlockNo;
}

/** Write property of int mActiveBlockNo. */
bool ClimateFileReader::setActiveBlock( const unsigned int theBlockNo)
{
  // sanity check first!
  if (!mTextStream.is_open() || !mTextStream.good())
  {
    emit error ("Error input file is not properly open!");
    return false;
  }
  if ( theBlockNo > static_cast<unsigned int>(mBlockMarkersVector.size()))
  {
    emit error ("Attempting to read beyond blocks boundary");
    return false;
  }
  mActiveBlockNo = theBlockNo;
  mBlockStartPos = mBlockMarkersVector[theBlockNo];
  qDebug ("Moving to block %i position in file %i",theBlockNo,mBlockStartPos);
  mTextStream.seekg(mBlockStartPos,std::ios::beg);
  int myPos = mTextStream.tellg();
  if (myPos != mBlockStartPos)
  {
    emit error ("Seek to datastart failed!");
    qDebug ("Seek to datastart failed!");
    return false;
  }
  mCurrentElementNo=0;
  mCurrentColumn = 0;
  mCurrentRow=1;
  mEndOfMatrixFlag=false;
  return true;
}



const int ClimateFileReader::headerLineCount()
{
  return mFileHeaderLines;
}


const long ClimateFileReader::currentCol()
{
  return mCurrentColumn;
}



const long ClimateFileReader::currentRow()
{
  return mCurrentRow;
}



const long ClimateFileReader::currentElementNo()
{
  return mCurrentElementNo;
}


const int ClimateFileReader::blockHeaderLineCount()
{
  return mBlockHeaderLines;
}

const int ClimateFileReader::blockStartPos()
{
  return mBlockStartPos;
}




QVector <int> ClimateFileReader::getBlockMarkers(bool theForceFlag)
{
  //
  // Set up some vars
  //
  int myFileOffset=0; //store the current position in the file
  int myElementCount = mXDim * mYDim;
  //clear the vector
  mBlockMarkersVector.clear();

  //
  //see if we can retrieve the block markers from the .bmr file (if one exists)
  //

  QString myBmrFileName = mFileName;
  //replace the extension with .bmr - we assum the extension is last 3 chars
  myBmrFileName =  myBmrFileName.left(myBmrFileName.length()-3) + QString("bmr");
  if (QFile::exists(myBmrFileName) && !theForceFlag)
  {
    QFile myBmrFile( myBmrFileName );
    if ( !myBmrFile.open( QIODevice::ReadOnly ) )
    {
      emit error("Bmr file cant be opened!");
    }
    else if (theForceFlag)
    {
      emit message("Bmr file ignored - forceFlag is true ") ;
    }
    else
    {
      // now open the text stream on the filereader
      QTextStream mTextStream(&myBmrFile);
      while (!mTextStream.atEnd())
      {
        mTextStream >> myFileOffset ;
        mBlockMarkersVector.push_back(myFileOffset);
      }
      myBmrFile.close();
      return mBlockMarkersVector;
    }
  }


  //
  // Open the bmr file for writing seeing that it doesnt exists
  // or the calling fn has asked for forced parsing
  //
  QFile myBmrFile( myBmrFileName );
  QTextStream myOuputTextStream;
  if ( !myBmrFile.open( QIODevice::WriteOnly ) )
  {
    emit error("Cannot open file : " + myBmrFileName.toLocal8Bit() + " for writing") ;
    return mBlockMarkersVector;
  }
  else
  {
    //qDebug("Opened block marker file : " + myBmrFileName.toLocal8Bit() + " successfully.") ;
    // now open the text stream on the filereader
    myOuputTextStream.setDevice(&myBmrFile);
  }

  //if the datafile is a an arc/info grid file, there is only one data block
  if (mFileType==GDAL)
  {
    //no need to do anything as we'll be using gdal for data access
  }

  //
  // Start parsing the file
  //
  if (!mTextStream.is_open() || !mTextStream.good())
  {
    emit error ("File is not open, cannot generate block markers!");
    return mBlockMarkersVector;
  }
  //work out the file size

  //! @note this was done mainly for debugging...remove
  //mTextStream.seekg(0,std::ios::end);
  //int myEndPos = mTextStream.tellg();
  //qDebug("File length is %i",myEndPos);

  //make sure were at the start of the file
  mTextStream.seekg(0,std::ios::beg);
  if (!mTextStream.tellg()==0)
  {
    emit error ("Failed to rewind to start of file!");
    return mBlockMarkersVector;
  }
  //skip header lines at the top of the file
  for (int i=0; i < mFileHeaderLines; i++)
  {
    mTextStream.getline(mBuffer,mMaxLineLength-1);
  }
  double myTempVal=0.0;
  bool myFirstFlag = true;
  //loop through the rest of the file getting the start pos for each datablock
  while (!mTextStream.eof() && myFileOffset >= 0)
  {
    //skip the datablock headers
    for (int i=0; i < mBlockHeaderLines; i++)
    {
      mTextStream.getline(mBuffer,mMaxLineLength-1);
      //qDebug("Header %i : %s",i,mBuffer);
    }
    myFileOffset=mTextStream.tellg();
    if (myFileOffset == -1)
    {
      //end of file encountered
      break;
    }
    mBlockMarkersVector.push_back(myFileOffset);
    //qDebug("Found new block start at %i",myFileOffset);
    //write this marker to our bmr file
    if (myFirstFlag)
    {
      myFirstFlag=false;
      myOuputTextStream << myFileOffset ;
    }
    else
    {
      myOuputTextStream << "\n" << myFileOffset ;
    }
    //now skip the data objects for this datablock
    //qDebug ("Skipping %i elements", myElementCount);
    for (int i=0; i < myElementCount; i++)
    {
      mTextStream >> myTempVal;
      myFileOffset=mTextStream.tellg();
    }
    //read on till the end of the current line so we can be sure to be at the start of the header!
    mTextStream.getline(mBuffer,mMaxLineLength-1);
  }

  myBmrFile.close();
  //make sure were at the start of the file
  mTextStream.close();
  mTextStream.clear();


  mTextStream.open (mFileName.toLocal8Bit());
  if ( !mTextStream.is_open() || !mTextStream.good())
  {
    emit error ("Error opening " + mFileName);
    return mBlockMarkersVector;
  }
  mCurrentColumn=1;
  mCurrentRow=1;
  mCurrentElementNo=0;
  return  mBlockMarkersVector;
}

float ClimateFileReader::getElement()
{
  float myElementFloat=0;
  //see if it is ok to get another element
  if (!mEndOfMatrixFlag)
  {
    if (mFileType==GDAL)
    {
      //get the cell value for current col and row
      GDALRasterBand * myGdalBand = mGdalDataset->GetRasterBand(1);
      GDALDataType myType = myGdalBand->GetRasterDataType();
      int mySize = GDALGetDataTypeSize ( myType ) / 8;
      void *myData = CPLMalloc ( mySize );
      //-1 in row is to cater for different offset system used by non gdal readers
      CPLErr err = myGdalBand->RasterIO ( GF_Read, mCurrentColumn, mCurrentRow, 1, 1, myData, 1, 1, myType, 0, 0 );
      if (err==CE_Failure) 
      {
         qDebug("Error reading element from gdal!");
      }
      // CPLErr myResultCPLerr = myGdalBand->RasterIO(GF_Read, 0, 0, myXDimInt, myYDimInt, myGdalScanData, myXDimInt, myYDimInt, GDT_Float32, 0, 0 );
      myElementFloat = readValue ( myData, myType, 0 );
      //std::cout << "Gdal Driver retrieved : " << myElementFloat << " at " << currentColLong <<" , " << currentRowLong << " ... from... "<<  filenameString << std::endl;
      free (myData);
      mCurrentElementNo++;
    }
    else
    {
      //read a float from the file - this will advance the file pointer
      //int myPos = mTextStream.tellg();
      mTextStream >> myElementFloat;
      //qDebug ("Read element %f from position %i in file." ,myElementFloat,myPos);
      mCurrentElementNo++;
    }
    //check if we have now run to the end of the matrix
    if (mCurrentElementNo == ((mXDim*mYDim)))
    {
      mEndOfMatrixFlag=true;
    }
    else
    {
      mEndOfMatrixFlag=false;
    }
  }
  else
  {
    //you should not reach this code because you should stop any reading
    //when end of block has been detected.
    qDebug(" XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX ") ;
    qDebug(" ClimateFileReader Notice:         ") ;
    qDebug(" Error trying to get element beyond   ") ;
    qDebug(" end of block! A vector of zeros will ") ;
    qDebug(" be returned!                         ") ;
    qDebug(" XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX ") ;
    emit error ("Climate reader tried to read past end of block!");
  }

  //if we arnt at the end of the file we can update position markers...
  
  if (mFileType==GDAL)
  {
    if ((mCurrentColumn ) == mXDim-1)
    {
      mCurrentColumn=0;
      mCurrentRow++;
    }
    else
    {
      mCurrentColumn++;
    }
  }
  else
  {
    //increment the column and row counter and wrap them if needed
    if ((mCurrentColumn ) == mXDim)
    {
      mCurrentColumn=1;
      mCurrentRow++;
    }
    else
    {
      mCurrentColumn++;
    }
  }
  return myElementFloat;
}
// only aplicable to gdal!
double ClimateFileReader::readValue ( void *theData, GDALDataType theType, int theIndex )
{
  double myVal;

  switch ( theType )
  {
  case GDT_Byte:
    return (double) ((GByte *)theData)[theIndex];
    break;
  case GDT_UInt16:
    return (double) ((GUInt16 *)theData)[theIndex];
    break;
  case GDT_Int16:
    return (double) ((GInt16 *)theData)[theIndex];
    break;
  case GDT_UInt32:
    return (double) ((GUInt32 *)theData)[theIndex];
    break;
  case GDT_Int32:
    return (double) ((GInt32 *)theData)[theIndex];
    break;
  case GDT_Float32:
    return (double) ((float *)theData)[theIndex];
    break;
  case GDT_Float64:
    myVal = ((double *)theData)[theIndex];
    return (double) ((double *)theData)[theIndex];
    break;
  default:
    qWarning("Data type %d is not supported", theType);
  }
  return 0.0;
}
const int ClimateFileReader::blockCount()
{
  return mBlockMarkersVector.size();
}

void ClimateFileReader::printFirstCellInEachBlock()
{
  qDebug("Printing first cell in each block:");
  double myElementDouble=0;
  for (int i=0; i < mBlockMarkersVector.size();i++)
  {
    setActiveBlock(i);
    myElementDouble=getElement();
    qDebug ("First cell in block %i is %f \n",i,myElementDouble);
  }
}
void ClimateFileReader::printLastCellInEachBlock()
{
  qDebug("Printing first cell in each block:");
  double myElementDouble=0;
  for (int i=0; i < mBlockMarkersVector.size();i++)
  {
    setActiveBlock(i);
    do
    {
      myElementDouble=getElement();
    }
    while (!isAtMatrixEnd());
    qDebug ("Last cell in block %i is %f \n",i,myElementDouble);
  }

}
void ClimateFileReader::printBlockMarkers()
{
  qDebug("Printing block markers:");
  for (int i=0; i < mBlockMarkersVector.size(); i++ )
  {
    qDebug(QString::number(i).toLocal8Bit() + " -- " + QString::number(mBlockMarkersVector.at(i)).toLocal8Bit());
  }
}

void ClimateFileReader::printBlock(int theBlock)
{
  qDebug("Printing block:%i",theBlock);
  setActiveBlock(theBlock);
  while (!isAtMatrixEnd())
  {
    printf("%f\n",getElement());
    if (currentCol()==1)
    {
      printf("\n");
    }
  }
}
const QString  ClimateFileReader::getWorldFile()
{
  QString myHeader;
  if (mFileType==GDAL)
  {
    GDALDataset  *gdalDataset = (GDALDataset *) GDALOpen( mFileName.toLocal8Bit(), GA_ReadOnly );
    if ( gdalDataset == NULL )
    {
      return QString("");
    }
    //get the geotransform stuff from gdal
    double myTransform[6];
    if (gdalDataset->GetGeoTransform(myTransform) != CE_None)
    {
      std::cout << "Failed to get geo transform from GDAL, aborting" << std::endl;
      GDALClose(gdalDataset);
      return QString("");
    }
    else
    {
      GDALClose(gdalDataset);
    }

    myHeader += "Pixel XDim " + QString::number(myTransform[1]) + "\r\n";
    myHeader += "Rot 0 \r\n";
    myHeader += "Rot 0 \r\n";
    myHeader += "Pixel YDim " + QString::number(myTransform[5]) + "\r\n";
    myHeader += "Origin X   " + QString::number(myTransform[0]) + "\r\n";
    myHeader += "Origin Y   " + QString::number(myTransform[3]) + "\r\n";
  }
  return myHeader;
}

const QString  ClimateFileReader::getAsciiHeader()
{
  QString myHeader;
  if (mFileType==GDAL)
  {
    GDALDataset  *gdalDataset = (GDALDataset *) GDALOpen( mFileName.toLocal8Bit(), GA_ReadOnly );
    if ( gdalDataset == NULL )
    {
      return QString("");
    }

    //get dimesnions and no data value
    int myColsInt = gdalDataset->GetRasterXSize();
    int myRowsInt = gdalDataset->GetRasterYSize();
    double myNullValue=gdalDataset->GetRasterBand(1)->GetNoDataValue();
    //get the geotransform stuff from gdal
    double myTransform[6];
    if (gdalDataset->GetGeoTransform(myTransform) != CE_None)
    {
      std::cout << "Failed to get geo transform from GDAL, aborting" << std::endl;
      GDALClose(gdalDataset);
      return QString("");
    }
    else
    {
      GDALClose(gdalDataset);
    }

  

    myHeader =  "NCOLS "        + QString::number(myColsInt) + "\r\n";
    myHeader += "NROWS "        + QString::number(myRowsInt) + "\r\n";
    float myYTop = myTransform[3];
    float myXLeft = myTransform[0];
    float myCellHeight = myTransform[5];
    float myAbsCellHeight = fabs(myCellHeight);
    float myHeight = myAbsCellHeight * myRowsInt;
    float myYBottom = myYTop - myHeight;
    //qDebug("YTop: " + QString::number(myYTop).toLocal8Bit());
    //qDebug("XLeft: " + QString::number(myXLeft).toLocal8Bit());
    //qDebug("CellHeight: " + QString::number(myCellHeight).toLocal8Bit());
    //qDebug("RowCount: " + QString::number(myRowsInt).toLocal8Bit());
    //qDebug("YBottom = YTop - (fabs(CellHeight) * fabs(RowsCount))");
    //qDebug("YBottom: " + QString::number(myYBottom).toLocal8Bit());
    myHeader += "XLLCORNER "    + QString::number(myXLeft) +  "\r\n";;
    myHeader += "YLLCORNER "   + QString::number(myYBottom) +  "\r\n";
    myHeader += "CELLSIZE "     + QString::number(myTransform[1]) +  "\r\n";
    myHeader += "NODATA_VALUE " + QString::number(myNullValue) +  "\r\n";
  }
  else // non gdal
  {
        // Use the fixed matrix dimensions to create the ascii file
        // Warning: this assumes a GLOBAL dataset
        // Warning: this screws up cellsizes that are not square
        // Warning: this only works for integers at present
        myHeader = "ncols         " + QString::number (mXDim) + "\n" +
            "nrows         " + QString::number (mYDim) + "\n" +
            "xllcorner     -180\n" +
            "yllcorner     -90\n"+
            "cellsize      " + QString::number (360/static_cast<float>(mXDim)) + "\n" +
            "nodata_value  -9999.5\n";
  }
  return myHeader;
}
