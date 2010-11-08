/***************************************************************************
  filegroup.cpp  -  description
  -------------------
begin                : Sat May 10 2003
copyright            : (C) 2003 by Tim Sutton
email                : t.sutton@reading.ac.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "climatefilegroup.h"

ClimateFileGroup::ClimateFileGroup() : QObject()
{
  mElementCount=0;
  mXDim=0;
  mYDim=0;
  mEndOfMatrixFlag=false;
}

ClimateFileGroup::~ClimateFileGroup()
{
}
void ClimateFileGroup::close()
{
  QVectorIterator<ClimateFileReader *> myIterater(mReaderVector);
  while (myIterater.hasNext()) 
  {
    ClimateFileReader * myReader = myIterater.next() ;
    delete myReader;
  }
  mReaderVector.clear();
}
bool ClimateFileGroup::add(ClimateFileReader* theReader)
{
  if (!theReader)
  {
      emit error (QString("ClimateFileGroup::addClimateFileReader() error - theReader is uninitialised!"));
      return false;
  }

  //expand the filereader vector by one and insert the new filereader
  //onto the end of the list
  mReaderVector.push_back(theReader);
  int mySizeInt = mReaderVector.count();
  //see if this was the first filereader being added and if so set the
  //mElementCount property.
  if (mySizeInt==1)
  {
    mElementCount=theReader->xDim() * theReader->yDim();
    mXDim=theReader->xDim();
    mYDim=theReader->yDim();
    qDebug("First reader in group has properties: x %ld , y %ld",theReader->xDim(),theReader->yDim());
  }
  return true;
}

int ClimateFileGroup::elementCount()
{
  return mElementCount;
}

int ClimateFileGroup::fileCount()
{
  return mReaderVector.count();
}
 

QVector<float> ClimateFileGroup::getElementVector()
{
  QVector<float> myFloatVector;
  if (mReaderVector.isEmpty())
  {
    emit (QString("Error, the file group is empty, how can I get an element when there are no files?"));
    return myFloatVector;
  }
  
  //test that there are some files in our filereader group
  if (0==mReaderVector.count())
  {
    emit (QString("Error, the file group is empty, how can I get an element when there are no files?"));
    return myFloatVector;
  }
  //test we are not at the end of the matrix
  if (true==mEndOfMatrixFlag)
  {
    emit (QString("Error, the file group is at the end of the matrix. Use rewind before trying to read from it again!"));
    return myFloatVector;
  }
  //retrieve the each ClimateFileReader from the colelction and get its current element
  bool myFirstFlag=true;
  QVectorIterator<ClimateFileReader *> myIterater(mReaderVector);
  while (myIterater.hasNext()) 
  {
    ClimateFileReader * myReader = myIterater.next() ;
    float myFloat = myReader->getElement();
    //test if we are at the end of the matrix
    if ( myFirstFlag==true)
    {
      mEndOfMatrixFlag=myReader->isAtMatrixEnd();
      myFirstFlag=false;
    }
    myFloatVector.push_back(myFloat);
  }

  return myFloatVector;
}

const bool ClimateFileGroup::isAtMatrixEnd()
{
  return mEndOfMatrixFlag;
}

bool ClimateFileGroup::rewind()
{
  if (0==mReaderVector.count())
  {
    return false;
  }
  QVectorIterator<ClimateFileReader *> myIterater(mReaderVector);
  while (myIterater.hasNext()) 
  {
    ClimateFileReader * myReader = myIterater.next() ;
    myReader->setActiveBlock(0);
  }
  mEndOfMatrixFlag=false;
  //add better error checking
  return true;
}

