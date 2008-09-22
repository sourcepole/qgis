/***************************************************************************
 *   Copyright (C) September 2008 by Tim Sutton   *
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "climatedataprocessorgui.h"
#include <cdp.h>
#include <stdio.h>
#include <stdlib.h>
//qt includes
#include <QApplication>
#include <QBitmap>
#include <QDir>
#include <QLocale>
#include <QPixmap>
#include <QImageReader>
#include <QPlastiqueStyle>
#include <QSettings>
#include <QString>
#include <QStyle>
#include <QSettings>
#include <QTextCodec>
#include <QTranslator>
#include <QImageReader>


#ifdef Q_OS_MACX
//for getting app bundle path
#include <ApplicationServices/ApplicationServices.h>
#endif
//std includes

int main(int argc, char *argv[])
{
  QApplication myApp(argc,argv);


  //NOTE: make sure these lines stay after myApp init above
  QCoreApplication::setOrganizationName("Linfiniti Consulting");
  QCoreApplication::setOrganizationDomain("linfiniti.com");
  QCoreApplication::setApplicationName("ClimateDataProcessor");

  // For non static builds on mac and win
  // we need to be sure we can find the qt image
  // plugins. In mac be sure to look in the
  // application bundle...
#ifdef Q_WS_WIN
  QCoreApplication::addLibraryPath( QApplication::applicationDirPath() 
      + QDir::separator() + "plugins" );
#endif
#ifdef Q_OS_MACX
  qDebug("Adding qt image plugins to plugin search path...");
  CFURLRef myBundleRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
  CFStringRef myMacPath = CFURLCopyFileSystemPath(myBundleRef, kCFURLPOSIXPathStyle);
  const char *mypPathPtr = CFStringGetCStringPtr(myMacPath,CFStringGetSystemEncoding());
  CFRelease(myBundleRef);
  CFRelease(myMacPath);
  QString myPath(mypPathPtr);
  // if we are not in a bundle assume that the app is built
  // as a non bundle app and that image plugins will be
  // in system Qt frameworks. If the app is a bundle
  // lets try to set the qt plugin search path...
  if (myPath.contains(".app"))
  {
    myPath += "/Contents/plugins";
    QCoreApplication::addLibraryPath( myPath );
    QCoreApplication::addLibraryPath( myPath + "/imageformats" );
    foreach (myPath, myApp.libraryPaths())
    {
      qDebug("Path:" + myPath.toLocal8Bit());
    }
    qDebug( "Added %s to plugin search path", qPrintable( myPath ) );
    QList<QByteArray> myFormats = QImageReader::supportedImageFormats();
    for ( int x = 0; x < myFormats.count(); ++x ) {
      qDebug("Format: " + myFormats[x]);
    } 
  }
#endif

  QImageReader::supportedImageFormats(); // will check for plugins
#ifdef Q_WS_WIN
  //for windows lets use plastique syle!
  QApplication::setStyle(new QPlastiqueStyle);
#endif
  //make the library search path include the application dir on windows
  //this is so the plugins can find the dlls they are linked to at run time
  QApplication::addLibraryPath(QApplication::applicationDirPath());

  //
  // Set up translations
  //
  QSettings mySettings;
  QString mySystemLocale = QLocale::languageToString(QLocale::system().language());
  QString myUserLocale = mySettings.value("locale/userLocale", "").toString();
  bool myLocaleOverrideFlag = mySettings.value("locale/overrideFlag",false).toBool();
  QString myI18nPath = Cdp::i18nPath();
  QString myLocaleString;
  QLocale myLocale;
  if (!myLocaleOverrideFlag || myUserLocale.isEmpty())
  {
    //deprecated in qt4
    //myLocale = QTextCodec::locale();
    myLocaleString = myLocale.name();
  }
  else
  {
    myLocaleString = myUserLocale;
  }
  //qDebug( "Setting translation to "
  //+ myI18nPath.toLocal8Bit()  + "/cdpDesktop_" +
  //myLocale.toLocal8Bit());

  /* Translation file for Qt.
   * The strings from the QMenuBar context section are used by Qt/Mac to shift
   * the About, Preferences and Quit items to the Mac Application menu.
   * These items must be translated identically in both qt_ and qgis_ files.
   */
  QTranslator myQtTranslator(0);
  if (myQtTranslator.load(QString("qt_") + myLocaleString, myI18nPath))
  {
    myApp.installTranslator(&myQtTranslator);
  }

  /* Translation file for cdp.
  */
  QTranslator myCdpTranslator(0);
  if (myCdpTranslator.load(QString("cdp_") + myLocaleString, myI18nPath))
  {
    myApp.installTranslator(&myCdpTranslator);
  }
  QLocale::setDefault ( myLocaleString );
  //
  // Now set up the main window and lauch the app
  //
  ClimateDataProcessorGui * mypGui = new ClimateDataProcessorGui();
  mypGui->show();
  // note if the widget does not inherit qdialog
  // (as in the case of our main window)
  // you must call app exec!
  return myApp.exec();

}

