/***************************************************************************
  QgsAttributeTableIdColumnPair.h - Helper class for attribute tables
  -------------------
         date                 : Feb 2009
         copyright            : Vita Cizek
         email                : weetya (at) gmail.com

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSATTRIBUTETABKEIDCOLUMNPAIR_H
#define QGSATTRIBUTETABKEIDCOLUMNPAIR_H

#include <QVariant>

class QgsAttributeTableIdColumnPair
{
    int mId;
    QVariant mItem;

  public:
    QgsAttributeTableIdColumnPair( int anId, QVariant anItem ) : mId( anId ), mItem( anItem ) {}
    /**
     * Returns true if this id-column pair is less the the tested one
     * @param b the tested id-column pair
     */
    bool operator<( const QgsAttributeTableIdColumnPair &b ) const;
    int id() { return mId; }
};

#endif
