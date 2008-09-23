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

#ifndef CLIMATEDATAPROCESSORGUI_H
#define CLIMATEDATAPROCESSORGUI_H

//I had to break my naming convention a bit here and 
//use the word gui because we already have another class
//with the same name in lib/climatedataprocessor.h
#include <ui_climatedataprocessorguibase.h>


//
// QT includes
//
#include <QDialog>

/** \ingroup gui
 * \brief A dialog to assist in creating aggregats from climate data
 * for example mean rainfall in the coolest month etc.
 * @author Tim Sutton
 * */

class ClimateDataProcessorGui : public QDialog, private Ui::ClimateDataProcessorGuiBase
{
  Q_OBJECT;

public:
  ClimateDataProcessorGui(QWidget* parent = 0, Qt::WFlags fl = 0 );
  ~ClimateDataProcessorGui();

private slots:
  /** Auto connect slot to let the user select a mean temp file */
  void on_pbnMeanTemp_clicked();
  /** Auto connect slot to let the user select a min temp file */
  void on_pbnMinTemp_clicked();
  /** Auto connect slot to let the user select a max temp file */
  void on_pbnMaxTemp_clicked();
  /** Auto connect slot to let the user select a diurnal temp file */
  void on_pbnDiurnalTemp_clicked();
  /** Auto connect slot to let the user select a mean annual 
   * precipitation file */
  void on_pbnMeanPrecipitation_clicked();
  /** Auto connect slot to let the user select a frost days file */
  void on_pbnFrostDays_clicked();
  /** Auto connect slot to let the user select a total solar radiation file */
  void on_pbnTotalSolarRad_clicked();
  /** Auto connect slot to let the user select a output path */
  void on_pbnOutputPath_clicked();
   
  /** Runs when the close button is pressed and saves current form state
  *
  */
  void accept();
  
  /** Helper method to generically get a file name.
   * @param theDefaultFile - default file used as a basis for path location
   * @param theMessage - a prompt message to use when asking for the file name
   * @return QString - containing the users selected filename or empty 
   *                  if the user cancelled.
   */
  QString getFileName(QString theDefaultFile, QString theMessage);

  /** Helper method to generically get a directory.
   * @param theDefaultFile - default file used as a basis for path location
   * @param theMessage - a prompt message to use when asking for the dir
   * @return QString - containing the users selected dir or empty 
   *                  if the user cancelled.
   */
  QString getDirName(QString theDefaultDir, QString theMessage);

  /** Runs when process button is pressed.
  * Does not close the form afterwards.
  */
  void on_pbnProcess_clicked();
  void on_cbxSelectAllVars_toggled( bool theFlag );
  void error(QString theError);
  void message(QString theMessage);
  void updateProgress (int theCurrentValue, int theMaximumValue);

  void prepareList();
  void updateList();
private: //non slots
  void readSettings();
  void writeSettings();
  QString mLastDir;
};
#endif //CLIMATEDATAPROCESSORGUI_H
