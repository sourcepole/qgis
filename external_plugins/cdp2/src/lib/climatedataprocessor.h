/***************************************************************************
                          dataprocessor.h  -  description
                             -------------------
    begin                : Wed Jan 8 2003
    copyright            : (C) 2003 by Tim Sutton
    email                : tim@linfiniti.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CLIMATEDATAPROCESSOR_H
#define CLIMATEDATAPROCESSOR_H

#include <QVector>

/** \ingroup library
  * \brief This class contains various functions that can be used to post process raw climate data
  * sets such as obtained from the IPCC. 
  *
  *Methods that take ArrayLengths as parameters should be passed as the number of elements,
  *not the vector size. e.g. an vector of 12 months is int[11] but should be passed as 12.
  *@author Tim Sutton
  */

class CDP_LIB_EXPORT ClimateDataProcessor
{
public:
    /** Default constructor */
    ClimateDataProcessor(float mNoData=-9999.0);
    /** Destructor */
    ~ClimateDataProcessor();

    /** This method calculates the mean value over the quarter with the lowest values
    * (i.e. the three consecutive months with the minimum combined total). */
    float meanOverLowestQ(QVector <float> theClimateVector);

    /** This method calculates the mean value over the quarter with the highest values
    * (i.e. the three consecutive months with the maximum combined total). */
    float meanOverHighestQ ( QVector <float> theClimateVector);

    /** This method returns the month that starts the quarter with the highest
    average values.*/
    int firstMonthOfHighestQ (QVector <float> theClimateVector);

    /** This method returns the month that starts the quarter with the lowest
    average values. For example, if 12 months were :

    1   2   3   4   5   6   7   8   9   10  11  12
    -----------------------------------------------
    21 18  19  15   12  5   6   8   12  15  16  20

    Then the return from this method would be 6 because
    5,6 and 8 combined form the lowest quarter. */
    int firstMonthOfLowestQ (QVector <float> theClimateVector);

    /** This method will return the mean over three months in
    theClimateVector, starting at theStartMonth. */
    float meanOverQuarter (QVector <float> theClimateVector, int theStartMonth);

    /** Given an vector, this method will return the value of the smallest
    element in the vector. */
    float lowestValue (QVector <float> theClimateVector);

    /** Given an vector, this function will return the value of the largest element
    in the vector. */
    float highestValue (QVector <float> theClimateVector);

    /** Given two arrays (e.g. min temp and max temp) range will determine
    * the smallest and largest values that occur and return the difference.
    * The function is indescriminate as to whether the values are in the same
    * month or not.
    * @see greatestMonthlyRange
    */
    float greatestTotalRange (QVector <float> theClimateVector1,
                              QVector <float> theClimateVector2);

    /** Given two arrays (e.g. min temp and max temp) range will determine
    * the smallest and largest values that occur and return the difference.
    * The value of the max-min difference for a given month that is the
    * greatest is returned.
    * @see greatestTotalRange */
    float greatestMonthlyRange (QVector <float> theClimateVector,
                                QVector <float> theClimateVector2);

    /** This function will return the standard deviation of the climate vector. */
    float stddevOverYear (QVector <float> theClimateVector);

    /** A new vector the same length as original will be returned when each element
     * of the input vector that is greater or equal to threshold will be true. Values
     * below threshold will be assigned false.
     * @param &QVector <float> A vector of float values
     * @param float the desired threshold
     * @return bool True on success otherwise something went wrong.
     */
    bool threshold(QVector <float> &theClimateVector, float theThreshold);
    /** Overloaded version of the above function.
     * If the input value  is greater or equal to threshold will be true. Values
     * below threshold will be assigned false.
     * @param float A vector of float values
     * @param float the desired threshold
     * @return bool True on success otherwise something went wrong.
     */
    float threshold(float theFloat, float theThreshold);
    /** Determine the sum of a vector of values
    *
    * @param QVector <float> values to be summed
    * @return float sum of values provided in input.
    */
    float sum(QVector <float> theClimateVector);

    /** This function will return the sum of theClimateVector divided by the
    * number of elements in theClimateVector. */
    float mean (QVector <float> theClimateVector);

    /** This function will return the value of the element in theClimateVector
    * that corresponds to theMonth. */
    float valueGivenMonth (QVector <float> theClimateVector, int theMonth);

    /** This function will return an integer between 1 and 12 corresponding to
    * the month with the highest value. */
    int monthWithHighestValue (QVector <float> theClimateVector);

    /** This function will return an integer between 1 and 12 corresponding to
    * the month with the lowest value. */
    int monthWithLowestValue(QVector <float> theClimateVector);

    /** This function will return an integer in the range 0-12 representing
    * the number of months in theClimateVector where the value for that
    * month is above zero. Typically used to calculate how many months
    * in the year there are where the average temp is above freezing. */
    int numberOfMonthsAboveZero (QVector <float> theClimateVector);

    /** This value will return the mean value of months in theClimateVector
     *  where the corresponding months in theFrostArray have no frost free days. */
    float meanValueOverFrostFreeMonths (QVector <float> theFrostVector,
                                        QVector <float> theClimateVector);


private:
    float mNoData;

};



#endif
