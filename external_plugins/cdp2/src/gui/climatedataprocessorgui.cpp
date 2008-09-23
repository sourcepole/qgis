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

#include "climatedataprocessorgui.h"
#include <climatedataprocessorcontroller.h>
//qt includes
#include <QDebug>
#include <QFileDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMap>
#include <QMessageBox>
#include <QSettings>
#include <QString>


ClimateDataProcessorGui::ClimateDataProcessorGui(QWidget* parent, Qt::WFlags fl)
    : QDialog(parent,fl) 
{
  //required by Qt4 to initialise the ui
  setupUi(this);
  readSettings();
  /*
  connect(mpClimateImporter, SIGNAL(error(QString)), this, SLOT(error(QString)));
  connect(mpClimateImporter, SIGNAL(message(QString)), this, SLOT(message(QString)));
  connect(mpClimateImporter, SIGNAL(updateProgress(int, int )), this, SLOT(updateProgress(int, int )));
  */
}

ClimateDataProcessorGui::~ClimateDataProcessorGui()
{}

void ClimateDataProcessorGui::accept()
{
  writeSettings();
  ClimateDataProcessorController myController;
  myController.setMeanTempFileName(leMeanTemp->text());
  myController.setMinTempFileName(leMinTemp->text());
  myController.setMaxTempFileName(leMaxTemp->text());
  myController.setDiurnalTempFileName(leDiurnalTemp->text());
  myController.setMeanPrecipFileName(leMeanPrecipitation->text());
  myController.setFrostDaysFileName(leFrostDays->text());
  myController.setTotalSolarRadFileName(leTotalSolarRadiation->text());
  myController.setFrostDaysFileName(leFrostDays->text());

  myController.setOutputPath(leOutputPath->text());

  //specify which of the available calcs we want to actually do
  if (! myController.makeAvailableCalculationsMap() )
  {
    qDebug ("Error making available calcs map");
  }
  else //available calcs map made ok
  {
    for ( int myCounter = 0; myCounter < lstVariablesToCalc->count(); myCounter++ )
    {
      QListWidgetItem * mypItem = lstVariablesToCalc->item(myCounter);
      if (mypItem->checkState()==Qt::Checked)
      {
        myController.addUserCalculation(mypItem->text());
      }
    }
    // Show a summary of the controller state (for debug purposes only)  
    qDebug(myController.description().toLocal8Bit());
    myController.run();
  }
  
  
  //done(1);
}

QString ClimateDataProcessorGui::getFileName(QString theDefaultFile, QString theMessage)
{
  QString myFileName = QFileDialog::getOpenFileName(
      this,
      theMessage , //caption
      QFileInfo(theDefaultFile).absolutePath(), //initial dir
      "Climate Climate Data (*.asc *.mea *.dat)"  //filters to select
    );
  return (myFileName);
}

QString ClimateDataProcessorGui::getDirName(QString theDefaultDir, QString theMessage)
{
  QString myFileName = QFileDialog::getExistingDirectory(
      this,
      theMessage , //caption
      theDefaultDir //initial dir
    );
  return (myFileName);
}

void ClimateDataProcessorGui::on_pbnProcess_clicked()
{
  /**
  writeSettings();
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  progressTotal->setMaximum(lstInputFiles->count());
  progressTotal->setValue(0);
  // Iterate through file list and process
  for ( int myFileInt = 0; myFileInt < lstInputFiles->count(); myFileInt++ )
  {
    QListWidgetItem *myFileItem = lstInputFiles->item( myFileInt );
    mpClimateImporter->import(myFileItem->text(), leOutputDir->text(), cboInputFormat->currentText());
    progressTask->reset();
    qDebug(myFileItem->text().toLocal8Bit()  + " import completed");
    progressTotal->setValue(myFileInt+1);
  }
  QApplication::restoreOverrideCursor();
  */
  
}

void ClimateDataProcessorGui::readSettings()
{
  /**
  QSettings mySettings;
  //leFileName->setText(mySettings.value("climateconverter/filename", "").toString());
  mLastDir=(mySettings.value("climateconverter/lastdir",".").toString());
  leOutputDir->setText(mLastDir);
  */
}

void ClimateDataProcessorGui::writeSettings()
{
  /**
  QSettings mySettings;
  //mySettings.setValue("climateconverter/filename", leFileName->text());
  mySettings.setValue("climateconverter/lastdir",mLastDir);
  */
}

void ClimateDataProcessorGui::error(QString theError)
{
  /**
  QMessageBox::warning( this,QString(tr("File Format Converter Error")),theError);
  */
}

void ClimateDataProcessorGui::message(QString theMessage)
{
  /**
  lblMessage->setText(theMessage);
  */
}

void ClimateDataProcessorGui::updateList()
{

  ClimateDataProcessorController myController;
  myController.setMeanTempFileName(leMeanTemp->text());
  myController.setMinTempFileName(leMinTemp->text());
  myController.setMaxTempFileName(leMaxTemp->text());
  myController.setDiurnalTempFileName(leDiurnalTemp->text());
  myController.setMeanPrecipFileName(leMeanPrecipitation->text());
  myController.setFrostDaysFileName(leFrostDays->text());
  myController.setTotalSolarRadFileName(leTotalSolarRadiation->text());
  myController.setFrostDaysFileName(leFrostDays->text());

  myController.setOutputPath(leOutputPath->text());

  //specif which of the available calcs we want to actually do
  if (! myController.makeAvailableCalculationsMap() )
  {
    qDebug ("Error making available calcs map");
  }
  else //available calcs map made ok
  {
    QMap<QString, bool> myMap = myController.availableCalculationsMap();
    QMapIterator<QString, bool> myMapIterator(myMap);
    lstVariablesToCalc->clear();
    while (myMapIterator.hasNext())
    {
      myMapIterator.next();
      QString myKey = myMapIterator.key();
      bool myValue = myMapIterator.value();
      QListWidgetItem * mypItem = new QListWidgetItem(myKey,lstVariablesToCalc);
      mypItem->setFlags(mypItem->flags() | Qt::ItemIsUserCheckable);
      if (myValue)
      {
        mypItem->setCheckState(Qt::Checked);
      }
      else
      {
        mypItem->setCheckState(Qt::Unchecked);
      }
    }
    // Show a summary of the controller state (for debug purposes only)  
    qDebug(myController.description().toLocal8Bit());
  }
}

void ClimateDataProcessorGui::prepareList ()
{

}
void ClimateDataProcessorGui::on_cbxSelectAllVars_toggled ( bool theFlag )
{
  for ( int myCounter = 0; myCounter < lstVariablesToCalc->count(); myCounter++ )
  {
    QListWidgetItem *  mypItem = lstVariablesToCalc->item(myCounter);
    if (theFlag)
    {
      mypItem->setCheckState(Qt::Checked);
    }
    else
    {
      mypItem->setCheckState(Qt::Unchecked);
    }
  }
}

void ClimateDataProcessorGui::updateProgress (int theCurrentValue, int theMaximumValue)
{
  /**
  progressTask->setMaximum(theMaximumValue);
  progressTask->setValue(theCurrentValue);
  QApplication::processEvents();
  */
}

void ClimateDataProcessorGui::on_pbnMeanTemp_clicked()
{
  QString myFileName = 
    getFileName(leMeanTemp->text(), tr("Choose the mean temp file"));
  if (!myFileName.isEmpty())
  {
    leMeanTemp->setText(myFileName);
  }
  updateList();
}
void ClimateDataProcessorGui::on_pbnMinTemp_clicked()
{
  QString myFileName = 
    getFileName(leMinTemp->text(), tr("Choose the min temp file"));
  if (!myFileName.isEmpty())
  {
    leMinTemp->setText(myFileName);
  }
  updateList();
}
void ClimateDataProcessorGui::on_pbnMaxTemp_clicked()
{
  QString myFileName = 
    getFileName(leMaxTemp->text(), tr("Choose the max temp file"));
  if (!myFileName.isEmpty())
  {
    leMaxTemp->setText(myFileName);
  }
  updateList();
}
void ClimateDataProcessorGui::on_pbnDiurnalTemp_clicked()
{
  QString myFileName = 
    getFileName(leDiurnalTemp->text(), tr("Choose the diurnal temp file"));
  if (!myFileName.isEmpty())
  {
    leDiurnalTemp->setText(myFileName);
  }
  updateList();

}
void ClimateDataProcessorGui::on_pbnMeanPrecipitation_clicked()
{
  QString myFileName = 
    getFileName(leMeanPrecipitation->text(), tr("Choose the mean precipitation file"));
  if (!myFileName.isEmpty())
  {
    leMeanPrecipitation->setText(myFileName);
  }
  updateList();
}
void ClimateDataProcessorGui::on_pbnFrostDays_clicked()
{
  QString myFileName = 
    getFileName(leFrostDays->text(), tr("Choose the frost days file"));
  if (!myFileName.isEmpty())
  {
    leFrostDays->setText(myFileName);
  }
  updateList();
}
void ClimateDataProcessorGui::on_pbnTotalSolarRad_clicked()
{
  QString myFileName = 
    getFileName(leTotalSolarRadiation->text(), tr("Choose the total solar radiation file"));
  if (!myFileName.isEmpty())
  {
    leTotalSolarRadiation->setText(myFileName);
  }
  updateList();
}
void ClimateDataProcessorGui::on_pbnOutputPath_clicked()
{
  QString myFileName = 
    getDirName(leOutputPath->text(), tr("Choose the output directory"));
  if (!myFileName.isEmpty())
  {
    leOutputPath->setText(myFileName);
  }
  updateList();
}
