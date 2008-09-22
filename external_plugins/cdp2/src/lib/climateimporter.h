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
#ifndef CLIMATEIMPORTER_H
#define CLIMATEIMPORTER_H

#include <QObject>

class QString;
class QStringList;

/** \ingroup library
 * \brief This is a utilitiy for importing climate data such as Hadley, CRU etc.
 */
class CDP_LIB_EXPORT ClimateImporter : public QObject
{
Q_OBJECT;

public:

 ClimateImporter();
 ~ClimateImporter();
 /** Import a Climate file 
 * Output files will have the same name suffix as input file, with a numerical
 * suffix for each monthly block.
 * @note if an errror occurs, an empty QStringList or partially filled QStringList 
 * will be returned (and an error will be emitted)
 * @param theInputFile A valid hadley climate data file
 * @param theOutputDir The directory where the output files will be generated.
 * @param theFileType The type of climate file to process
 * @return a QStringList containing the list of output file names.
 */
 QStringList import(QString theInputFile, QString theOutputDir, QString theFileType);

signals:
  void error (QString theError);
  void message (QString theMessage);
  /** Emit a progress signal.
  * @NOTE should only ever be called using the showProgress method so we can properly 
  * support gdal style callbacks
  * @see showProgress
  */
  void updateProgress (int theProgress,int theMaximum);

public slots:
  /** Pass an error received on up to any listening qobjects */
  void propogateError (QString theError);
  /** Pass an error received on up to any listening qobjects */
  void propogateMessage (QString theMessage);

};

#endif //CLIMATEIMPORTER_H
