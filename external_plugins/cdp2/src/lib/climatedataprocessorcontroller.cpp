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
    meanTempFileName=myString;
    minTempFileName=myString;
    maxTempFileName=myString;
    diurnalTempFileName=myString;
    meanPrecipFileName=myString;
    frostDaysFileName=myString;
    totalSolarRadFileName=myString;
    windSpeedFileName=myString;

    outputFilePath = myString;
    inputFileType=ClimateFileReader::GDAL;


}

/** Destructor */
ClimateDataProcessorController::~ClimateDataProcessorController()
{}


const QString ClimateDataProcessorController::getOutputHeader()
{
    return outputHeader;
}

void ClimateDataProcessorController::setOutputHeader( const QString& theOutputHeader)
{
    outputHeader = theOutputHeader;
}

bool ClimateDataProcessorController::makeInputFileTypeMap()
{
    QString myString;
    ClimateFileReader::FileType myFileType;
    inputFileTypeMap.clear();
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
    inputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("Hadley Centre HadCM3 SRES Scenario");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::HADLEY_SRES;
    //add it to the associative array
    inputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("Hadley Centre HadCM3 IS92a Scenario");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::HADLEY_IS92;
    //add it to the associative array
    inputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("IPCC Observed Climatology");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::IPCC_OBSERVED;
    //add it to the associative array
    inputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("University of Reading Palaeoclimate data");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::VALDES;
    //add it to the associative array
    inputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("Max Planck Institute fur Meteorologie (MPIfM) ECHAM4 data");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::ECHAM4;
    //add it to the associative array
    inputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("CSIRO-Mk2 Model data");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::CSIRO_MK2;
    //add it to the associative array
    inputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("National Center for Atmospheric Research (NCAR) NCAR-CSM and NCAR-PCM data");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::NCAR_CSM_PCM;
    //add it to the associative array
    inputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("Geophysical Fluid Dynamics Laboratory (GFDL) R30 Model data");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::GFDL_R30;
    //add it to the associative array
    inputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("Canadian Center for Climate Modelling and Analysis (CCCma) CGCM2 Model data");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::CGCM2;
    //add it to the associative array
    inputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("CCSR/NIES AGCM model data and CCSR OGCM model data");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=ClimateFileReader::CCSR_AGCM_OGCM;
    //add it to the associative array
    inputFileTypeMap[myString]=myFileType;

    return true;
}

bool ClimateDataProcessorController::makeOutputFileTypeMap()
{
    QString myString;
    FileWriter::FileType myFileType;
    outputFileTypeMap.clear();
    // Copied from FileWriter header file:
    //enum FileFormatEnum { GDAL_TIFF, CSM_MATLAB , CSM_OCTAVE ,  GARP ,  ESRI_ASCII ,  PLAIN };

    //declare the key value
    myString=QString("GDAL Tiff Image");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=FileWriter::ESRI_ASCII;
    //add it to the associative array
    outputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("Matlab");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=FileWriter::MATLAB;
    //add it to the associative array
    outputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("ESRI ASCII Grid");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=FileWriter::ESRI_ASCII;
    //add it to the associative array
    outputFileTypeMap[myString]=myFileType;

    //declare the key value
    myString=QString("Plain matrix with no header");
    //convert it to toUpper case
    myString=myString.toUpper();
    //set its associated enum
    myFileType=FileWriter::PLAIN;
    //add it to the associative array
    outputFileTypeMap[myString]=myFileType;
    return true;
}

const QString  ClimateDataProcessorController::getMeanTempFileName()
{
    return meanTempFileName;
}

void ClimateDataProcessorController::setMeanTempFileName( QString theFileName)
{
    meanTempFileName = theFileName;
    qDebug( "meanTempFileName set to : " + meanTempFileName.toLocal8Bit() );
}

const QString  ClimateDataProcessorController::getMinTempFileName()
{
    return minTempFileName;
}

void ClimateDataProcessorController::setMinTempFileName( QString theFileName)
{
    minTempFileName = theFileName;
    qDebug( "minTempFileName set to : " + minTempFileName.toLocal8Bit() );
}

const QString  ClimateDataProcessorController::getMaxTempFileName()
{
    return maxTempFileName;
}

void ClimateDataProcessorController::setMaxTempFileName( QString theFileName)
{
    maxTempFileName = theFileName;
    qDebug( "maxTempFileName set to : " + maxTempFileName.toLocal8Bit() );
}

const QString  ClimateDataProcessorController::getDiurnalTempFileName()
{
    return diurnalTempFileName;
}

void ClimateDataProcessorController::setDiurnalTempFileName( QString theFileName)
{
    diurnalTempFileName = theFileName;
    qDebug( "diurnalTempFileName set to : " + diurnalTempFileName.toLocal8Bit() );
}

const QString  ClimateDataProcessorController::getMeanPrecipFileName()
{
    return meanPrecipFileName;
}

void ClimateDataProcessorController::setMeanPrecipFileName( QString theFileName)
{
    meanPrecipFileName = theFileName;
    qDebug( "meanPrecipFileName set to : " + meanPrecipFileName.toLocal8Bit() );
}

const QString  ClimateDataProcessorController::getFrostDaysFileName()
{

    qDebug( "frostDaysFileName set to : " + frostDaysFileName.toLocal8Bit() );
    return frostDaysFileName;
}

void ClimateDataProcessorController::setFrostDaysFileName( QString theFileName)
{
    frostDaysFileName = theFileName;
    qDebug( "frostDaysFileName set to : " + frostDaysFileName.toLocal8Bit() );
}

const QString  ClimateDataProcessorController::getTotalSolarRadFileName()
{
    return totalSolarRadFileName;
}

void ClimateDataProcessorController::setTotalSolarRadFileName( QString theFileName)
{
    totalSolarRadFileName = theFileName;

    qDebug( "totalSolarRadFileName set to : " + totalSolarRadFileName.toLocal8Bit() );
}

const QString  ClimateDataProcessorController::getWindSpeedFileName()
{
    return windSpeedFileName;
}

void ClimateDataProcessorController::setWindSpeedFileName( QString theFileName)
{
    windSpeedFileName = theFileName;
    qDebug( "windSpeedFileName set to : " + windSpeedFileName.toLocal8Bit() );
}

const QString  ClimateDataProcessorController::getOutputFilePathString()
{
    return outputFilePath;
}

void ClimateDataProcessorController::setOutputFilePathString( QString theFilePathString)
{
    outputFilePath = theFilePathString;
    qDebug( "outputFilePath set to : " + outputFilePath.toLocal8Bit() );
}

bool ClimateDataProcessorController::makeFileGroups()
{

    /*   These are the possible filegroups available:
         -----------------------------------------------------------------
         meanTempFileGroup;
         minTempFileGroup;
         maxTempFileGroup;
         diurnalTempFileGroup;
         meanPrecipFileGroup;
         frostDaysFileGroup;
         totalSolarRadFileGroup;
         windSpeedFileGroup;
         -----------------------------------------------------------------
         */
    const int START_YEAR=1; //this will be deprecated soon!
    if (meanTempFileName==QString(""))
    {
        qDebug(     "makeFileGroups - meanTempFileName is NOT initialised! *****************************" );
    }
    else
    {
        meanTempFileGroup = initialiseFileGroup(meanTempFileName,START_YEAR);
    }

    if (minTempFileName==QString(""))
    {
        qDebug(     "makeFileGroups - minTempFileName is NOT initialised! *****************************" );
    }
    else
    {
        minTempFileGroup = initialiseFileGroup(minTempFileName,START_YEAR);
    }

    if (maxTempFileName==QString(""))
    {
        qDebug(     "makeFileGroups - maxTempFileName is NOT initialised! *****************************" );
    }
    else
    {
        maxTempFileGroup = initialiseFileGroup(maxTempFileName,START_YEAR);
    }

    if (diurnalTempFileName==QString(""))
    {
        qDebug(     "makeFileGroups - diurnalTempFileName is NOT initialised! *****************************" );
    }
    else
    {
        diurnalTempFileGroup = initialiseFileGroup(diurnalTempFileName,START_YEAR);
    }

    if (meanPrecipFileName==QString(""))
    {
        qDebug(     "makeFileGroups - meanPrecipFileName is NOT initialised! *****************************" );
    }
    else
    {
        meanPrecipFileGroup = initialiseFileGroup(meanPrecipFileName,START_YEAR);
    }

    if (meanPrecipFileName==QString(""))
    {
        qDebug(     "makeFileGroups - meanPrecipFileName is NOT initialised! *****************************" );
    }
    else
    {
        meanPrecipFileGroup = initialiseFileGroup(meanPrecipFileName,START_YEAR);
    }


    if (frostDaysFileName==QString(""))
    {
        qDebug(     "makeFileGroups - frostDaysFileName is NOT initialised! *****************************" );
    }
    else
    {
        frostDaysFileGroup = initialiseFileGroup(frostDaysFileName,START_YEAR);
    }


    if (totalSolarRadFileName==QString(""))
    {
        qDebug( "makeFileGroups - totalSolarRadFileName is NOT initialised! *****************************" );
    }
    else
    {
        totalSolarRadFileGroup = initialiseFileGroup(totalSolarRadFileName,START_YEAR);
    }


    if (windSpeedFileName==QString(""))
    {
        qDebug( "makeFileGroups - totalSolarRadFileName is NOT initialised! *****************************" );
    }
    else
    {
        windSpeedFileGroup = initialiseFileGroup(windSpeedFileName,START_YEAR);
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
    if (filesInSeriesFlag)
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
            ClimateFileReader *myClimateFileReader = new ClimateFileReader();
            myClimateFileReader->initialise(myCurrentFileName,inputFileType);            
            qDebug( "initialiseFileGroup - *** Adding " + myCurrentFileName.toLocal8Bit()
            + " to file group *********************" );
            myFileGroup->add(myClimateFileReader);

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
            myClimateFileReader->initialise(theFileName,inputFileType);            
            myClimateFileReader->setActiveBlock(myInt);
            myFileGroup->add(myClimateFileReader);
            qDebug( "Adding block " + QString::number(myInt).toLocal8Bit() + " to the block list to be processed" );
        }

        //ownership of the filereader pointer is transferred to the file group

        return myFileGroup;
    } //end of second part of files in series flag test

}


/** Read property of ClimateFileReader::FileType FileType. */
const ClimateFileReader::FileType ClimateDataProcessorController::getInputFileType()
{
    return inputFileType;
}
/** Write property of ClimateFileReader::FileType FileType. */
void ClimateDataProcessorController::setInputFileType( const ClimateFileReader::FileType theInputFileType)
{
    inputFileType = theInputFileType;
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
    myInputFileType = inputFileTypeMap[myString];
    //set the filetype given the enum
    setInputFileType (myInputFileType);

}


/** Read property of FileWriter::FileType outputFileType. */
const FileWriter::FileType ClimateDataProcessorController::getOutputFileType()
{
    return outputFileType;
}
/** Write property of FileWriter::FileType FileType. */
void ClimateDataProcessorController::setOutputFileType( const FileWriter::FileType theOutputFileType)
{
    outputFileType = theOutputFileType;
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
    myOutputFileType = outputFileTypeMap[myString];
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
    availableCalculationsMap.clear();
    /*   These are the possible filegroups available:
         -----------------------------------------------------------------
         meanTempFileName;
         minTempFileName;
         maxTempFileName;
         diurnalTempFileName;
         meanPrecipFileName;
         frostDaysFileName;
         totalSolarRadFileName;
         windSpeedFileName;
         -----------------------------------------------------------------
         */
    qDebug( "Mean Temp FileName : " +  getMeanTempFileName().toLocal8Bit() );
    qDebug( "Max Temp FileName : " + getMaxTempFileName().toLocal8Bit() );
    qDebug( "Min Temp FileName : " + getMinTempFileName().toLocal8Bit() );
    qDebug( "Diurnal Temp FileName : " + getDiurnalTempFileName().toLocal8Bit() );
    qDebug( "Mean Precipitation FileName : " + getMeanPrecipFileName().toLocal8Bit() );
    qDebug( "Frost Days FileName : " + getFrostDaysFileName().toLocal8Bit() );
    qDebug( "Total Solar Radiation FileName : " + getTotalSolarRadFileName().toLocal8Bit() );
    qDebug( "Wind Speed FileName : " + getWindSpeedFileName().toLocal8Bit() );

    if (diurnalTempFileName != "")
    {
        //declare the key value
        myString=QString("Annual mean diurnal temperature range");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }

    if (frostDaysFileName != "")
    {
        //declare the key value
        myString=QString("Annual mean number of frost days");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }

    if (totalSolarRadFileName != "")
    {
        qDebug( "Solar incident radiation : " +    totalSolarRadFileName.toLocal8Bit() );
        //declare the key value
        myString=QString("Annual mean total incident solar radiation");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }

    if (maxTempFileGroup && minTempFileName  != "" && maxTempFileName != "" )
    {
        //declare the key value
        myString=QString("Annual temperature range");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }

    if (maxTempFileName != "")
    {
        //declare the key value
        myString=QString("Highest temperature in warmest month");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }

    if (minTempFileName != "")
    {
        //declare the key value
        myString=QString("Lowest temperature in coolest month");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if (meanPrecipFileName != "")
    {
        //declare the key value
        myString=QString("Mean daily precipitation");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if (meanPrecipFileName != "" && meanTempFileName != "" )
    {
        //declare the key value
        myString=QString("Mean daily precipitation in coolest month");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if (meanPrecipFileName != "" && meanTempFileName != "" )
    {
        //declare the key value
        myString=QString("Mean daily precipitation in coolest quarter");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if (meanPrecipFileName != "" )
    {
        //declare the key value
        myString=QString("Mean daily precipitation in driest month");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if (meanPrecipFileName != "" )
    {
        //declare the key value
        myString=QString("Mean daily precipitation in driest quarter");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if (meanPrecipFileName != ""  &&  meanTempFileName != ""  )
    {
        //declare the key value
        myString=QString("Mean daily precipitation in warmest month");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if (meanPrecipFileName != ""  && meanTempFileName != ""  )
    {
        //declare the key value
        myString=QString("Mean daily precipitation in warmest quarter");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if (meanPrecipFileName != ""  )
    {
        //declare the key value
        myString=QString("Mean daily precipitation in wettest month");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if (meanPrecipFileName != ""  )
    {
        //declare the key value
        myString=QString("Mean daily precipitation in wettest quarter");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if (diurnalTempFileName != "" && meanTempFileName !="")
    {
        //declare the key value
        myString=QString("Mean diurnal temperature range in coolest month");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if ( diurnalTempFileName != "" && meanTempFileName !="")
    {
        //declare the key value
        myString=QString("Mean diurnal temperature range in warmest month");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if (  meanPrecipFileName != ""  &&  frostDaysFileName != "")
    {
        //declare the key value
        myString=QString("Mean precipitation in frost free months");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if ( meanTempFileName !="")
    {
        //declare the key value
        myString=QString("Mean temperature");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if (meanTempFileName !="")
    {
        //declare the key value
        myString=QString("Mean temperature in coolest month");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if ( meanTempFileName !="")
    {
        //declare the key value
        myString=QString("Mean temperature in coolest quarter");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if ( meanTempFileName !="" &&  frostDaysFileName != "")
    {
        //declare the key value
        myString=QString("Mean temperature in frost free months");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if ( meanTempFileName !="")
    {
        //declare the key value
        myString=QString("Mean temperature in warmest month");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if ( meanTempFileName !="")
    {
        //declare the key value
        myString=QString("Mean temperature in warmest quarter");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if ( windSpeedFileName != "")
    {
        //declare the key value
        myString=QString("Mean wind speed");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if ( minTempFileName !="")
    {
        //declare the key value
        myString=QString("Number of months with minimum temperature above freezing");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if ( totalSolarRadFileName != "" && meanTempFileName !="")
    {
        //declare the key value
        myString=QString("Radiation in coolest month");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if ( totalSolarRadFileName != ""  && meanTempFileName !="")
    {
        //declare the key value
        myString=QString("Radiation in coolest quarter");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if ( totalSolarRadFileName != "" && meanPrecipFileName != "")
    {
        //declare the key value
        myString=QString("Radiation in driest month");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if (totalSolarRadFileName != "" && meanPrecipFileName != "")
    {
        //declare the key value
        myString=QString("Radiation in driest quarter");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if ( totalSolarRadFileName != ""  && meanTempFileName != "")
    {
        //declare the key value
        myString=QString("Radiation in warmest month");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if ( totalSolarRadFileName != ""  && meanTempFileName != "")
    {
        //declare the key value
        myString=QString("Radiation in warmest quarter");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if ( totalSolarRadFileName != "" && meanPrecipFileName != "")
    {
        //declare the key value
        myString=QString("Radiation in wettest month");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if ( totalSolarRadFileName != ""  && meanPrecipFileName != "")
    {
        //declare the key value
        myString=QString("Radiation in wettest quarter");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if ( meanPrecipFileName != "")
    {
        //declare the key value
        myString=QString("Standard deviation of mean precipitation");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }
    if (meanTempFileName != "")
    {
        //declare the key value
        myString=QString("Standard deviation of mean temperature");
        //add it to the associative array
        availableCalculationsMap[myString]=myBool;
    }

    //presume all went ok - need to add better error checking later
    return true;
} //end of makeAvailableCalculationsMap

/** Return the map of available calculations */
QMap <QString, bool > ClimateDataProcessorController::getAvailableCalculationsMap()
{

    return  availableCalculationsMap;

}

bool ClimateDataProcessorController::addUserCalculation(QString theCalculationName)
{
    QMap<QString, bool>::iterator myMapIterator = availableCalculationsMap.begin();
    myMapIterator = availableCalculationsMap.find(theCalculationName);

    if (myMapIterator != availableCalculationsMap.end())
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





/** Read property of bool filesInSeriesFlag. */
const bool ClimateDataProcessorController::getFilesInSeriesFlag()
{
    return filesInSeriesFlag;
}
/** Write property of bool filesInSeriesFlag. */
void ClimateDataProcessorController::setFilesInSeriesFlag( const bool theFlag)
{
    filesInSeriesFlag = theFlag;
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
  int myNumberOfVariablesInt = 0;
  QMap<QString, bool>::const_iterator myIter;
  for (myIter=availableCalculationsMap.begin(); myIter != availableCalculationsMap.end(); myIter++)
  {
    if (myIter.value()) //true
    {
      myNumberOfVariablesInt++;
    }
  }
  //
  //work out how many cells need to be processed for each calculations
  //
  int myNumberOfCells =0;
  int myXDimInt= 0;
  int myYDimInt=0;
  if (meanTempFileGroup)
  {
    myNumberOfCells = meanTempFileGroup->elementCount();
    myXDimInt=meanTempFileGroup->xDim();
    myYDimInt=meanTempFileGroup->yDim();
  }
  else if (minTempFileGroup)
  {
    myNumberOfCells = minTempFileGroup->elementCount();
    myXDimInt=minTempFileGroup->xDim();
    myYDimInt=minTempFileGroup->yDim();
  }
  else if (maxTempFileGroup)
  {
    myNumberOfCells = maxTempFileGroup->elementCount();
    myXDimInt=maxTempFileGroup->xDim();
    myYDimInt=maxTempFileGroup->yDim();
  }
  else if (diurnalTempFileGroup)
  {
    myNumberOfCells = diurnalTempFileGroup->elementCount();
    myXDimInt=diurnalTempFileGroup->xDim();
    myYDimInt=diurnalTempFileGroup->yDim();
  }
  else if (meanPrecipFileGroup)
  {
    myNumberOfCells = meanPrecipFileGroup->elementCount();
    myXDimInt=meanPrecipFileGroup->xDim();
    myYDimInt=meanPrecipFileGroup->yDim();
  }
  else if (frostDaysFileGroup)
  {
    myNumberOfCells = frostDaysFileGroup->elementCount();
    myXDimInt=frostDaysFileGroup->xDim();
    myYDimInt=frostDaysFileGroup->yDim();
  }
  else if (totalSolarRadFileGroup)
  {
    myNumberOfCells = totalSolarRadFileGroup->elementCount();
    myXDimInt=totalSolarRadFileGroup->xDim();
    myYDimInt=totalSolarRadFileGroup->yDim();
  }
  else if (windSpeedFileGroup)
  {
    myNumberOfCells = windSpeedFileGroup->elementCount();
    myXDimInt=windSpeedFileGroup->xDim();
    myYDimInt=windSpeedFileGroup->yDim();
  }
  //check nothing fishy is going on
  if (myNumberOfCells ==  0)
  {
    return false;
  }


  //send singals so progress monitors can set themselves up
  emit numberOfCellsToCalc(myNumberOfCells);
  emit numberOfVariablesToCalc(myNumberOfVariablesInt);


  //create a filewriter map for storing the OUTPUTS of each selected user calculation
  //this is not very element - I put the filewriter pointer into a struct and then  put the struct in to the
  //map, because I cant seem to be able to put the pointer directly into the map itself :-(
  QMap<QString, FileWriterStruct> myFileWriterMap;

  for (myIter=availableCalculationsMap.begin(); myIter != availableCalculationsMap.end(); myIter++)
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
      myFileName =  outputFilePath + myFileName + ".asc";
      FileWriter * myFileWriter = new FileWriter(myFileName,outputFileType);

      //Use externally defined header if its been set
      if (!outputHeader.isEmpty())
      {
        myFileWriter->writeString(outputHeader);
      }
      //Otherwise calculate one dynamically
      else
      {
        // Use the matrix dimensions to create the ascii file
        // Warning: this assumes a GLOBAL dataset
        // Warning: this screws up cellsizes that are not square
        // Warning: this only works for integers at present
        QString myHeader=
            QString ("ncols         ") +
            QString::number (myXDimInt) + 
            QString ("\n")+
            QString ("nrows         ") + 
            QString::number (myYDimInt) + 
            QString ("\n")+
            QString ("xllcorner     -180\n")+
            QString ("yllcorner     -90\n")+
            QString ("cellsize      ") + 
            QString::number (360/static_cast<float>(myXDimInt)) +
            QString ("\n")+
            QString ("nodata_value  -9999.5\n");
        myFileWriter->writeString(myHeader);
        // Formerly this was fixed to the following
        //QString myHeader=
        //QString ("ncols         720\n")+
        //QString ("nrows         360\n")+
        //QString ("xllcorner     -180\n")+
        //QString ("yllcorner     -90\n")+
        //QString ("cellsize      0.5\n")+
        //QString ("nodata_value  -9999\n");                    myFileWriter->writeString(myHeader);

      }

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


  if (meanTempFileGroup && meanTempFileName !="" &&
          availableCalculationsMap["Mean temperature"])
  {
    emit variableStart("Mean temperature");
    qDebug( "ClimateDataProcessorController::run - Mean temperature requested" );
    //move to start of the current data matrix
    meanTempFileGroup->rewind();

    //get the struct containing the filewriter pointer and full file name from the writer map
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      ////emit cellDone( myFloat );
    }
    qDebug( " ++++++ Emitting variableDone signal! " );
    emit variableDone(myFileWriterStruct.fullFileName);

  }
  if (diurnalTempFileGroup && diurnalTempFileName != "" &&
          availableCalculationsMap["Annual mean diurnal temperature range"])
  {
    emit variableStart("Annual mean diurnal temperature range");
    qDebug( "ClimateDataProcessorController::run Annual mean diurnal temperature range requested" );
    //move to start of the current data matrix
    diurnalTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Annual mean diurnal temperature range"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!diurnalTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = diurnalTempFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      ////emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }

  if (frostDaysFileGroup && frostDaysFileName != "" &&
          availableCalculationsMap["Annual mean number of frost days"])
  {
    emit variableStart("Annual mean number of frost days");
    qDebug( "ClimateDataProcessorController::run Annual mean number of frost days requested" );
    //move to start of the current data matrix
    frostDaysFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Annual mean number of frost days"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!frostDaysFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = frostDaysFileGroup->getElementVector();

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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      ////emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }

  if (totalSolarRadFileGroup && totalSolarRadFileName != "" &&
          availableCalculationsMap["Annual mean total incident solar radiation"])
  {
    emit variableStart("Annual mean total incident solar radiation");
    qDebug( "ClimateDataProcessorController::run Annual mean total incident solar radiation requested" );
    //move to start of the current data matrix
    totalSolarRadFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Annual mean total incident solar radiation"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!totalSolarRadFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = totalSolarRadFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }

  if ( minTempFileGroup
          && maxTempFileGroup
          && minTempFileName != ""
          && maxTempFileName != ""
          && availableCalculationsMap["Annual temperature range"])
  {

    emit variableStart("Annual temperature range");
    qDebug( "ClimateDataProcessorController::run - Annual temperature range requested" );
    //move to start of the current data matrix
    minTempFileGroup->rewind();
    maxTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Annual temperature range"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!minTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector, myVector2;
      //get the next element from the file group
      myVector = minTempFileGroup->getElementVector();
      myVector2 = maxTempFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }

  if (maxTempFileGroup
          && maxTempFileName != ""
          && availableCalculationsMap["Highest temperature in warmest month"])
  {
    emit variableStart("Highest temperature in warmest month");
    qDebug( "ClimateDataProcessorController::run - Highest temperature in warmest month requested" );
    maxTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Highest temperature in warmest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!maxTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = maxTempFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }

  if (minTempFileGroup
          && minTempFileName != ""
          && availableCalculationsMap["Lowest temperature in coolest month"])
  {
    emit variableStart("Lowest temperature in coolest month");
    qDebug( "ClimateDataProcessorController::run - Lowest temperature in coolest month requested" );
    minTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Lowest temperature in coolest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!minTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = minTempFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);

  }
  if (meanPrecipFileGroup
          && meanPrecipFileName != ""
          && availableCalculationsMap["Mean daily precipitation"])
  {
    emit variableStart("Mean daily precipitation");
    qDebug( "ClimateDataProcessorController::run Mean daily precipitation" );
    meanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean daily precipitation"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!meanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = meanPrecipFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if ( meanPrecipFileGroup &&  minTempFileGroup
          && meanPrecipFileName != "" && meanTempFileName != ""
          && availableCalculationsMap["Mean daily precipitation in coolest month"])
  {
    emit variableStart("Mean daily precipitation in coolest month");
    qDebug( "ClimateDataProcessorController::run Mean daily precipitation in coolest month" );
    meanPrecipFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean daily precipitation in coolest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!meanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = meanPrecipFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (meanPrecipFileGroup &&  minTempFileGroup && meanPrecipFileName != ""
          && meanTempFileName != ""
          && availableCalculationsMap["Mean daily precipitation in coolest quarter"])
  {
    emit variableStart("Mean daily precipitation in coolest quarter");
    qDebug( "ClimateDataProcessorController::run Mean daily precipitation in coolest month" );
    meanPrecipFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean daily precipitation in coolest quarter"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!meanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = meanPrecipFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (meanPrecipFileGroup &&  meanPrecipFileName != ""  &&
          availableCalculationsMap["Mean daily precipitation in driest month"])
  {
    emit variableStart("Mean daily precipitation in driest month");
    qDebug( "ClimateDataProcessorController::run Mean daily precipitation in driest month" );
    meanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean daily precipitation in driest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!meanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = meanPrecipFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (meanPrecipFileGroup &&  meanPrecipFileName != ""  &&
          availableCalculationsMap["Mean daily precipitation in driest quarter"])
  {
    emit variableStart("Mean daily precipitation in driest quarter");
    qDebug( "ClimateDataProcessorController::run Mean daily precipitation in driest quarter" );
    meanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean daily precipitation in driest quarter"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!meanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = meanPrecipFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (meanPrecipFileGroup &&  meanPrecipFileName != ""
          && meanTempFileGroup
          && meanTempFileName != ""
          && availableCalculationsMap["Mean daily precipitation in warmest month"])
  {
    emit variableStart("Mean daily precipitation in warmest month");
    qDebug( "ClimateDataProcessorController::run Mean daily precipitation in warmest month" );
    //move to the start of data blocks
    meanPrecipFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean daily precipitation in warmest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!meanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = meanPrecipFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (meanPrecipFileGroup &&  meanPrecipFileName != ""
          && meanTempFileGroup &&  meanTempFileName != ""
          &&availableCalculationsMap["Mean daily precipitation in warmest quarter"])
  {
    emit variableStart("Mean daily precipitation in warmest quarter");
    qDebug( "ClimateDataProcessorController::run Mean daily precipitation in warmest quarter" );
    meanPrecipFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean daily precipitation in warmest quarter"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!meanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = meanPrecipFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (meanPrecipFileGroup &&  meanPrecipFileName != ""   &&
          availableCalculationsMap["Mean daily precipitation in wettest month"])
  {
    emit variableStart("Mean daily precipitation in wettest month");
    qDebug( "ClimateDataProcessorController::run Mean daily precipitation in wettest month" );
    meanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean daily precipitation in wettest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!meanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = meanPrecipFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (meanPrecipFileGroup &&  meanPrecipFileName != ""   &&
          availableCalculationsMap["Mean daily precipitation in wettest quarter"])
  {
    emit variableStart("Mean daily precipitation in wettest quarter");
    qDebug( "ClimateDataProcessorController::run Mean daily precipitation in wettest quarter" );
    meanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean daily precipitation in wettest quarter"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!meanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float > myVector;
      //get the next element from the file group
      myVector = meanPrecipFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (diurnalTempFileGroup && diurnalTempFileName != ""
          && meanTempFileGroup
          && meanTempFileName !=""
          && availableCalculationsMap["Mean diurnal temperature range in coolest month"])
  {
    emit variableStart("Mean diurnal temperature range in coolest month");
    qDebug( "ClimateDataProcessorController::run Mean diurnal temperature range in coolest month" );
    diurnalTempFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean diurnal temperature range in coolest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!diurnalTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = diurnalTempFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (diurnalTempFileGroup && diurnalTempFileName != "" && meanTempFileGroup
          && meanTempFileName !=""
          && availableCalculationsMap["Mean diurnal temperature range in warmest month"])
  {
    emit variableStart("Mean diurnal temperature range in warmest month");
    qDebug( "ClimateDataProcessorController::run Mean diurnal temperature range in warmest month" );
    diurnalTempFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean diurnal temperature range in warmest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!diurnalTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = diurnalTempFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (meanPrecipFileGroup &&  meanPrecipFileName != ""  && frostDaysFileGroup
          && frostDaysFileName != ""
          && availableCalculationsMap["Mean precipitation in frost free months"])
  {
    emit variableStart("Mean precipitation in frost free months");
    qDebug( "ClimateDataProcessorController::run Mean precipitation in frost free months" );
    meanPrecipFileGroup->rewind();
    frostDaysFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean precipitation in frost free months"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!meanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = meanPrecipFileGroup->getElementVector();
      myVector2 = frostDaysFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }

  if (meanTempFileGroup && meanTempFileName !="" &&
          availableCalculationsMap["Mean temperature in coolest month"])
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (meanTempFileGroup && meanTempFileName !="" &&
          availableCalculationsMap["Mean temperature in coolest quarter"])
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (meanTempFileGroup && meanTempFileName !="" && frostDaysFileGroup
          && frostDaysFileName != ""
          && availableCalculationsMap["Mean temperature in frost free months"])
  {
    emit variableStart("Mean temperature in frost free months");
    qDebug( "ClimateDataProcessorController::run Mean temperature in frost free months" );
    meanTempFileGroup->rewind();
    frostDaysFileGroup->rewind();
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
      myVector2 = frostDaysFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (meanTempFileGroup && meanTempFileName !="" &&
          availableCalculationsMap["Mean temperature in warmest month"])
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (meanTempFileGroup && meanTempFileName !="" &&
          availableCalculationsMap["Mean temperature in warmest quarter"])
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (windSpeedFileGroup && windSpeedFileName != "" &&
          availableCalculationsMap["Mean wind speed"])
  {
    emit variableStart("Mean wind speed");
    qDebug( "ClimateDataProcessorController::run Mean wind speed" );
    windSpeedFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Mean wind speed"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!windSpeedFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = windSpeedFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (minTempFileGroup && minTempFileName !="" &&
          availableCalculationsMap["Number of months with minimum temperature above freezing"])
  {
    emit variableStart("Number of months with minimum temperature above freezing");
    qDebug( "ClimateDataProcessorController::run Number of months with minimum temperature above freezing" );
    minTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Number of months with minimum temperature above freezing"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!minTempFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = minTempFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (totalSolarRadFileGroup && totalSolarRadFileName != ""
          && meanTempFileGroup && meanTempFileName !=""
          && availableCalculationsMap["Radiation in coolest month"])
  {
    emit variableStart("Radiation in coolest quarter");
    qDebug( "ClimateDataProcessorController::run Radiation in coolest month" );
    totalSolarRadFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Radiation in coolest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!totalSolarRadFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = totalSolarRadFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (totalSolarRadFileGroup && totalSolarRadFileName != ""
          && meanTempFileGroup && meanTempFileName !=""
          && availableCalculationsMap["Radiation in coolest quarter"])
  {
    emit variableStart("Radiation in coolest quarter");
    qDebug( "ClimateDataProcessorController::run Radiation in coolest quarter" );
    totalSolarRadFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Radiation in coolest quarter"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!totalSolarRadFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = totalSolarRadFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (totalSolarRadFileGroup && totalSolarRadFileName != ""
          && meanTempFileGroup && meanTempFileName != ""
          && availableCalculationsMap["Radiation in warmest month"])
  {
    emit variableStart("Radiation in warmest month");
    qDebug( "ClimateDataProcessorController::run Radiation in warmest month" );
    totalSolarRadFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Radiation in warmest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!totalSolarRadFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = totalSolarRadFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (totalSolarRadFileGroup && totalSolarRadFileName != ""
          && meanTempFileGroup && meanTempFileName != ""
          && availableCalculationsMap["Radiation in warmest quarter"])
  {
    emit variableStart("Radiation in warmest quarter");
    qDebug( "ClimateDataProcessorController::run Radiation in warmest quarter" );
    totalSolarRadFileGroup->rewind();
    meanTempFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Radiation in warmest quarter"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!totalSolarRadFileGroup->isAtMatrixEnd())
    {

      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = totalSolarRadFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (totalSolarRadFileGroup && totalSolarRadFileName != ""
          && meanPrecipFileGroup && meanPrecipFileName != ""
          && availableCalculationsMap["Radiation in driest month"])
  {
    emit variableStart("Radiation in driest month");
    qDebug( "ClimateDataProcessorController::run Radiation in driest month" );
    totalSolarRadFileGroup->rewind();
    meanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Radiation in driest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!totalSolarRadFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = totalSolarRadFileGroup->getElementVector();
      myVector2 = meanPrecipFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (totalSolarRadFileGroup && totalSolarRadFileName != ""
          && meanPrecipFileGroup && meanPrecipFileName != ""
          && availableCalculationsMap["Radiation in driest quarter"])
  {
    emit variableStart("Radiation in driest quarter");
    qDebug( "ClimateDataProcessorController::run Radiation in driest quarter" );
    totalSolarRadFileGroup->rewind();
    meanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Radiation in driest quarter"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!totalSolarRadFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = totalSolarRadFileGroup->getElementVector();
      myVector2 = meanPrecipFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }

  if (totalSolarRadFileGroup && totalSolarRadFileName != ""
          && meanPrecipFileGroup && meanPrecipFileName != ""
          && availableCalculationsMap["Radiation in wettest month"])
  {
    emit variableStart("Radiation in wettest month");
    qDebug( "ClimateDataProcessorController::run Radiation in wettest month" );
    totalSolarRadFileGroup->rewind();
    meanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Radiation in wettest month"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!totalSolarRadFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = totalSolarRadFileGroup->getElementVector();
      myVector2 = meanPrecipFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (totalSolarRadFileGroup && totalSolarRadFileName != ""
          && meanPrecipFileGroup && meanPrecipFileName != ""
          && availableCalculationsMap["Radiation in wettest quarter"])
  {
    emit variableStart("Radiation in wettest quarter");
    qDebug( "ClimateDataProcessorController::run Radiation in wettest quarter" );
    totalSolarRadFileGroup->rewind();
    meanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Radiation in wettest quarter"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!totalSolarRadFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector,myVector2;
      //get the next element from the file group
      myVector = totalSolarRadFileGroup->getElementVector();
      myVector2 = meanPrecipFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (meanPrecipFileGroup && meanPrecipFileName != "" &&
          availableCalculationsMap["Standard deviation of mean precipitation"])
  {
    emit variableStart("Standard deviation of mean precipitation");
    qDebug( "ClimateDataProcessorController::run Standard deviation of mean precipitation" );
    meanPrecipFileGroup->rewind();
    //get the struct containing the filewriter pointer and full file name from the writer map
    FileWriterStruct myFileWriterStruct = myFileWriterMap["Standard deviation of mean precipitation"];
    //get the filewriter from out of the struct
    FileWriter *myFileWriter = myFileWriterStruct.fileWriter;
    int myXCountInt=0;
    while (!meanPrecipFileGroup->isAtMatrixEnd())
    {
      QVector<float> myVector;
      //get the next element from the file group
      myVector = meanPrecipFileGroup->getElementVector();
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
    emit variableDone(myFileWriterStruct.fullFileName);
  }
  if (meanTempFileGroup && meanTempFileName != ""
          && availableCalculationsMap["Standard deviation of mean temperature"])
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
      if (myXCountInt%myXDimInt==0)
      {
        myFileWriter->writeString("\n");
      }
      //emit cellDone( myFloat );
    }
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
QString ClimateDataProcessorController::getDescription()
{
    //must have an include for sstream.h!

    QString myString, myNumber;
    myString += "\n Climate Data Processor Description \n";
    myString += " ---------------------------------- \n";


    myNumber = QString::number(getInputFileType());
    myString += QString("Input File Type Enum : ") + myNumber+ QString("\n");

    myNumber = QString::number(getOutputFileType());
    myString += QString("Output File Type Enum : ") + myNumber + QString("\n");


    //these properties are just plain strings and dont need conversion
    myString += QString("Mean Temp FileName : ") + getMeanTempFileName() + QString("\n");
    myString += QString("Max Temp FileName : ") + getMaxTempFileName() + QString("\n");
    myString += QString("Min Temp FileName : ") + getMinTempFileName() + QString("\n");
    myString += QString("Diurnal Temp FileName : ") + getDiurnalTempFileName() + QString("\n");
    myString += QString("Mean Precipitation FileName : ") + getMeanPrecipFileName() + QString("\n");
    myString += QString("Frost Days FileName : ") + getFrostDaysFileName() + QString("\n");
    myString += QString("Total Solar Radiation FileName : ") + getTotalSolarRadFileName() + QString("\n");
    myString += QString("Wind Speed FileName : ") + getWindSpeedFileName() + QString("\n");
    if (filesInSeriesFlag)
    {
        myString += QString("Datafiles are a series of numbered files for each month \n");
    }
    else
    {
        myString += QString("Datafiles contain all monthly data in a single file \n");
    }

    //List the calculations in  availableCalculationsMap  using an iterator
    myString += QString("Listing items in availableCalculationsMap \n");
    myString += QString("Boolean value suffix indicates whether the user want to use this calculation \n");
    myString += QString("---------------------------------------------------------------------------------------------------- \n");
    QMap<QString, bool>::const_iterator myIter;
    for (myIter=availableCalculationsMap.begin(); myIter != availableCalculationsMap.end(); myIter++)
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




