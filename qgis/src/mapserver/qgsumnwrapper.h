/***************************************************************************
                              qgsumnwrapper.h
                              ---------------
  begin                : Jan 25, 2011
  copyright            : (C) 2011 by Pirmin Kalberer
  email                : pka at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef UMNWRAPPER_H
#define UMNWRAPPER_H


/**UMN Mapserver wrapper */
class UmnWrapper
{
  public:
    UmnWrapper( );
    virtual ~UmnWrapper();

    /**Execute WMS request*/
    void executeRequest( );
};

#endif // UMNWRAPPER_H
