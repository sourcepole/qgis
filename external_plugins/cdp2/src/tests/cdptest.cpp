/***************************************************************************
 *   Copyright (C) 2007 by Tim Sutton   *
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
#include <QtTest/QtTest>
#include <climatedataprocessorcontroller.h>
#include <climatefilereader.h>
#include <QImage>
#include <QList>

class CdpTest: public QObject
{
  Q_OBJECT;
  private slots:
  void runTest(); 
  void initTestCase();// will be called before the first testfunction is executed.
  void cleanupTestCase();// will be called after the last testfunction was executed.
  void init(){};// will be called before each testfunction is executed.
  void cleanup(){};// will be called after every testfunction.
  private:
  //add private test vars here
  QString mMeanTempFile;
  QString mMinTempFile;
  QString mMaxTempFile;
  QString mDiurnalTempFile;
  QString mSolarRadFile;
  QString mMeanPrecipFile;
  QString mFrostDaysFile;
};

void CdpTest::initTestCase()
{
  QCoreApplication::setOrganizationName("Linfiniti Consulting");
  QCoreApplication::setOrganizationDomain("linfiniti.com");
  QCoreApplication::setApplicationName("ClimateDataProcessor");
  QString myFileName (TEST_DATA_DIR); //defined in CmakeLists.txt
  mMeanTempFile = myFileName + QDir::separator() + "meantemp01.asc";
  mMinTempFile = myFileName + QDir::separator() + "mintemp01.asc";
  mMaxTempFile = myFileName + QDir::separator() + "maxtemp01.asc";
  mDiurnalTempFile = myFileName + QDir::separator() + "diurnaltemp01.asc";
  mSolarRadFile = myFileName + QDir::separator() + "solarrad01.asc";
  mMeanPrecipFile = myFileName + QDir::separator() + "meanprecip01.asc";
  mFrostDaysFile = myFileName + QDir::separator() + "frostdays01.asc";
}

void CdpTest::cleanupTestCase()
{
}
void CdpTest::runTest()
{
  ClimateDataProcessorController myController;
  myController.setMeanTempFileName(mMeanTempFile);
  myController.setMinTempFileName(mMinTempFile);
  myController.setMaxTempFileName(mMaxTempFile);
  myController.setDiurnalTempFileName(mDiurnalTempFile);
  myController.setMeanPrecipFileName(mMeanPrecipFile);
  myController.setTotalSolarRadFileName(mSolarRadFile);
  myController.setFrostDaysFileName(mFrostDaysFile);

  myController.setOutputPath( QDir::tempPath() + QDir::separator() );

  //specify which of the available calcs we want to actually do
  if (! myController.makeAvailableCalculationsMap() )
  {
    QFAIL ("Error making available calcs map");
  }
  QMap<QString, bool> myMap = myController.availableCalculationsMap();
  QMapIterator<QString, bool> myMapIterator(myMap);
  while (myMapIterator.hasNext())
  {
    myMapIterator.next();
    QString myKey = myMapIterator.key();
    myController.addUserCalculation(myKey);
  }
  myController.setInputFileType(ClimateFileReader::GDAL);
  myController.setOutputFileType(FileWriter::ESRI_ASCII);
  // Show a summary of the controller state (for debug purposes only)  
  qDebug(myController.description().toLocal8Bit());
  myController.run();

}

QTEST_MAIN(CdpTest) 
#include "moc_cdptest.cxx"
  


