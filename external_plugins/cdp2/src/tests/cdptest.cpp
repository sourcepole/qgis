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
#include <QImage>
#include <QList>

class CdpTest: public QObject
{
  Q_OBJECT;
  private slots:
  /** Regression test for making sure 
      available calcs always computes correctly grid layers */
  void availableCalcsTest(); 
  void initTestCase();// will be called before the first testfunction is executed.
  void cleanupTestCase();// will be called after the last testfunction was executed.
  void init(){};// will be called before each testfunction is executed.
  void cleanup(){};// will be called after every testfunction.
  private:
  //add private test vars here
};

void CdpTest::initTestCase()
{
  QCoreApplication::setOrganizationName("Linfiniti Consulting");
  QCoreApplication::setOrganizationDomain("linfiniti.com");
  QCoreApplication::setApplicationName("ClimateDataProcessor");
}
void CdpTest::cleanupTestCase()
{
}
void CdpTest::availableCalcsTest()
{
}

QTEST_MAIN(CdpTest) 
#include "moc_cdptest.cxx"
  


