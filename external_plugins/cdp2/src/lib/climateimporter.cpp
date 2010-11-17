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

#include "climateimporter.h"
#include "climatefilereader.h"
#include "meridianswitcher.h"

//qt includes
#include <QString>
#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

ClimateImporter::ClimateImporter() : QObject()
{}

ClimateImporter::~ClimateImporter()
{}

QStringList ClimateImporter::import(QString theInputFile, QString theOutputDir, QString theFileType)
{

  // I should take a moment to explain before we start reading the data that
  // hadley cells are rectangular (3.75deg x 2.5deg). In order to make them square
  // we need to multiply the number of cells in each direction to get them to the
  // lowest common denominator. That means x cells are multiplied by 15
  // and y cells are multiplied by 10. We then end up with a block of 15x10 cells representing
  // the original rectangular cell. The hadley matrix originally was 96x73, but now
  // it will be 1440 x 730. Then lastly we discard the5 top rows and 5 bottom rows
  // to end up with a final resolution of 1440x720 (0.25 x 0.25 cells)
  float myInputCellSizeX = 0;
  int myXMultiplier = 0;
  int myYMultiplier = 0;
  int myEndRowMultiplier = 0;
  bool myDoMeridianShiftFlag = false;

  ClimateFileReader myReader;
  if (theFileType=="CRU")
  {
    myReader.initialise(theInputFile,ClimateFileReader::CRU_CL1_MONTHLY);

    myInputCellSizeX = 0.5;
    myXMultiplier = 2;
    myYMultiplier = 2;
    myEndRowMultiplier = 2;
    myDoMeridianShiftFlag = true;
  }
  else if (theFileType=="Hadley")
  {
    myReader.initialise(theInputFile,ClimateFileReader::HADLEY_SRES_MEAN);
    myInputCellSizeX = 3.75;
    myXMultiplier = 15;
    myYMultiplier = 10;
    myEndRowMultiplier = 5;
    myDoMeridianShiftFlag = true;
  }
  else // Invalid filetype
  {
    emit error("Invalid file type");
    return QStringList ();
  }
  connect(&myReader, SIGNAL(error(QString)), this, SLOT(propogateError(QString)));
  connect(&myReader, SIGNAL(message(QString)), this, SLOT(propogateMessage(QString)));

  //calculate the base name of the file without its extension of path
  QFileInfo myFileInfo(theInputFile);
  QString myBaseName = myFileInfo.baseName();

  int myXDim = myReader.xDim();
  int myYDim = myReader.yDim();
  int myBlockCount = myReader.blockCount();

  QStringList myOutputFileList;

  // shameless hardcoding taking place here....
  QString myHeader;
  myHeader += "ncols         " + QString::number(myXDim*myXMultiplier ) + "\r\n";
  myHeader += "nrows         " + QString::number(((myYDim-2)*(myYMultiplier))+(myEndRowMultiplier*2) ) + "\r\n";
  myHeader += "xllcorner     -180\r\n";
  myHeader += "yllcorner     -90\r\n";
  myHeader += "cellsize       " + QString::number(1.0/(static_cast<float>(myXDim*myXMultiplier)/360) ) + "\r\n";
  //note hdaley seen to use +9999 for no data....
  myHeader += "NODATA_value  -9999\r\n";

  //make a meridian shift object
  MeridianSwitcher mySwitcher;
  connect(&mySwitcher, SIGNAL(error(QString)), this, SLOT(propogateError(QString)));
  connect(&mySwitcher, SIGNAL(message(QString)), this, SLOT(propogateMessage(QString)));

  //for debugging....to be removed...
  //myReader.printBlockMarkers();
  //myReader.printFirstCellInEachBlock();
  //myReader.printLastCellInEachBlock();

  for (int i=0; i<myBlockCount;i++)
  {
    if (!myReader.setActiveBlock(i))
    {
      break;
    }
    //note filereader use base 1 not base 0!
    emit message("Processing block " + QString::number(i+1));
    QString myOutputFileName;
    //months are base1 !
    int myMonthNo = i+1;
    if (myMonthNo<10)
    {
      myOutputFileName = theOutputDir + "/" + myBaseName + "_0" + QString::number(myMonthNo).toLocal8Bit() + ".asc";
    }
    else
    {
      myOutputFileName = theOutputDir + "/" + myBaseName + "_" + QString::number(myMonthNo).toLocal8Bit() + ".asc";
    }
    QFile myOutputFile (myOutputFileName);
    //note file is not appended to but overwritten!
    if ( myOutputFile.open( QIODevice::WriteOnly ) )
    {
      myOutputFileList.append(myOutputFileName);
      QTextStream myOutputTextStream( &myOutputFile );
      myOutputTextStream << myHeader.toLocal8Bit();
      int myCurrentCount=0;
      QString myCurrentLine;
      while (!myReader.isAtMatrixEnd())
      {
        float myFloat = myReader.getElement();
        //repeat in the x direction the prescribed amount of times
        for (int x=1 ; x <= myXMultiplier; x++)
        {
          myCurrentLine +=  QString::number(myFloat);
          if (x < myXMultiplier)
          {
            myCurrentLine += " ";
          }
        }
        //if we are at rows' end we must repeat in the y direction too....
        //(simply by writing the same line several times)
        if (myReader.currentCol()==myXDim)
        {
          myCurrentLine += "\r\n";
          int myCurrentRowNo = myReader.currentRow();
          if (myCurrentRowNo==1 || myCurrentRowNo == myYDim)
          {
            //qDebug(myCurrentLine.toLocal8Bit());
            for (int y=0; y < myEndRowMultiplier; y++)
            {
              myOutputTextStream << myCurrentLine.toLocal8Bit();
            }
          }
          else
          {
            for (int y=0; y < myYMultiplier; y++)
            {
              myOutputTextStream << myCurrentLine.toLocal8Bit();
            }
          }
          myCurrentLine="";
        }
        else
        {
          myCurrentLine += " ";
        }
        myCurrentCount++;
      }
      myOutputFile.close();
    }
    else
    {
      emit error ("Could not open output file " + myOutputFileName.toLocal8Bit() + " for writing.");
      break;
    }

    if (myDoMeridianShiftFlag)
    {
      QString myMeridianShiftOutputFileName;
      if (myMonthNo<10)
      {
        myMeridianShiftOutputFileName = theOutputDir + "/" + myBaseName + "_shift_0" + QString::number(myMonthNo).toLocal8Bit() + ".asc";
      }
      else
      {
        myMeridianShiftOutputFileName = theOutputDir + "/" + myBaseName + "_shift_" + QString::number(myMonthNo).toLocal8Bit() + ".asc";
      }
      emit message("Shifting meridian");
      mySwitcher.doSwitch(myOutputFileName, myMeridianShiftOutputFileName);
    }
    emit updateProgress (i,myBlockCount);


  }
  emit message("Conversion Complete, " + QString::number(myBlockCount) + " blocks processed succssfully.");
  return myOutputFileList;
}


void ClimateImporter::propogateError (QString theError)
{
  //just pass it on up the food chain
  emit error(theError);
}

void ClimateImporter::propogateMessage (QString theMessage)
{
  //just pass it on up the food chain
  emit message(theMessage);
}
