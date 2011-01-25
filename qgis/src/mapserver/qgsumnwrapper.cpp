/***************************************************************************
                              qgsumnwrapper.cpp
                              -----------------
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

extern "C"
{
#include "umn/mapserver.h"
#include "umn/maptemplate.h"
#include "umn/mapogcsld.h"
#include "umn/mapows.h"
#include "umn/cgiutil.h"
#include "umn/mapcopy.h"
#include "umn/maperror.h"
#include "umn/mapprimitive.h"
#include "umn/mapshape.h"
}

#include "qgsumnwrapper.h"
#include "qgslogger.h"
#include "qgsmapserverlogger.h"

UmnWrapper::UmnWrapper( )
{
}

UmnWrapper::~UmnWrapper()
{
}

void UmnWrapper::executeRequest( )
{
  mapservObj* mapserv = msAllocMapServObj();
  mapserv->request->ParamNames = ( char ** ) malloc( MS_MAX_CGI_PARAMS * sizeof( char* ) );
  mapserv->request->ParamValues = ( char ** ) malloc( MS_MAX_CGI_PARAMS * sizeof( char* ) );
  if ( mapserv->request->ParamNames == NULL || mapserv->request->ParamValues == NULL )
  {
    msSetError( MS_MEMERR, NULL, "mapserv()" );
    //writeError();
  }

  if ( getenv( "REQUEST_METHOD" ) == NULL )
  {
    putenv(( char* )"REQUEST_METHOD=GET" );
  }
  mapserv->request->NumParams = loadParams( mapserv->request );
  if ( mapserv->request->NumParams == -1 )
  {
    msResetErrorList();
    //CGI: msCleanup(); exit( 0 );
    return;
  }

  QgsMSDebugMsg( "msLoadMap" )
  int i;
  for ( i = 0;i < mapserv->request->NumParams;i++ ) /* find the mapfile parameter first */
    if ( strcasecmp( mapserv->request->ParamNames[i], "map" ) == 0 ) break;

  if ( i == mapserv->request->NumParams )
  {
    QgsMSDebugMsg( "Map file missing" );
    return;
  }
  mapserv->map = msLoadMap( mapserv->request->ParamValues[i], NULL );

  QgsMSDebugMsg( "msOWSDispatch" )
  int status = msOWSDispatch( mapserv->map, mapserv->request,
                              true );
  /*
  ** OWSDispatch returned either MS_SUCCESS or MS_FAILURE
  **
  ** Normally if the OWS service fails it will issue an exception,
  ** and clear the error stack but still return MS_FAILURE.  But in
  ** a few situations it can't issue the exception and will instead
  ** just an error and let us report it.
  */
  if ( status == MS_FAILURE )
  {
    errorObj *ms_error = msGetErrorObj();

    if ( ms_error->code != MS_NOERR )
      ;//writeError();
  }

  msFreeMapServObj( mapserv );
}
