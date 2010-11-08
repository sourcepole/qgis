/***************************************************************************
  climatedataprocessor.cpp  -  description
  -------------------
begin                : Thu May 15 2003
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

#include "climatedataprocessorcontroller.h"
#include "climatedataprocessor.h"
#include "filewriter.h"
#include "climatefilereader.h"
#include "climatefilegroup.h"
#include <iostream>
//QT Includes
#include <QTextStream>
#include <QMessageBox> //this must go!
#include <QDir>
#include <QRegExp>
#include <QVector>

const float NO_DATA=-9999.0;

ClimateDataProcessorController::ClimateDataProcessorController() : QObject()
{

    // initialise the fileGroup file names  - not strictly needed but neater
    QString myString=QString("");
    mMeanTempFileName=myString;
    mMinTempFileName=myString;
    mMaxTempFileName=myString;
    mDiurnalTempFileName=myString;
    mMeanPrecipFileName=myString;
    mFrostDaysFileName=myString;
    mTotalSolarRadFileName=myString;
    mWindSpeedFileName=myString;

    mOutputPath = myString;
    mInputFileType=ClimateFileReader::GDAL;


}

/** Destructor */
ClimateDataProcessorController::~ClimateDataProcessorController()
{}


const QString ClimateDataProcessorController::outputHeader()
{
    return mOutputHeader;
}

void ClimateDataProcessorController::setOutputHeader( const QString& theOutputHeader)
{
    mOutputHeader = theOutputHeader;
}

bool ClimateDataProcessorController::makeInputFileTypeMap()
{
    QString myString;
    ClimateFileReader::FileType myFileType;
    mInputFileTypeMap.clear();
    // Copied from ClimateFileReader header file:
    //enum FileType { GDAL, HADLEY_SRES , HADLEY_IS92 ,  IPCC_OBSERVED ,
    //                                    VALDES ,  ECHAM4 ,  CSIRO_MK2 ,  NCAR_CSM_PCM , GFDL_R30 , CGCM2 ,
    //                                    CCSR_AGCM_OGCM };


    //declare the key value
    myString=QString("GDAL Supported Raster");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::GDAL;
    //add it to the associative array
    mInputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("Hadley Centre HadCM3 SRES Scenario");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::HADLEY_SRES;
    //add it to the associative array
    mInputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("Hadley Centre HadCM3 IS92a Scenario");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::HADLEY_IS92;
    //add it to the associative array
    mInputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("IPCC Observed Climatology");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::IPCC_OBSERVED;
    //add it to the associative array
    mInputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("University of Reading Palaeoclimate data");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::VALDES;
    //add it to the associative array
    mInputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("Max Planck Institute fur Meteorologie (MPIfM) ECHAM4 data");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::ECHAM4;
    //add it to the associative array
    mInputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("CSIRO-Mk2 Model data");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::CSIRO_MK2;
    //add it to the associative array
    mInputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("National Center for Atmospheric Research (NCAR) NCAR-CSM and NCAR-PCM data");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::NCAR_CSM_PCM;
    //add it to the associative array
    mInputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("Geophysical Fluid Dynamics Laboratory (GFDL) R30 Model data");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::GFDL_R30;
    //add it to the associative array
    mInputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("Canadian Center for Climate Modelling and Analysis (CCCma) CGCM2 Model data");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::CGCM2;
    //add it to the associative array
    mInputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("CCSR/NIES AGCM model data and CCSR OGCM model data");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::CCSR_AGCM_OGCM;
    //add it to the associative array
    mInputFileTypeMap[myString]=myFileType;

    return true;
}

bool ClimateDataProcessorController::makeOutputFileTypeMap()
{
    QString myString;
    FileWriter::FileType myFileType;
    mOutputFileTypeMap.clear();
    // Copied from FileWriter header file:
    //enum FileFormatEnum { GDAL_TIFF, CSM_MATLAB , CSM_OCTAVE ,  GARP ,  ESRI_ASCII ,  PLAIN };

    //declare the key value
    myString=QString("GDAL Tiff Image");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=FileWriter::ESRI_ASCII;
    //add it to the associative array
    mOutputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("Matlab");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=FileWriter::MATLAB;
    //add it to the associative array
    mOutputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("ESRI ASCII Grid");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=FileWriter::ESRI_ASCII;
    //add it to the associative array
    mOutputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("Plain matrix with no header");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=FileWriter::PLAIN;
    //add it to the associative array
    mOutputFileTypeMap[myString]=myFileType;
    return true;
}

const QString  ClimateDataProcessorController::meanTempFileName()
{
    return mMeanTempFileName;
}

void ClimateDataProcessorController::setMeanTempFileName( QString theFileName)
{
    mMeanTempFileName = theFileName;
    qDebug( "meanTempFileName set to : " + mMeanTempFileName.toLocal8Bit() );
}

const QString  ClimateDataProcessorController::minTempFileName()
{
    return mMinTempFileName;
}

void ClimateDataProcessorController::setMinTempFileName( QString theFileName)
{
    mMinTempFileName = theFileName;
    qDebug( "minTempFileName set to : " + mMinTempFileName.toLocal8Bit() );
}

const QString  ClimateDataProcessorController::maxTempFileName()
{
    return mMaxTempFileName;
}

void ClimateDataProcessorController::setMaxTempFileName( QString theFileName)
{
    mMaxTempFileName = theFileName;
    qDebug( "maxTempFileName set to : " + mMaxTempFileName.toLocal8Bit() );
}

const QString  ClimateDataProcessorController::diurnalTempFileName()
{
    return mDiurnalTempFileName;
}

void ClimateDataProcessorController::setDiurnalTempFileName( QString theFileName)
{
    mDiurnalTempFileName = theFileName;
    qDebug( "diurnalTempFileName set to : " + mDiurnalTempFileName.toLocal8Bit() );
}

const QString  ClimateDataProcessorController::meanPrecipFileName()
{
    return mMeanPrecipFileName;
}

void ClimateDataProcessorController::setMeanPrecipFileName( QString theFileName)
{
    mMeanPrecipFileName = theFileName;
    qDebug( "meanPrecipFileName set to : " + mMeanPrecipFileName.toLocal8Bit() );
}

const QString  ClimateDataProcessorController::frostDaysFileName()
{

    qDebug( "frostDaysFileName set to : " + mFrostDaysFileName.toLocal8Bit() );
    return mFrostDaysFileName;
}

void ClimateDataProcessorController::setFrostDaysFileName( QString theFileName)
{
    mFrostDaysFileName = theFileName;
    qDebug( "frostDaysFileName set to : " + mFrostDaysFileName.toLocal8Bit() );
}

const QString  ClimateDataProcessorController::totalSolarRadFileName()
{
    return mTotalSolarRadFileName;
}

void ClimateDataProcessorController::setTotalSolarRadFileName( QString theFileName)
{
    mTotalSolarRadFileName = theFileName;

    qDebug( "totalSolarRadFileName set to : " + mTotalSolarRadFileName.toLocal8Bit() );
}

const QString  ClimateDataProcessorController::windSpeedFileName()
{
    return mWindSpeedFileName;
}

void ClimateDataProcessorController::setWindSpeedFileName( QString theFileName)
{
    mWindSpeedFileName = theFileName;
    qDebug( "windSpeedFileName set to : " + mWindSpeedFileName.toLocal8Bit() );
}

const QString  ClimateDataProcessorController::outputFilePath()
{
    return mOutputPath;
}

void ClimateDataProcessorController::setOutputPath( QString theFilePath )
{
    mOutputPath = theFilePath;
    qDebug( "outputFilePath set to : " + mOutputPath.toLocal8Bit() );
}

bool ClimateDataProcessorController::makeFileGroups()
{

    /*   These are the possible filegroups available:
         -----------------------------------------------------------------
         meanTempFileGroup;
         mpMinTempFileGroup;
         mpMaxTempFileGroup;
         mpDiurnalTempFileGroup;
         mpMeanPrecipFileGroup;
         mpFrostDaysFileGroup;
         mpTotalSolarRadFileGroup;
         mpWindSpeedFileGroup;
         -----------------------------------------------------------------
         */
    const int START_YEAR=1; //this will be deprecated soon!
    if (mMeanTempFileName==QString(""))
    {
        qDebug(     "makeFileGroups - meanTempFileName is NOT initialised! *****************************" );
    }
    else
    {
        meanTempFileGroup = initialiseFileGroup(mMeanTempFileName,START_YEAR);
    }

    if (mMinTempFileName==QString(""))
    {
        qDebug(     "makeFileGroups - minTempFileName is NOT initialised! *****************************" );
    }
    else
    {
        mpMinTempFileGroup = initialiseFileGroup(mMinTempFileName,START_YEAR);
    }

    if (mMaxTempFileName==QString(""))
    {
        qDebug(     "makeFileGroups - maxTempFileName is NOT initialised! *****************************" );
    }
    else
    {
        mpMaxTempFileGroup = initialiseFileGroup(mMaxTempFileName,START_YEAR);
    }

    if (mDiurnalTempFileName==QString(""))
    {
        qDebug(     "makeFileGroups - diurnalTempFileName is NOT initialised! *****************************" );
    }
    else
    {
        mpDiurnalTempFileGroup = initialiseFileGroup(mDiurnalTempFileName,START_YEAR);
    }

    if (mMeanPrecipFileName==QString(""))
    {
        qDebug(     "makeFileGroups - meanPrecipFileName is NOT initialised! *****************************" );
    }
    else
    {
        mpMeanPrecipFileGroup = initialiseFileGroup(mMeanPrecipFileName,START_YEAR);
    }

    if (mMeanPrecipFileName==QString(""))
    {
        qDebug(     "makeFileGroups - meanPrecipFileName is NOT initialised! *****************************" );
    }
    else
    {
        mpMeanPrecipFileGroup = initialiseFileGroup(mMeanPrecipFileName,START_YEAR);
    }


    if (mFrostDaysFileName==QString(""))
    {
        qDebug(     "makeFileGroups - frostDaysFileName is NOT initialised! *****************************" );
    }
    else
    {
        mpFrostDaysFileGroup = initialiseFileGroup(mFrostDaysFileName,START_YEAR);
    }


    if (mTotalSolarRadFileName==QString(""))
    {
        qDebug( "makeFileGroups - totalSolarRadFileName is NOT initialised! *****************************" );
    }
    else
    {
        mpTotalSolarRadFileGroup = initialiseFileGroup(mTotalSolarRadFileName,START_YEAR);
    }


    if (mWindSpeedFileName==QString(""))
    {
        qDebug( "makeFileGroups - totalSolarRadFileName is NOT initialised! *****************************" );
    }
    else
    {
        mpWindSpeedFileGroup = initialiseFileGroup(mWindSpeedFileName,START_YEAR);
    }
    return true;
}
/*   Construct a filegroup given a base file name.
 *   @param theStartYear Used when processing data NOT in series to shift the
 *           internal file pointer to the correct year. Note this is a base 1 number.
 *           Default is 1.
 */
ClimateFileGroup *ClimateDataProcessorController::initialiseFileGroup(QString theFileName,int theStartYear=1)
{
    ClimateFileGroup * myFileGroup = new ClimateFileGroup();
    qDebug( "initialiseFileGroup - theFileName is initialised to : " + theFileName.toLocal8Bit() );
    if (mFilesInSeriesFlag)
    {
        //we have a separate file for each months data - the
        //assumption we make then is that the user selected the first file
        //e.g. file_00.asc and that the remaining files are numbered sequentially
        qDebug( "initialiseFileGroup - files in series!" );
        //create a file group from this file
        qDebug( "initialiseFileGroup - theFileName = " + theFileName.toLocal8Bit() );
        //need to add some error handling here...

       QFileInfo myFileInfo(theFileName);
       QString myPath = myFileInfo.dir().path();
       QString myExtension = myFileInfo.completeSuffix(); //include all dotted extensions e.g. tar.gz
       QString myFileName = myFileInfo.baseName();//exclude all dotted extentsions e.g. will return 'file' from 'file.tar.gz'
       //! @note the assumption is implicit here that the first file in a file series ends in 00 !!!! 
       QString myFileNameBase = myFileName.left(myFileName.length()-2);//e.g. 'somefile00' becomes 'somefile'

        for (int myInt=1; myInt < 13; myInt++)
        {
            QString myCurrentFileName = myPath+QDir::separator()+myFileNameBase;

            if (myInt < 10)
            {
                myCurrentFileName+=QString("0")+QString::number(myInt);
            }
            else
            {
                myCurrentFileName+=QString::number(myInt);
            }
            myCurrentFileName+="."+myExtension;
            qDebug( "initialiseFileGroup - opening file : " + myCurrentFileName.toLocal8Bit() );
            ClimateFileReader *mypClimateFileReader = new ClimateFileReader();
            mypClimateFileReader->initialise(myCurrentFileName,mInputFileType);
            mOutputHeader=mypClimateFileReader->getAsciiHeader();            
            qDebug( "initialiseFileGroup - *** Adding " + myCurrentFileName.toLocal8Bit()
            + " to file group *********************" );
            myFileGroup->add(mypClimateFileReader);

        }

        return myFileGroup;

    }//end of first part of file in series flag test
    else
    {
        //the file is not in series this consists of a single file with
        //multiple datablocks - one for each period (day / month / year etc)
        qDebug( "initialiseFileGroup - files NOT in series!" );
        qDebug( "initialiseFileGroup - theFileName = " + theFileName.toLocal8Bit() );
        //need to add some error handling here...
        //calculate the actual blocks numbers represented by the start & end years
        unsigned int myCurrentBlockInt = ((theStartYear-1)*12)+1;
        unsigned int myEndBlockInt = (12*theStartYear)+1;

        printf ("ClimatteDataProcessor::initialiseFileGroup Calculated start block : %i and end block is %i\n",
                myCurrentBlockInt,
                myEndBlockInt
               );

        //now we loop through 12 blocks to create our filegroup
        //this is the loop that sets up file readers - one per data
        //block for the required year, for the current month.
        for (unsigned int myInt=myCurrentBlockInt; myInt < myEndBlockInt; ++myInt)
        {
            ClimateFileReader *myClimateFileReader = new ClimateFileReader();
            myClimateFileReader->initialise(theFileName,mInputFileType);            
            myClimateFileReader->setActiveBlock(myInt);
            myFileGroup->add(myClimateFileReader);
            qDebug( "Adding block " + QString::number(myInt).toLocal8Bit() + " to the block list to be processed" );
        }

        //ownership of the filereader pointer is transferred to the file group

        return myFileGroup;
    } //end of second part of files in series flag test

}


/** Read property of ClimateFileReader::FileType FileType. */
const ClimateFileReader::FileType ClimateDataProcessorController::inputFileType()
{
    return mInputFileType;
}
/** Write property of ClimateFileReader::FileType FileType. */
void ClimateDataProcessorController::setInputFileType( const ClimateFileReader::FileType theInputFileType)
{
    mInputFileType = theInputFileType;
}

/** Overloaded version of above that taks a string and looks up the enum */
void ClimateDataProcessorController::setInputFileType( const QString theInputFileTypeString)
{


    QString myString = theInputFileTypeString;
    ClimateFileReader::FileType myInputFileType;
    //make sure the fileTypeMap exists
    makeInputFileTypeMap();
    //convert the input string to ucase
    myString=myString.toUpper();
    //look up the filetype enum given the string
    myInputFileType = mInputFileTypeMap[myString];
    //set the filetype given the enum
    setInputFileType (myInputFileType);

}


/** Read property of FileWriter::FileType outputFileType. */
const FileWriter::FileType ClimateDataProcessorController::outputFileType()
{
    return mOutputFileType;
}
/** Write property of FileWriter::FileType FileType. */
void ClimateDataProcessorController::setOutputFileType( const FileWriter::FileType theOutputFileType)
{
    mOutputFileType = theOutputFileType;
}

/** Overloaded version of above that taks a string and looks up the enum */
void ClimateDataProcessorController::setOutputFileType( const QString theOutputFileTypeString)
{


    QString myString = theOutputFileTypeString;
    FileWriter::FileType myOutputFileType;
    //make sure the fileTypeMap exists
    makeOutputFileTypeMap();
    //convert the Output string to ucase
    myString=myString.toUpper();
    //look up the filetype enum given the string
    myOutputFileType = mOutputFileTypeMap[myString];
    //set the filetype given the enum
    setOutputFileType (myOutputFileType);

}

/**  Build a list of which calculations can be performed given the input files
 *    that have been registered.
 *   The boolean flag will be used to indicate whether the user actually wants to
 *   perform the calculation on the input dataset(s) defaults to false.
 */
bool  ClimateDataProcessorController::makeAvailableCalculationsMap()
{
#ifdef QGISDEBUG
    qDebug( "ClimateDataProcessorController::makeAvailableCalculationsMap() called!" );
#endif

    QString myString;
    bool myBool=false;  //default is not to perform any eligible  calculation
    mAvailableCalculationsMap.clear();
    /*   These are the possible filegroups available:
         -----------------------------------------------------------------
         mMeanTempFileName;
         mMinTempFileName;
         mMaxTempFileName;
         mDiurnalTempFileName;
         mMeanPrecipFileName;
         mFrostDaysFileName;
         mTotalSolarRadFileName;
         mWindSpeedFileName;
         -----------------------------------------------------------------
         */
    qDebug( "Mean Temp FileName : " +  meanTempFileName().toLocal8Bit() );
    qDebug( "Max Temp FileName : " + maxTempFileName().toLocal8Bit() );
    qDebug( "Min Temp FileName : " + minTempFileName().toLocal8Bit() );
    qDebug( "Diurnal Temp FileName : " + diurnalTempFileName().toLocal8Bit() );
    qDebug( "Mean Precipitation FileName : " + meanPrecipFileName().toLocal8Bit() );
    qDebug( "Frost Days FileName : " + frostDaysFileName().toLocal8Bit() );
    qDebug( "Total Solar Radiation FileName : " + totalSolarRadFileName().toLocal8Bit() );
    qDebug( "Wind Speed FileName : " + windSpeedFileName().toLocal8Bit() );

    if (mDiurnalTempFileName != "")
    {
        //declare the key value
        myString=QString("Annual mean diurnal temperature range");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }

    if (mFrostDaysFileName != "")
    {
        //declare the key value
        myString=QString("Annual mean number of frost days");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }

    if (mTotalSolarRadFileName != "")
    {
        qDebug( "Solar incident radiation : " +    mTotalSolarRadFileName.toLocal8Bit() );
        //declare the key value
        myString=QString("Annual mean total incident solar radiation");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }

    if (mpMaxTempFileGroup && mMinTempFileName  != "" && mMaxTempFileName != "" )
    {
        //declare the key value
        myString=QString("Annual temperature range");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }

    if (mMaxTempFileName != "")
    {
        //declare the key value
        myString=QString("Highest temperature in warmest month");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }

    if (mMinTempFileName != "")
    {
        //declare the key value
        myString=QString("Lowest temperature in coolest month");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if (mMeanPrecipFileName != "")
    {
        //declare the key value
        myString=QString("Mean daily precipitation");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if (mMeanPrecipFileName != "" && mMeanTempFileName != "" )
    {
        //declare the key value
        myString=QString("Mean daily precipitation in coolest month");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if (mMeanPrecipFileName != "" && mMeanTempFileName != "" )
    {
        //declare the key value
        myString=QString("Mean daily precipitation in coolest quarter");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if (mMeanPrecipFileName != "" )
    {
        //declare the key value
        myString=QString("Mean daily precipitation in driest month");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if (mMeanPrecipFileName != "" )
    {
        //declare the key value
        myString=QString("Mean daily precipitation in driest quarter");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if (mMeanPrecipFileName != ""  &&  mMeanTempFileName != ""  )
    {
        //declare the key value
        myString=QString("Mean daily precipitation in warmest month");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if (mMeanPrecipFileName != ""  && mMeanTempFileName != ""  )
    {
        //declare the key value
        myString=QString("Mean daily precipitation in warmest quarter");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if (mMeanPrecipFileName != ""  )
    {
        //declare the key value
        myString=QString("Mean daily precipitation in wettest month");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if (mMeanPrecipFileName != ""  )
    {
        //declare the key value
        myString=QString("Mean daily precipitation in wettest quarter");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if (mDiurnalTempFileName != "" && mMeanTempFileName !="")
    {
        //declare the key value
        myString=QString("Mean diurnal temperature range in coolest month");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if ( mDiurnalTempFileName != "" && mMeanTempFileName !="")
    {
        //declare the key value
        myString=QString("Mean diurnal temperature range in warmest month");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if (  mMeanPrecipFileName != ""  &&  mFrostDaysFileName != "")
    {
        //declare the key value
        myString=QString("Mean precipitation in frost free months");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if ( mMeanTempFileName !="")
    {
        //declare the key value
        myString=QString("Mean temperature");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if (mMeanTempFileName !="")
    {
        //declare the key value
        myString=QString("Mean temperature in coolest month");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if ( mMeanTempFileName !="")
    {
        //declare the key value
        myString=QString("Mean temperature in coolest quarter");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if ( mMeanTempFileName !="" &&  mFrostDaysFileName != "")
    {
        //declare the key value
        myString=QString("Mean temperature in frost free months");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if ( mMeanTempFileName !="")
    {
        //declare the key value
        myString=QString("Mean temperature in warmest month");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if ( mMeanTempFileName !="")
    {
        //declare the key value
        myString=QString("Mean temperature in warmest quarter");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if ( mWindSpeedFileName != "")
    {
        //declare the key value
        myString=QString("Mean wind speed");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if ( mMinTempFileName !="")
    {
        //declare the key value
        myString=QString("Number of months with minimum temperature above freezing");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if ( mTotalSolarRadFileName != "" && mMeanTempFileName !="")
    {
        //declare the key value
        myString=QString("Radiation in coolest month");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if ( mTotalSolarRadFileName != ""  && mMeanTempFileName !="")
    {
        //declare the key value
        myString=QString("Radiation in coolest quarter");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if ( mTotalSolarRadFileName != "" && mMeanPrecipFileName != "")
    {
        //declare the key value
        myString=QString("Radiation in driest month");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if (mTotalSolarRadFileName != "" && mMeanPrecipFileName != "")
    {
        //declare the key value
        myString=QString("Radiation in driest quarter");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if ( mTotalSolarRadFileName != ""  && mMeanTempFileName != "")
    {
        //declare the key value
        myString=QString("Radiation in warmest month");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if ( mTotalSolarRadFileName != ""  && mMeanTempFileName != "")
    {
        //declare the key value
        myString=QString("Radiation in warmest quarter");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if ( mTotalSolarRadFileName != "" && mMeanPrecipFileName != "")
    {
        //declare the key value
        myString=QString("Radiation in wettest month");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if ( mTotalSolarRadFileName != ""  && mMeanPrecipFileName != "")
    {
        //declare the key value
        myString=QString("Radiation in wettest quarter");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if ( mMeanPrecipFileName != "")
    {
        //declare the key value
        myString=QString("Standard deviation of mean precipitation");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }
    if (mMeanTempFileName != "")
    {
        //declare the key value
        myString=QString("Standard deviation of mean temperature");
        //add it to the associative array
        mAvailableCalculationsMap[myString]=myBool;
    }

    //presume all went ok - need to add better error checking later
    return true;
} //end of makeAvailableCalculationsMap

/** Return the map of available calculations */
QMap <QString, bool > ClimateDataProcessorController::availableCalculationsMap()
{

    return  mAvailableCalculationsMap;

}

bool ClimateDataProcessorController::addUserCalculation(QString theCalculationName)
{
    QMap<QString, bool>::iterator myMapIterator = mAvailableCalculationsMap.begin();
    myMapIterator = mAvailableCalculationsMap.find(theCalculationName);

    if (myMapIterator != mAvailableCalculationsMap.end())
    {
        myMapIterator.value()=true;
        //presume all went ok - need to add better error checking later
        return true;
    }
    else
    {
        return false;
    }
}





/** Read property of bool mFilesInSeriesFlag. */
const bool ClimateDataProcessorController::filesInSeriesFlag()
{
    return mFilesInSeriesFlag;
}
/** Write property of bool mFilesInSeriesFlag. */
void ClimateDataProcessorController::setFilesInSeriesFlag( const bool theFlag)
{
    mFilesInSeriesFlag = theFlag;
}

/** Start the data analysis process. When everything else is set up,
  this is the method to call! */
bool ClimateDataProcessorController::run()
{
  qDebug( "ClimateDataProcessorController run() method called.\n " );
  qDebug( "Go and have a cup of tea cause this may take a while!" );
  qDebug( "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" );

  //create a data processor to do the low level processing
  ClimateDataProcessor myDataProcessor;
  //work out how many variables we are going to calculate
  int myVariableCount = 0;
  QMap<QString, bool>::const_iterator myIter;
  for (myIter=mAvailableCalculationsMap.begin(); myIter != mAvailableCalculationsMap.end(); myIter++)
  {
    if (myIter.value()) //true
    {
      myVariableCount++;
    }
  }
  makeFileGroups();
  //
  //work out how many cells need to be processed for each calculations
  //
  int myCellCount =0;
  int myXDim= 0;
  int myYDim=0;
  if (meanTempFileGroup)
  {
    myCellCount = meanTempFileGroup->elementCount();
    myXDim=meanTempFileGroup->xDim();
    myYDim=meanTempFileGroup->yDim();
  }
  else if (mpMinTempFileGroup)
  {
    myCellCount = mpMinTempFileGroup->elementCount();
    myXDim=mpMinTempFileGroup->xDim();
    myYDim=mpMinTempFileGroup->yDim();
  }
  else if (mpMaxTempFileGroup)
  {
    myCellCount = mpMaxTempFileGroup->elementCount();
    myXDim=mpMaxTempFileGroup->xDim();
    myYDim=mpMaxTempFileGroup->yDim();
  }
  else if (mpDiurnalTempFileGroup)
  {
    myCellCount = mpDiurnalTempFileGroup->elementCount();
    myXDim=mpDiurnalTempFileGroup->xDim();
    myYDim=mpDiurnalTempFileGroup->yDim();
  }
  else if (mpMeanPrecipFileGroup)
  {
    myCellCount = mpMeanPrecipFileGroup->elementCount();
    myXDim=mpMeanPrecipFileGroup->xDim();
    myYDim=mpMeanPrecipFileGroup->yDim();
  }
  else if (mpFrostDaysFileGroup)
  {
    myCellCount = mpFrostDaysFileGroup->elementCount();
    myXDim=mpFrostDaysFileGroup->xDim();
    myYDim=mpFrostDaysFileGroup->yDim();
  }
  else if (mpTotalSolarRadFileGroup)
  {
    myCellCount = mpTotalSolarRadFileGroup->elementCount();
    myXDim=mpTotalSolarRadFileGroup->xDim();
    myYDim=mpTotalSolarRadFileGroup->yDim();
  }
  else if (mpWindSpeedFileGroup)
  {
    myCellCount = mpWindSpeedFileGroup->elementCount();
    myXDim=mpWindSpeedFileGroup->xDim();
    myYDim=mpWindSpeedFileGroup->yDim();
  }
  //check nothing fishy is going on
  if (myCellCount ==  0)
  {
    return false;
  }


  //send signals so progress monitors can set themselves up
  emit numberOfCellsToCalc(myCellCount);
  emit numberOfVariablesToCalc(myVariableCount);


  //create a filewriter map for storing the OUTPUTS of each selected user calculation
  //this is not very element - I put the filewriter pointer into a struct and then  put the struct in to the
  //map, because I cant seem to be able to put the pointer directly into the map itself :-(
  QMap<QString, FileWriterStruct> myFileWriterMap;

  for (myIter=mAvailableCalculationsMap.begin(); myIter != mAvailableCalculationsMap.end(); myIter++)
  {
    if (myIter.value()) //true
    {
      qDebug( "Adding " +  myIter.key().toLocal8Bit() + " to myFileWriterMap\n");
      //create the fileWriter object
      //note I am not using pointer & the 'new' keyword here
      //because map doesnt let me add a pointer to the list

      QString myFileName = myIter.key();

      //replace any spaces with underscores in the name
      myFileName = myFileName.replace( QRegExp(" "), "_");
      //set the extension
      myFileName =  mOutputPath + myFileName + ".asc";
      FileWriter * myFileWriter = new FileWriter(myFileName,mOutputFileType);
      // add check to make sure header was initialised
      myFileWriter->writeString(mOutputHeader);


      qDebug( "Added " + myFileWriter->fileName().toLocal8Bit() );
      FileWriterStruct myFileWriterStruct;
      myFileWriterStruct.fileWriter=myFileWriter;
      myFileWriterStruct.fullFileName=myFileName;
      //add it to the map
      myFileWriterMap[myIter.key()]=myFileWriterStruct;
      qDebug( "Added " + myFileWriterStruct.fullFileName.toLocal8Bit() );
    }
  }


  // cycle through each FileGroup, fetching the element array from it and running any user
  // selected calculations, then writing the outputs to its associated filewriter in
  // myFileWriterMap


  if (meanTempFileGroup && mMeanTempFileName !="" &&
          mAvailableCalculationsMap["Mean temperature"])
  {
    emit variableStart("Mean temperature");
    qDebug( "ClimateDataProcessorController::run - Mean temperature requested" );
    //move to start of the current data matrix
    meanTempFileGroup->rewind();

    // get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean temperature"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;

    int myXCountInt=0;
    bool myFirstIterationFlag=true;
    while (!meanTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = meanTempFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.mean(myVector );
      if (myFirstIterationFlag || meanTempFileGroup->isAtMatrixEnd())
      {
        //this next bit is just for debugging purposes"
        printVectorAndResult(myVector,myFloat);
        myFirstIterationFlag=false;
      }
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      ////emit cellDone( myFloat );
    }
    qDebug( " ++++++ Emitting variableDone signal! " );
    emit variableDone(myFileWriterStruct.fullFileName);
    myFileWriter->close();

  }
  if (mpDiurnalTempFileGroup && mDiurnalTempFileName != "" &&
          mAvailableCalculationsMap["Annual mean diurnal temperature range"])
  {
    emit variableStart("Annual mean diurnal temperature range");
    qDebug( "ClimateDataProcessorController::run Annual mean diurnal temperature range requested" );
    //move to start of the current data matrix
    mpDiurnalTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Annual mean diurnal temperature range"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpDiurnalTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = mpDiurnalTempFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.mean(myVector );
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      ////emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }

  if (mpFrostDaysFileGroup && mFrostDaysFileName != "" &&
          mAvailableCalculationsMap["Annual mean number of frost days"])
  {
    emit variableStart("Annual mean number of frost days");
    qDebug( "ClimateDataProcessorController::run Annual mean number of frost days requested" );
    //move to start of the current data matrix
    mpFrostDaysFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Annual mean number of frost days"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpFrostDaysFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = mpFrostDaysFileGroup->getElementVector();

      //we are using mean over year summary
      float myFloat = myDataProcessor.mean(myVector );
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      ////emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }

  if (mpTotalSolarRadFileGroup && mTotalSolarRadFileName != "" &&
          mAvailableCalculationsMap["Annual mean total incident solar radiation"])
  {
    emit variableStart("Annual mean total incident solar radiation");
    qDebug( "ClimateDataProcessorController::run Annual mean total incident solar radiation requested" );
    //move to start of the current data matrix
    mpTotalSolarRadFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Annual mean total incident solar radiation"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpTotalSolarRadFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = mpTotalSolarRadFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.mean(myVector );
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }

  if ( mpMinTempFileGroup
          && mpMaxTempFileGroup
          && mMinTempFileName != ""
          && mMaxTempFileName != ""
          && mAvailableCalculationsMap["Annual temperature range"])
  {

    emit variableStart("Annual temperature range");
    qDebug( "ClimateDataProcessorController::run - Annual temperature range requested" );
    //move to start of the current data matrix
    mpMinTempFileGroup->rewind();
    mpMaxTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Annual temperature range"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpMinTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector, myVector2;
      //get the next element from the file group
      myVector = mpMinTempFileGroup->getElementVector();
      myVector2 = mpMaxTempFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.greatestTotalRange(myVector,myVector2);
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }

  if (mpMaxTempFileGroup
          && mMaxTempFileName != ""
          && mAvailableCalculationsMap["Highest temperature in warmest month"])
  {
    emit variableStart("Highest temperature in warmest month");
    qDebug( "ClimateDataProcessorController::run - Highest temperature in warmest month requested" );
    mpMaxTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Highest temperature in warmest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpMaxTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = mpMaxTempFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.highestValue(myVector);
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }

  if (mpMinTempFileGroup
          && mMinTempFileName != ""
          && mAvailableCalculationsMap["Lowest temperature in coolest month"])
  {
    emit variableStart("Lowest temperature in coolest month");
    qDebug( "ClimateDataProcessorController::run - Lowest temperature in coolest month requested" );
    mpMinTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Lowest temperature in coolest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpMinTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = mpMinTempFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.lowestValue(myVector);
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);

  }
  if (mpMeanPrecipFileGroup
          && mMeanPrecipFileName != ""
          && mAvailableCalculationsMap["Mean daily precipitation"])
  {
    emit variableStart("Mean daily precipitation");
    qDebug( "ClimateDataProcessorController::run Mean daily precipitation" );
    mpMeanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean daily precipitation"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpMeanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = mpMeanPrecipFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.mean(myVector );
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if ( mpMeanPrecipFileGroup &&  mpMinTempFileGroup
          && mMeanPrecipFileName != "" && mMeanTempFileName != ""
          && mAvailableCalculationsMap["Mean daily precipitation in coolest month"])
  {
    emit variableStart("Mean daily precipitation in coolest month");
    qDebug( "ClimateDataProcessorController::run Mean daily precipitation in coolest month" );
    mpMeanPrecipFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean daily precipitation in coolest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpMeanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = mpMeanPrecipFileGroup->getElementVector();
      myVector2 = meanTempFileGroup->getElementVector();
      //we are using mean over year summary
      int myBlockInt = myDataProcessor.monthWithLowestValue(myVector2);
      float myFloat=NO_DATA;
      if (static_cast<int>(myBlockInt) != NO_DATA)
      {
          myFloat  = myDataProcessor.valueGivenMonth(myVector,myBlockInt);
      }
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpMeanPrecipFileGroup &&  mpMinTempFileGroup && mMeanPrecipFileName != ""
          && mMeanTempFileName != ""
          && mAvailableCalculationsMap["Mean daily precipitation in coolest quarter"])
  {
    emit variableStart("Mean daily precipitation in coolest quarter");
    qDebug( "ClimateDataProcessorController::run Mean daily precipitation in coolest month" );
    mpMeanPrecipFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean daily precipitation in coolest quarter"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpMeanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = mpMeanPrecipFileGroup->getElementVector();
      myVector2 = meanTempFileGroup->getElementVector();
      //we are using mean over year summary
      int myBlockInt = myDataProcessor.firstMonthOfLowestQ(myVector2);
      float myFloat=NO_DATA;
      if (static_cast<int>(myBlockInt) != NO_DATA)
      {
         myFloat  = myDataProcessor.meanOverQuarter(myVector,myBlockInt);
      }
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpMeanPrecipFileGroup &&  mMeanPrecipFileName != ""  &&
          mAvailableCalculationsMap["Mean daily precipitation in driest month"])
  {
    emit variableStart("Mean daily precipitation in driest month");
    qDebug( "ClimateDataProcessorController::run Mean daily precipitation in driest month" );
    mpMeanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean daily precipitation in driest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpMeanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = mpMeanPrecipFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.lowestValue(myVector );
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpMeanPrecipFileGroup &&  mMeanPrecipFileName != ""  &&
          mAvailableCalculationsMap["Mean daily precipitation in driest quarter"])
  {
    emit variableStart("Mean daily precipitation in driest quarter");
    qDebug( "ClimateDataProcessorController::run Mean daily precipitation in driest quarter" );
    mpMeanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean daily precipitation in driest quarter"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpMeanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = mpMeanPrecipFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.meanOverLowestQ(myVector);
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpMeanPrecipFileGroup &&  mMeanPrecipFileName != ""
          && meanTempFileGroup
          && mMeanTempFileName != ""
          && mAvailableCalculationsMap["Mean daily precipitation in warmest month"])
  {
    emit variableStart("Mean daily precipitation in warmest month");
    qDebug( "ClimateDataProcessorController::run Mean daily precipitation in warmest month" );
    //move to the start of data blocks
    mpMeanPrecipFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean daily precipitation in warmest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpMeanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = mpMeanPrecipFileGroup->getElementVector();
      myVector2 = meanTempFileGroup->getElementVector();
      //we are using mean over year summary
      int myBlockInt = myDataProcessor.monthWithHighestValue(myVector2);
      float myFloat=NO_DATA;
      if (static_cast<int>(myBlockInt) != NO_DATA)
      {
        myFloat  = myDataProcessor.valueGivenMonth(myVector,myBlockInt);
      }
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpMeanPrecipFileGroup &&  mMeanPrecipFileName != ""
          && meanTempFileGroup &&  mMeanTempFileName != ""
          &&mAvailableCalculationsMap["Mean daily precipitation in warmest quarter"])
  {
    emit variableStart("Mean daily precipitation in warmest quarter");
    qDebug( "ClimateDataProcessorController::run Mean daily precipitation in warmest quarter" );
    mpMeanPrecipFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean daily precipitation in warmest quarter"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpMeanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = mpMeanPrecipFileGroup->getElementVector();
      myVector2 = meanTempFileGroup->getElementVector();
      //we are using mean over year summary
      int myCurrentBlockOfWarmestQuarterInt = myDataProcessor.firstMonthOfHighestQ(myVector2);
      float myFloat=NO_DATA;
      if (static_cast<int>(myCurrentBlockOfWarmestQuarterInt) != NO_DATA)
      {
        myFloat = myDataProcessor.meanOverQuarter(myVector,myCurrentBlockOfWarmestQuarterInt);
      }
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpMeanPrecipFileGroup &&  mMeanPrecipFileName != ""   &&
          mAvailableCalculationsMap["Mean daily precipitation in wettest month"])
  {
    emit variableStart("Mean daily precipitation in wettest month");
    qDebug( "ClimateDataProcessorController::run Mean daily precipitation in wettest month" );
    mpMeanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean daily precipitation in wettest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpMeanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = mpMeanPrecipFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.highestValue(myVector );
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpMeanPrecipFileGroup &&  mMeanPrecipFileName != ""   &&
          mAvailableCalculationsMap["Mean daily precipitation in wettest quarter"])
  {
    emit variableStart("Mean daily precipitation in wettest quarter");
    qDebug( "ClimateDataProcessorController::run Mean daily precipitation in wettest quarter" );
    mpMeanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean daily precipitation in wettest quarter"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpMeanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float > myVector;
      //get the next element from the file group
      myVector = mpMeanPrecipFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.meanOverHighestQ(myVector);
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpDiurnalTempFileGroup && mDiurnalTempFileName != ""
          && meanTempFileGroup
          && mMeanTempFileName !=""
          && mAvailableCalculationsMap["Mean diurnal temperature range in coolest month"])
  {
    emit variableStart("Mean diurnal temperature range in coolest month");
    qDebug( "ClimateDataProcessorController::run Mean diurnal temperature range in coolest month" );
    mpDiurnalTempFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean diurnal temperature range in coolest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpDiurnalTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = mpDiurnalTempFileGroup->getElementVector();
      myVector2 = meanTempFileGroup->getElementVector();
      //we are using mean over year summary
      int myCoolestBlockInt = myDataProcessor.monthWithLowestValue(myVector2);
      float myFloat=NO_DATA;
      if (static_cast<int>(myCoolestBlockInt) != NO_DATA)
      {
        myFloat = myDataProcessor.valueGivenMonth(myVector,myCoolestBlockInt);
      }
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpDiurnalTempFileGroup && mDiurnalTempFileName != "" && meanTempFileGroup
          && mMeanTempFileName !=""
          && mAvailableCalculationsMap["Mean diurnal temperature range in warmest month"])
  {
    emit variableStart("Mean diurnal temperature range in warmest month");
    qDebug( "ClimateDataProcessorController::run Mean diurnal temperature range in warmest month" );
    mpDiurnalTempFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean diurnal temperature range in warmest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpDiurnalTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = mpDiurnalTempFileGroup->getElementVector();
      myVector2 = meanTempFileGroup->getElementVector();
      //we are using mean over year summary
      int myCoolestBlockInt = myDataProcessor.monthWithHighestValue(myVector2);
      float myFloat=NO_DATA;
      if (static_cast<int>(myCoolestBlockInt) != NO_DATA)
      {
        myFloat = myDataProcessor.valueGivenMonth(myVector,myCoolestBlockInt);
      }
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpMeanPrecipFileGroup &&  mMeanPrecipFileName != ""  && mpFrostDaysFileGroup
          && mFrostDaysFileName != ""
          && mAvailableCalculationsMap["Mean precipitation in frost free months"])
  {
    emit variableStart("Mean precipitation in frost free months");
    qDebug( "ClimateDataProcessorController::run Mean precipitation in frost free months" );
    mpMeanPrecipFileGroup->rewind();
    mpFrostDaysFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean precipitation in frost free months"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpMeanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = mpMeanPrecipFileGroup->getElementVector();
      myVector2 = mpFrostDaysFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.meanValueOverFrostFreeMonths(myVector2, myVector);
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }

  if (meanTempFileGroup && mMeanTempFileName !="" &&
          mAvailableCalculationsMap["Mean temperature in coolest month"])
  {
    emit variableStart("Mean temperature in coolest month");
    qDebug( "ClimateDataProcessorController::run Mean temperature in coolest month" );

    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean temperature in coolest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!meanTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = meanTempFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.lowestValue(myVector );
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (meanTempFileGroup && mMeanTempFileName !="" &&
          mAvailableCalculationsMap["Mean temperature in coolest quarter"])
  {
    emit variableStart("Mean temperature in coolest quarter");
    qDebug( "ClimateDataProcessorController::run Mean temperature in coolest quarter" );
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean temperature in coolest quarter"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!meanTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = meanTempFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.meanOverLowestQ(myVector);
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (meanTempFileGroup && mMeanTempFileName !="" && mpFrostDaysFileGroup
          && mFrostDaysFileName != ""
          && mAvailableCalculationsMap["Mean temperature in frost free months"])
  {
    emit variableStart("Mean temperature in frost free months");
    qDebug( "ClimateDataProcessorController::run Mean temperature in frost free months" );
    meanTempFileGroup->rewind();
    mpFrostDaysFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean temperature in frost free months"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!meanTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = meanTempFileGroup->getElementVector();
      myVector2 = mpFrostDaysFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.meanValueOverFrostFreeMonths(myVector2, myVector);
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (meanTempFileGroup && mMeanTempFileName !="" &&
          mAvailableCalculationsMap["Mean temperature in warmest month"])
  {
    emit variableStart("Mean temperature in warmest month");
    qDebug( "ClimateDataProcessorController::run Mean temperature in warmest month" );
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean temperature in warmest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!meanTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = meanTempFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.highestValue(myVector );
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (meanTempFileGroup && mMeanTempFileName !="" &&
          mAvailableCalculationsMap["Mean temperature in warmest quarter"])
  {
    emit variableStart("Mean temperature in warmest quarter");
    qDebug( "ClimateDataProcessorController::run Mean temperature in warmest quarter" );
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean temperature in warmest quarter"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!meanTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = meanTempFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.meanOverHighestQ(myVector);
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpWindSpeedFileGroup && mWindSpeedFileName != "" &&
          mAvailableCalculationsMap["Mean wind speed"])
  {
    emit variableStart("Mean wind speed");
    qDebug( "ClimateDataProcessorController::run Mean wind speed" );
    mpWindSpeedFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean wind speed"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpWindSpeedFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = mpWindSpeedFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.mean(myVector );
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpMinTempFileGroup && mMinTempFileName !="" &&
          mAvailableCalculationsMap["Number of months with minimum temperature above freezing"])
  {
    emit variableStart("Number of months with minimum temperature above freezing");
    qDebug( "ClimateDataProcessorController::run Number of months with minimum temperature above freezing" );
    mpMinTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Number of months with minimum temperature above freezing"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpMinTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = mpMinTempFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.numberOfMonthsAboveZero(myVector );
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpTotalSolarRadFileGroup && mTotalSolarRadFileName != ""
          && meanTempFileGroup && mMeanTempFileName !=""
          && mAvailableCalculationsMap["Radiation in coolest month"])
  {
    emit variableStart("Radiation in coolest quarter");
    qDebug( "ClimateDataProcessorController::run Radiation in coolest month" );
    mpTotalSolarRadFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Radiation in coolest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpTotalSolarRadFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = mpTotalSolarRadFileGroup->getElementVector();
      myVector2 = meanTempFileGroup->getElementVector();
      //we are using mean over year summary
      int myCoolestBlockInt = myDataProcessor.monthWithLowestValue(myVector2);
      float myFloat=NO_DATA;
      if (static_cast<int>(myCoolestBlockInt) != NO_DATA)
      {
        myFloat = myDataProcessor.valueGivenMonth(myVector,myCoolestBlockInt );
      }
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpTotalSolarRadFileGroup && mTotalSolarRadFileName != ""
          && meanTempFileGroup && mMeanTempFileName !=""
          && mAvailableCalculationsMap["Radiation in coolest quarter"])
  {
    emit variableStart("Radiation in coolest quarter");
    qDebug( "ClimateDataProcessorController::run Radiation in coolest quarter" );
    mpTotalSolarRadFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Radiation in coolest quarter"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpTotalSolarRadFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = mpTotalSolarRadFileGroup->getElementVector();
      myVector2 = meanTempFileGroup->getElementVector();
      //we are using mean over year summary
      int myFirstBlockInt = myDataProcessor.firstMonthOfLowestQ(myVector2);
      float myFloat=NO_DATA;
      if (static_cast<int>(myFirstBlockInt) != NO_DATA)
      {
        myFloat = myDataProcessor.meanOverQuarter(myVector,myFirstBlockInt );
      }
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpTotalSolarRadFileGroup && mTotalSolarRadFileName != ""
          && meanTempFileGroup && mMeanTempFileName != ""
          && mAvailableCalculationsMap["Radiation in warmest month"])
  {
    emit variableStart("Radiation in warmest month");
    qDebug( "ClimateDataProcessorController::run Radiation in warmest month" );
    mpTotalSolarRadFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Radiation in warmest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpTotalSolarRadFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = mpTotalSolarRadFileGroup->getElementVector();
      myVector2 = meanTempFileGroup->getElementVector();
      //we are using mean over year summary
      int myHighestBlockInt = myDataProcessor.monthWithHighestValue(myVector2);
      float myFloat=NO_DATA;
      if (static_cast<int>(myHighestBlockInt) != NO_DATA)
      {
        myFloat = myDataProcessor.valueGivenMonth(myVector,myHighestBlockInt );
      }
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpTotalSolarRadFileGroup && mTotalSolarRadFileName != ""
          && meanTempFileGroup && mMeanTempFileName != ""
          && mAvailableCalculationsMap["Radiation in warmest quarter"])
  {
    emit variableStart("Radiation in warmest quarter");
    qDebug( "ClimateDataProcessorController::run Radiation in warmest quarter" );
    mpTotalSolarRadFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Radiation in warmest quarter"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpTotalSolarRadFileGroup->isAtMatrixEnd())
    {

      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = mpTotalSolarRadFileGroup->getElementVector();
      myVector2 = meanTempFileGroup->getElementVector();
      //we are using mean over year summary
      int myFirstBlockInt = myDataProcessor.firstMonthOfHighestQ(myVector2);
      float myFloat=NO_DATA;
      if (static_cast<int>(myFirstBlockInt) != NO_DATA)
      {
        myFloat = myDataProcessor.meanOverQuarter(myVector,myFirstBlockInt );
      }
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpTotalSolarRadFileGroup && mTotalSolarRadFileName != ""
          && mpMeanPrecipFileGroup && mMeanPrecipFileName != ""
          && mAvailableCalculationsMap["Radiation in driest month"])
  {
    emit variableStart("Radiation in driest month");
    qDebug( "ClimateDataProcessorController::run Radiation in driest month" );
    mpTotalSolarRadFileGroup->rewind();
    mpMeanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Radiation in driest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpTotalSolarRadFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = mpTotalSolarRadFileGroup->getElementVector();
      myVector2 = mpMeanPrecipFileGroup->getElementVector();
      //we are using mean over year summary
      int myDriestBlockInt = myDataProcessor.monthWithLowestValue(myVector2);
      float myFloat=NO_DATA;
      if (static_cast<int>(myDriestBlockInt) != NO_DATA)
      {
        myFloat = myDataProcessor.valueGivenMonth(myVector,myDriestBlockInt );
      }
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpTotalSolarRadFileGroup && mTotalSolarRadFileName != ""
          && mpMeanPrecipFileGroup && mMeanPrecipFileName != ""
          && mAvailableCalculationsMap["Radiation in driest quarter"])
  {
    emit variableStart("Radiation in driest quarter");
    qDebug( "ClimateDataProcessorController::run Radiation in driest quarter" );
    mpTotalSolarRadFileGroup->rewind();
    mpMeanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Radiation in driest quarter"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpTotalSolarRadFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = mpTotalSolarRadFileGroup->getElementVector();
      myVector2 = mpMeanPrecipFileGroup->getElementVector();
      //we are using mean over year summary
      int myFirstBlockInt = myDataProcessor.firstMonthOfLowestQ(myVector2);
      float myFloat=NO_DATA;
      if (static_cast<int>(myFirstBlockInt) != NO_DATA)
      {
        myFloat = myDataProcessor.meanOverQuarter(myVector,myFirstBlockInt );
      }
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }

  if (mpTotalSolarRadFileGroup && mTotalSolarRadFileName != ""
          && mpMeanPrecipFileGroup && mMeanPrecipFileName != ""
          && mAvailableCalculationsMap["Radiation in wettest month"])
  {
    emit variableStart("Radiation in wettest month");
    qDebug( "ClimateDataProcessorController::run Radiation in wettest month" );
    mpTotalSolarRadFileGroup->rewind();
    mpMeanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Radiation in wettest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpTotalSolarRadFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = mpTotalSolarRadFileGroup->getElementVector();
      myVector2 = mpMeanPrecipFileGroup->getElementVector();
      //we are using mean over year summary
      int myWettestBlockInt = myDataProcessor.monthWithHighestValue(myVector2);
      float myFloat=NO_DATA;
      if (static_cast<int>(myWettestBlockInt) != NO_DATA)
      {
        myFloat = myDataProcessor.valueGivenMonth(myVector,myWettestBlockInt );
      }
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpTotalSolarRadFileGroup && mTotalSolarRadFileName != ""
          && mpMeanPrecipFileGroup && mMeanPrecipFileName != ""
          && mAvailableCalculationsMap["Radiation in wettest quarter"])
  {
    emit variableStart("Radiation in wettest quarter");
    qDebug( "ClimateDataProcessorController::run Radiation in wettest quarter" );
    mpTotalSolarRadFileGroup->rewind();
    mpMeanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Radiation in wettest quarter"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpTotalSolarRadFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = mpTotalSolarRadFileGroup->getElementVector();
      myVector2 = mpMeanPrecipFileGroup->getElementVector();
      //we are using mean over year summary
      int myFirstBlockInt = myDataProcessor.firstMonthOfHighestQ(myVector2);
      float myFloat=NO_DATA;
      if (static_cast<int>(myFirstBlockInt) != NO_DATA)
      {
        myFloat = myDataProcessor.meanOverQuarter(myVector,myFirstBlockInt );
      }
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (mpMeanPrecipFileGroup && mMeanPrecipFileName != "" &&
          mAvailableCalculationsMap["Standard deviation of mean precipitation"])
  {
    emit variableStart("Standard deviation of mean precipitation");
    qDebug( "ClimateDataProcessorController::run Standard deviation of mean precipitation" );
    mpMeanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Standard deviation of mean precipitation"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!mpMeanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = mpMeanPrecipFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.stddevOverYear(myVector);
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (meanTempFileGroup && mMeanTempFileName != ""
          && mAvailableCalculationsMap["Standard deviation of mean temperature"])
  {
    emit variableStart("Standard deviation of mean temperature");
    qDebug( "ClimateDataProcessorController::run Standard deviation of mean temperature" );
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Standard deviation of mean temperature"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!meanTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = meanTempFileGroup->getElementVector();
      //we are using mean over year summary
      float myFloat = myDataProcessor.stddevOverYear(myVector);
      //write the result to our output file
      bool myResultFlag = myFileWriter->writeElement( myFloat );
      if (!myResultFlag)
      {
        qDebug( "Error! Writing an element to " +  myFileWriterStruct.fullFileName.toLocal8Bit() +  " failed " );
        
        return false;
      }
      myXCountInt++;
      if (myXCountInt%myXDim==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    myFileWriter->close();
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  return true;
}

void ClimateDataProcessorController::printVectorAndResult(QVector<float> theVector, float theResult)
{
    int myVectorLengthInt=theVector.size();
    QString myString;
    for (int myInt = 0; myInt <= myVectorLengthInt-1; myInt++)
    {

        myString += QString::number(theVector[myInt]) + ",";
    }
    qDebug( myString.toLocal8Bit() + "\t : " + QString::number(theResult).toLocal8Bit() );
}

/** Return a nice summary about this ClimateDataProcessorController object */
QString ClimateDataProcessorController::description()
{
    //must have an include for sstream.h!

    QString myString, myNumber;
    myString += "\n Climate Data Processor Description \n";
    myString += " ---------------------------------- \n";


    myNumber = QString::number(inputFileType());
    myString += QString("Input File Type Enum : ") + myNumber+ QString("\n");

    myNumber = QString::number(outputFileType());
    myString += QString("Output File Type Enum : ") + myNumber + QString("\n");


    //these properties are just plain strings and dont need conversion
    myString += QString("Mean Temp FileName : ") + meanTempFileName() + QString("\n");
    myString += QString("Max Temp FileName : ") + maxTempFileName() + QString("\n");
    myString += QString("Min Temp FileName : ") + minTempFileName() + QString("\n");
    myString += QString("Diurnal Temp FileName : ") + diurnalTempFileName() + QString("\n");
    myString += QString("Mean Precipitation FileName : ") + meanPrecipFileName() + QString("\n");
    myString += QString("Frost Days FileName : ") + frostDaysFileName() + QString("\n");
    myString += QString("Total Solar Radiation FileName : ") + totalSolarRadFileName() + QString("\n");
    myString += QString("Wind Speed FileName : ") + windSpeedFileName() + QString("\n");
    if (mFilesInSeriesFlag)
    {
        myString += QString("Datafiles are a series of numbered files for each month \n");
    }
    else
    {
        myString += QString("Datafiles contain all monthly data in a single file \n");
    }

    //List the calculations in  mAvailableCalculationsMap  using an iterator
    myString += QString("Listing items in mAvailableCalculationsMap \n");
    myString += QString("Boolean value suffix indicates whether the user want to use this calculation \n");
    myString += QString("---------------------------------------------------------------------------------------------------- \n");
    QMap<QString, bool>::const_iterator myIter;
    for (myIter=mAvailableCalculationsMap.begin(); myIter != mAvailableCalculationsMap.end(); myIter++)
    {
        if (myIter.value())
        {
            myString += myIter.key() + QString(": true\n");
        }
        else
        {
            myString += myIter.key() + QString(": false\n");
        }
    }
    myString += QString("---------------------------------------------------------------------------------------------------- \n");

    return myString;

}




