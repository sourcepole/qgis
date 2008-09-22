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
//qt includes
#include <QSettings>
#include <QFileDialog>
#include <QString>
#include <QMessageBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>

ClimateDataProcessorGui::ClimateDataProcessorGui(QWidget* parent, Qt::WFlags fl)
    : QDialog(parent,fl) 
{
  /**
  //required by Qt4 to initialise the ui
  setupUi(this);
  readSettings();
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
  done(1);
}

void ClimateDataProcessorGui::on_pbnSelectFile_clicked()
{
  /**
  QSettings mySettings;
  QString myBaseDir = mySettings.value("climateconverter/inputLayersDirectory","/tmp").toString(); //initial dir
  QStringList myFileNameQStringList = QFileDialog::getOpenFileNames(
                                        this,
                                        "Select one or more files",
                                        myBaseDir, //initial dir
                                        "Climate Climate Data (*.asc *.mea *.dat)"  //filters to select
                                      );
  lstInputFiles->addItems(myFileNameQStringList);
  QFileInfo myFileInfo(myFileNameQStringList.at(0));
  QString myBaseString = myFileInfo.dir().path()+QString("/");
  mySettings.setValue("climateconverter/inputLayersDirectory",myBaseString);
  */
}

void ClimateDataProcessorGui::on_pbnSelectDir_clicked()
{
  /**
  QString myDir = QFileDialog::getExistingDirectory(
                    this,
                    "Choose a directory",
                    mLastDir);

  mLastDir = myDir;
  leOutputDir->setText(myDir);
  */
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

void ClimateDataProcessorGui::updateProgress (int theCurrentValue, int theMaximumValue)
{
  /**
  progressTask->setMaximum(theMaximumValue);
  progressTask->setValue(theCurrentValue);
  QApplication::processEvents();
  */
}
