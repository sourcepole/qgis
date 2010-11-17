#include "meridianswitcher.h"

#include "gdal_priv.h"
#include <QTextStream>
#include <QFile>
#include <iostream>
#include <QString>
#include <QVector>
#include <QMap>


MeridianSwitcher::MeridianSwitcher() : QObject()
{

}


MeridianSwitcher::~MeridianSwitcher() 
{
}

bool MeridianSwitcher::doSwitch(QString theInputFileString, QString theOutputFileString)
{
  QFile * myFilePointer=new QFile(theOutputFileString);
  QTextStream * myTextStream;
  QString mySeperatorString=QString(" ");

  if (!myFilePointer->open(QIODevice::WriteOnly))
  {
    emit error ("MeridianSwitcher::Cannot open file : "+ theOutputFileString);
    return false;
  }
  else
  {
    myTextStream = new QTextStream(myFilePointer);
    emit message("MeridianSwitcher::Opened file ... "+ theOutputFileString+" successfully.");
  }

  //printf("Started with input :  %s output: %s \n",theInputFileString,theOutputFileString);
  GDALAllRegister();
  GDALDataset  *gdalDataset = (GDALDataset *) GDALOpen( theInputFileString.toLocal8Bit(), GA_ReadOnly );
  if ( gdalDataset == NULL )
  {
    //valid = FALSE;
    emit error ("MeridianSwitcher::Cannot create gdal dataset: "+ theOutputFileString);
    return false;
  }
  int myXDimInt = gdalDataset->GetRasterXSize();
  int myYDimInt = gdalDataset->GetRasterYSize();
  printf("Raster is %i x %i cells\n", myXDimInt, myYDimInt);

  //note the ll corner needs softcoding!
  QString myHeaderString=
      QString ("ncols         ") +
      QString::number (myXDimInt) + 
      QString ("\n")+
      QString ("nrows         ") + 
      QString::number (myYDimInt) + 
      QString ("\n")+
      QString ("xllcorner     -180\n")+
      QString ("yllcorner     -90\n")+
      QString ("cellsize      ") + 
      QString::number (360/static_cast<float>(myXDimInt)) +
      QString ("\n")+
      QString ("nodata_value  -9999.0\n");                    
      *myTextStream << myHeaderString;

  GDALRasterBand  *myGdalBand = gdalDataset->GetRasterBand( 1 );
  float *myGdalScanData = (float*) CPLMalloc(sizeof(uint)*myXDimInt * sizeof(uint)*myYDimInt);
  CPLErr myResultCPLerr = myGdalBand->RasterIO(GF_Read, 0, 0, myXDimInt, myYDimInt, myGdalScanData, myXDimInt, myYDimInt, GDT_Float32, 0, 0 );
  if(myResultCPLerr != CE_None)
  {
    emit error ("MeridianSwitcher::Error reading from gdal dataset: "+ theOutputFileString);
    return false;
  }
  for (int myColumnInt = 0; myColumnInt < myYDimInt; myColumnInt++)
  {
    //grab the second half of this row and write to disk
    for (int myRowInt =myXDimInt/2; myRowInt < myXDimInt; myRowInt++)
    {
      float myFloat=myGdalScanData[myColumnInt*myXDimInt + myRowInt];
      //write the number to the file
      *myTextStream << myFloat << mySeperatorString;
    }
    //grab the first half of this row and write to disk
    for (int myRowInt =0; myRowInt < (myXDimInt/2); myRowInt++)
    {
      float myFloat=myGdalScanData[myColumnInt*myXDimInt + myRowInt];
      //write the number to the file
      *myTextStream << myFloat << mySeperatorString;
    }
    *myTextStream << "\n";
  }
  //draw with the experimental transaparency support
  CPLFree(myGdalScanData);
  GDALClose(gdalDataset);
  myFilePointer->close();
  delete myTextStream;
  delete myFilePointer;

  return true ;
}
