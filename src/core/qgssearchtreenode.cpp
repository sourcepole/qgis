/***************************************************************************
                          qgssearchtreenode.cpp
                  Implementation for evaluating parsed tree
                          --------------------
    begin                : 2005-07-26
    copyright            : (C) 2005 by Martin Dobias
    email                : won.der at centrum.sk
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#include "qgslogger.h"
#include "qgsdistancearea.h"
#include "qgsfield.h"
#include "qgsgeometry.h"
#include "qgssearchtreenode.h"
#include <QRegExp>
#include <QObject>
#include <QSet>
#include <QSettings>
#include <iostream>

#ifndef Q_OS_MACX
#include <cmath>
#else
#include <math.h>
#endif



#define EVAL_STR(x) (x.length() ? x : "(empty)")

QgsSearchTreeNode::QgsSearchTreeNode( double number )
{
  mType   = tNumber;
  mNumber = number;
  mLeft   = NULL;
  mRight  = NULL;

  init();
}


QgsSearchTreeNode::QgsSearchTreeNode( Operator op, QgsSearchTreeNode* left,
                                      QgsSearchTreeNode* right )
{
  mType  = tOperator;
  mOp    = op;
  mLeft  = left;
  mRight = right;

  init();
}


QgsSearchTreeNode::QgsSearchTreeNode( QString text, bool isColumnRef )
{
  mLeft  = NULL;
  mRight = NULL;

  if ( isColumnRef )
  {
    mType = tColumnRef;
    mText = text;
  }
  else
  {
    mType = tString;
    mText = text;
    stripText();
  }

  init();
}


QgsSearchTreeNode::QgsSearchTreeNode( const QgsSearchTreeNode& node )
{
  mType = node.mType;
  mOp = node.mOp;
  mNumber = node.mNumber;
  mText = node.mText;

  // recursively copy children
  if ( node.mLeft )
    mLeft =  new QgsSearchTreeNode( *node.mLeft );
  else
    mLeft = NULL;

  if ( node.mRight )
    mRight = new QgsSearchTreeNode( *node.mRight );
  else
    mRight = NULL;

  init();
}


QgsSearchTreeNode::~QgsSearchTreeNode()
{
  // delete children

  if ( mLeft )
    delete mLeft;

  if ( mRight )
    delete mRight;

  delete mCalc;
}


void QgsSearchTreeNode::init()
{
  if ( mType == tOperator && ( mOp == opLENGTH || mOp == opAREA ) )
  {
    //initialize QgsDistanceArea
    mCalc = new QgsDistanceArea;
    mCalc->setProjectionsEnabled( false );
    QSettings settings;
    QString ellipsoid = settings.value( "/qgis/measure/ellipsoid", "WGS84" ).toString();
    mCalc->setEllipsoid( ellipsoid );
  }
  else
  {
    mCalc = NULL;
  }
}

void QgsSearchTreeNode::stripText()
{
  // strip single quotes on start,end
  mText = mText.mid( 1, mText.length() - 2 );

  // make single "single quotes" from double "single quotes"
  mText.replace( QRegExp( "''" ), "'" );

  // strip \n \' etc.
  int index = 0;
  while (( index = mText.indexOf( '\\', index ) ) != -1 )
  {
    mText.remove( index, 1 ); // delete backslash
    QChar chr;
    switch ( mText[index].toLatin1() ) // evaluate backslashed character
    {
      case 'n':  chr = '\n'; break;
      case 't':  chr = '\t'; break;
      case '\\': chr = '\\'; break;
      case '\'': chr = '\''; break;
      default: chr = '?'; break;
    }
    mText[index++] = chr; // set new character and push index +1
  }

}

QString QgsSearchTreeNode::makeSearchString()
{
  QString str;
  if ( mType == tOperator )
  {
    str += "(";
    if ( mOp != opNOT )
    {
      if ( mLeft )
      {
        str += mLeft->makeSearchString();
      }
      switch ( mOp )
      {
        case opAND: str += " AND "; break;
        case opOR: str += " OR "; break;

        case opPLUS:  str += "+"; break;
        case opMINUS: str += "-"; break;
        case opMUL:   str += "*"; break;
        case opDIV:   str += "/"; break;

        case opEQ: str += " = "; break;
        case opNE: str += " != "; break;
        case opGT: str += " > "; break;
        case opLT: str += " < "; break;
        case opGE: str += " >= "; break;
        case opLE: str += " <= "; break;

        case opRegexp: str += " ~ "; break;
        case opLike: str += " LIKE "; break;

        default: str += " ? ";
      }

      if ( mRight )
      {
        str += mRight->makeSearchString();
      }
    }
    else
    {
      str += "NOT ";
      str += mLeft->makeSearchString();
    }
    str += ")";
  }
  else if ( mType == tNumber )
  {
    str += QString::number( mNumber );
  }
  else if ( mType == tString || mType == tColumnRef )
  {
    str += mText;
  }
  else // unknown type
  {
    str += "unknown_node_type:";
    str += QString::number( mType );
  }

  return str;
}

QStringList QgsSearchTreeNode::referencedColumns()
{
  QList<QgsSearchTreeNode*> columnNodeList = columnRefNodes();
  QSet<QString> columnStringSet;

  QList<QgsSearchTreeNode*>::const_iterator nodeIt = columnNodeList.constBegin();
  for ( ; nodeIt != columnNodeList.constEnd(); ++nodeIt )
  {
    columnStringSet.insert(( *nodeIt )->columnRef() );
  }
  return columnStringSet.toList();
}

QList<QgsSearchTreeNode*> QgsSearchTreeNode::columnRefNodes()
{
  QList<QgsSearchTreeNode*> nodeList;
  if ( mType == tOperator )
  {
    if ( mLeft )
    {
      nodeList += mLeft->columnRefNodes();
    }
    if ( mRight )
    {
      nodeList += mRight->columnRefNodes();
    }
  }
  else if ( mType == tColumnRef )
  {
    nodeList.push_back( this );
  }
  return nodeList;
}

bool QgsSearchTreeNode::needsGeometry()
{
  if ( mType == tOperator )
  {
    if ( mOp == opLENGTH || mOp == opAREA )
      return true;

    if ( mLeft && mLeft->needsGeometry() )
      return true;
    if ( mRight && mRight->needsGeometry() )
      return true;
    return false;
  }
  else
  {
    return false;
  }
}


bool QgsSearchTreeNode::checkAgainst( const QgsFieldMap& fields, const QgsAttributeMap& attributes, QgsGeometry* geom )
{
  QgsDebugMsgLevel( "checkAgainst: " + makeSearchString(), 2 );

  mError = "";

  // this error should be caught when checking syntax, but for sure...
  if ( mType != tOperator )
  {
    mError = "Expected operator, got scalar value!";
    return false;
  }

  QgsSearchTreeValue value1, value2;
  int res;

  switch ( mOp )
  {
    case opNOT:
      return !mLeft->checkAgainst( fields, attributes, geom );

    case opAND:
      if ( !mLeft->checkAgainst( fields, attributes, geom ) )
        return false;
      return mRight->checkAgainst( fields, attributes, geom );

    case opOR:
      if ( mLeft->checkAgainst( fields, attributes, geom ) )
        return true;
      return mRight->checkAgainst( fields, attributes, geom );

    case opISNULL:
    case opISNOTNULL:
      if ( !getValue( value1, mLeft, fields, attributes, geom ) )
        return false;

      if ( mOp == opISNULL )
      {
        return value1.isNull();
      }
      else if ( mOp == opISNOTNULL )
      {
        return !value1.isNull();
      }

    case opEQ:
    case opNE:
    case opGT:
    case opLT:
    case opGE:
    case opLE:

      if ( !getValue( value1, mLeft, fields, attributes, geom ) || !getValue( value2, mRight, fields, attributes, geom ) )
        return false;

      if ( value1.isNull() || value2.isNull() )
      {
        // NULL values never match
        return false;
      }

      res = QgsSearchTreeValue::compare( value1, value2 );

      switch ( mOp )
      {
        case opEQ: return ( res == 0 );
        case opNE: return ( res != 0 );
        case opGT: return ( res >  0 );
        case opLT: return ( res <  0 );
        case opGE: return ( res >= 0 );
        case opLE: return ( res <= 0 );
        default:
          mError = "Unexpected state when evaluating operator!";
          return false;
      }

    case opRegexp:
    case opLike:
    {
      if ( !getValue( value1, mLeft, fields, attributes, geom ) ||
           !getValue( value2, mRight, fields, attributes, geom ) )
        return false;

      // value1 is string to be matched
      // value2 is regular expression

      // XXX does it make sense to use regexp on numbers?
      // in what format should they be?
      if ( value1.isNumeric() || value2.isNumeric() )
      {
        mError = QObject::tr( "Regular expressions on numeric values don't make sense. Use comparison instead." );
        return false;
      }

      // TODO: reuse QRegExp

      QString str = value2.string();
      if ( mOp == opLike ) // change from LIKE syntax to regexp
      {
        // XXX escape % and _  ???
        str.replace( "%", ".*" );
        str.replace( "_", "." );
        return QRegExp( str ).exactMatch( value1.string() );
      }
      else
      {
        return ( QRegExp( str ).indexIn( value1.string() ) != -1 );
      }

    }

    default:
      mError = "Unknown operator: ";
      mError += QString::number( mOp );
      return false;
  }
}

bool QgsSearchTreeNode::getValue( QgsSearchTreeValue& value, QgsSearchTreeNode* node, const QgsFieldMap& fields, const QgsAttributeMap& attributes, QgsGeometry* geom )
{
  value = node->valueAgainst( fields, attributes, geom );
  if ( value.isError() )
  {
    switch (( int )value.number() )
    {
      case 1:
        mError = QObject::tr( "Referenced column wasn't found: %1" ).arg( value.string() );
        break;
      case 2:
        mError = QObject::tr( "Division by zero." );
        break;

        // these should never happen (no need to translate)
      case 3:
        mError = "Unknown operator: ";
        mError += value.string();
        break;
      case 4:
        mError = "Unknown token: ";
        mError += value.string();
        break;
      default:
        mError = "Unknown error!";
        break;
    }
    return false;
  }
  return true;
}

QgsSearchTreeValue QgsSearchTreeNode::valueAgainst( const QgsFieldMap& fields, const QgsAttributeMap& attributes, QgsGeometry* geom )
{
  QgsDebugMsgLevel( "valueAgainst: " + makeSearchString(), 2 );

  switch ( mType )
  {

    case tNumber:
      QgsDebugMsgLevel( "number: " + QString::number( mNumber ), 2 );
      return QgsSearchTreeValue( mNumber );

    case tString:
      QgsDebugMsgLevel( "text: " + EVAL_STR( mText ), 2 );
      return QgsSearchTreeValue( mText );

    case tColumnRef:
    {
      QgsDebugMsgLevel( "column (" + mText.toLower() + "): ", 2 );
      // find field index for the column
      QgsFieldMap::const_iterator it;
      for ( it = fields.begin(); it != fields.end(); it++ )
      {
        if ( it->name().toLower() == mText.toLower() ) // TODO: optimize
          break;
      }

      if ( it == fields.end() )
      {
        // report missing column if not found
        QgsDebugMsgLevel( "ERROR!", 2 );
        return QgsSearchTreeValue( 1, mText );
      }

      // get the value
      QVariant val = attributes[it.key()];
      if ( val.isNull() )
      {
        QgsDebugMsgLevel( "   NULL", 2 );
        return QgsSearchTreeValue();
      }
      else if ( val.type() == QVariant::Bool || val.type() == QVariant::Int || val.type() == QVariant::Double )
      {
        QgsDebugMsgLevel( "   number: " + QString::number( val.toDouble() ), 2 );
        return QgsSearchTreeValue( val.toDouble() );
      }
      else
      {
        QgsDebugMsgLevel( "   text: " + EVAL_STR( val.toString() ), 2 );
        return QgsSearchTreeValue( val.toString() );
      }

    }

    // arithmetic operators
    case tOperator:
    {
      QgsSearchTreeValue value1, value2;
      if ( mLeft )
      {
        if ( !getValue( value1, mLeft, fields, attributes, geom ) ) return value1;
      }
      if ( mRight )
      {
        if ( !getValue( value2, mRight, fields, attributes, geom ) ) return value2;
      }

      if ( mOp == opLENGTH || mOp == opAREA )
      {
        if ( !geom )
        {
          return QgsSearchTreeValue( 2, "Geometry is 0" );
        }

        //check that we don't use area for lines or length for polygons
        if ( mOp == opLENGTH && geom->type() != QGis::Line )
        {
          return QgsSearchTreeValue( 0 );
        }
        if ( mOp == opAREA && geom->type() != QGis::Polygon )
        {
          return QgsSearchTreeValue( 0 );
        }
        return QgsSearchTreeValue( mCalc->measure( geom ) );
      }

      //string operations with one argument
      if ( !mRight && !value1.isNumeric() )
      {
        if ( mOp == opTOINT )
        {
          return QgsSearchTreeValue( value1.string().toInt() );
        }
        else if ( mOp == opTOREAL )
        {
          return QgsSearchTreeValue( value1.string().toDouble() );
        }
      }

      //don't convert to numbers in case of string concatenation
      if ( mLeft && mRight && !value1.isNumeric() && !value2.isNumeric() )
      {
        if ( mOp == opPLUS )
        {
          return QgsSearchTreeValue( value1.string() + value2.string() );
        }
      }

      // for other operators, convert strings to numbers if needed
      double val1, val2;
      if ( value1.isNumeric() )
        val1 = value1.number();
      else
        val1 = value1.string().toDouble();
      if ( value2.isNumeric() )
        val2 = value2.number();
      else
        val2 = value2.string().toDouble();

      switch ( mOp )
      {
        case opPLUS:
          return QgsSearchTreeValue( val1 + val2 );
        case opMINUS:
          return QgsSearchTreeValue( val1 - val2 );
        case opMUL:
          return QgsSearchTreeValue( val1 * val2 );
        case opDIV:
          if ( val2 == 0 )
            return QgsSearchTreeValue( 2, "" ); // division by zero
          else
            return QgsSearchTreeValue( val1 / val2 );
        default:
          return QgsSearchTreeValue( 3, QString::number( mOp ) ); // unknown operator
        case opPOW:
          if (( val1 == 0 && val2 < 0 ) || ( val2 < 0 && ( val2 - floor( val2 ) ) > 0 ) )
          {
            return QgsSearchTreeValue( 4, "Error in power function" );
          }
          return QgsSearchTreeValue( pow( val1, val2 ) );
        case opSQRT:
          return QgsSearchTreeValue( sqrt( val1 ) );
        case opSIN:
          return QgsSearchTreeValue( sin( val1 ) );
        case opCOS:
          return QgsSearchTreeValue( cos( val1 ) );
        case opTAN:
          return QgsSearchTreeValue( tan( val1 ) );
        case opASIN:
          return QgsSearchTreeValue( asin( val1 ) );
        case opACOS:
          return QgsSearchTreeValue( acos( val1 ) );
        case opATAN:
          return QgsSearchTreeValue( atan( val1 ) );
        case opTOINT:
          return QgsSearchTreeValue( int( val1 ) );
        case opTOREAL:
          return QgsSearchTreeValue( val1 );
        case opTOSTRING:
          return QgsSearchTreeValue( QString::number( val1 ) );
      }
    }

    default:
      return QgsSearchTreeValue( 4, QString::number( mType ) ); // unknown token
  }
}


int QgsSearchTreeValue::compare( QgsSearchTreeValue& value1, QgsSearchTreeValue& value2, Qt::CaseSensitivity cs )
{
  if ( value1.isNumeric() || value2.isNumeric() )
  {
    // numeric comparison

    // convert to numbers if needed
    double val1, val2;
    if ( value1.isNumeric() )
      val1 = value1.number();
    else
      val1 = value1.string().toDouble();
    if ( value2.isNumeric() )
      val2 = value2.number();
    else
      val2 = value2.string().toDouble();

    QgsDebugMsgLevel( "NUM_COMP: " + QString::number( val1 ) + " ~ " + QString::number( val2 ), 2 );

    if ( val1 < val2 )
      return -1;
    else if ( val1 > val2 )
      return 1;
    else
      return 0;
  }
  else
  {
    // string comparison
    return value1.string().compare( value2.string(), cs );
  }
}
