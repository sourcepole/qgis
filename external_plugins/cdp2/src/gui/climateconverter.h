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

#ifndef CLIMATECONVERTER_H
#define CLIMATECONVERTER_H

#include <ui_climateconverterbase.h>
class ClimateImporter;


//
// QT includes
//
#include <QDialog>

/** \ingroup gui
 * \brief A dialog to assist in converting climate data to more
 * mainstream gdal formats.
 * @author Tim Sutton
 * */

class ClimateConverter : public QDialog, private Ui::ClimateConverterBase
{
  Q_OBJECT;

public:
  ClimateConverter(QWidget* parent = 0, Qt::WFlags fl = 0 );
  ~ClimateConverter();

private slots:
  /** Runs when the close button is pressed and saves current form state
  *
  */
  void accept();
  void on_pbnSelectFile_clicked();
  void on_pbnSelectDir_clicked();
  /** Runs when process button is pressed.
  * Does not close the form afterwards.
  */
  void on_pbnProcess_clicked();
  void error(QString theError);
  void message(QString theMessage);
  void updateProgress (int theCurrentValue, int theMaximumValue);

private:
  void readSettings();
  void writeSettings();
  QString mLastDir;
  ClimateImporter * mpClimateImporter;
};
#endif //CLIMATECONVERTER_H
