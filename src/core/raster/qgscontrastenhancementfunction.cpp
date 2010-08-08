/* **************************************************************************
              qgscontrastenhancementfunction.cpp -  description
                       -------------------
begin                : Fri Nov 16 2007
copyright            : (C) 2007 by Peter J. Ersts
email                : ersts@amnh.org

****************************************************************************/

/* **************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscontrastenhancementfunction.h"

QgsContrastEnhancementFunction::QgsContrastEnhancementFunction( QgsContrastEnhancement::QgsRasterDataType theDataType, double theMinimumValue, double theMaximumValue )
{
  mQgsRasterDataType = theDataType;
  mMaximumValue = theMaximumValue;
  mMinimumValue = theMinimumValue;
  mMinimumMaximumRange = mMaximumValue - mMinimumValue;

  mMinimumValuePossible = QgsContrastEnhancement::minimumValuePossible( mQgsRasterDataType );
  mMaximumValuePossible = QgsContrastEnhancement::maximumValuePossible( mQgsRasterDataType );
}


int QgsContrastEnhancementFunction::enhance( double theValue )
{
  if ( mQgsRasterDataType == QgsContrastEnhancement::QGS_Byte )
  {
    return static_cast<int>( theValue );
  }
  else
  {
    return static_cast<int>(((( theValue - mMinimumValuePossible ) / ( mMaximumValuePossible - mMinimumValuePossible ) )*255.0 ) );
  }
}

bool QgsContrastEnhancementFunction::isValueInDisplayableRange( double theValue )
{
  //A default check is to see if the provided value is with the range for the data type
  if ( theValue < mMinimumValuePossible || theValue > mMaximumValuePossible )
  {
    return false;
  }

  return true;
}

void QgsContrastEnhancementFunction::setMaximumValue( double theValue )
{
  if ( mMaximumValuePossible < theValue )
  {
    mMaximumValue = mMaximumValuePossible;
  }
  else
  {
    mMaximumValue = theValue;
  }

  mMinimumMaximumRange = mMaximumValue - mMinimumValue;
}

void QgsContrastEnhancementFunction::setMinimumValue( double theValue )
{

  if ( mMinimumValuePossible > theValue )
  {
    mMinimumValue = mMinimumValuePossible;
  }
  else
  {
    mMinimumValue = theValue;
  }

  mMinimumMaximumRange = mMaximumValue - mMinimumValue;
}
