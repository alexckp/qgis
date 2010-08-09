/***************************************************************************
  qgsdelimitedtextprovider.cpp -  Data provider for delimted text
  -------------------
          begin                : 2004-02-27
          copyright            : (C) 2004 by Gary E.Sherman
          email                : sherman at mrcc.com
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

#include "qgsdelimitedtextprovider.h"
#include "qgsdelimitedtextfeatureiterator.h"

#include <QtGlobal>
#include <QFile>
#include <QDataStream>
#include <QTextStream>
#include <QStringList>
#include <QMessageBox>
#include <QSettings>
#include <QRegExp>
#include <QUrl>


#include "qgsdataprovider.h"
#include "qgsfeature.h"
#include "qgsfield.h"
#include "qgslogger.h"
#include "qgsmessageoutput.h"
#include "qgsrectangle.h"
#include "qgscoordinatereferencesystem.h"
#include "qgis.h"

static const QString TEXT_PROVIDER_KEY = "delimitedtext";
static const QString TEXT_PROVIDER_DESCRIPTION = "Delimited text data provider";


QStringList QgsDelimitedTextProvider::splitLine( QString line )
{
  //QgsDebugMsg( "Attempting to split the input line: " + line + " using delimiter " + mDelimiter );

  QStringList parts;
  if ( mDelimiterType == "regexp" )
    parts = line.split( mDelimiterRegexp );
  else
    parts = line.split( mDelimiter );

  //QgsDebugMsg( "Split line into " + QString::number( parts.size() ) + " parts" );

  if ( mDelimiterType == "plain" )
  {
    QChar delim;
    int i = 0, first = parts.size();
    while ( i < parts.size() )
    {
      if ( delim == 0 && ( parts[i][0] == '"' || parts[i][0] == '\'' ) )
      {
        delim = parts[i][0];
        first = i;
        continue;
      }

      if ( delim != 0 && parts[i][ parts[i].length() - 1 ] == delim )
      {
        parts[first] = parts[first].mid( 1 );
        parts[i] = parts[i].left( parts[i].length() - 1 );

        if ( first < i )
        {
          QStringList values;
          while ( first <= i )
          {
            values << parts[first];
            parts.takeAt( first );
            i--;
          }

          parts.insert( first, values.join( mDelimiter ) );
        }

        first = -1;
        delim = 0;
      }

      i++;

      if ( i == parts.size() && first >= 0 )
      {
        i = first + 1;
        first = -1;
        delim = 0;
      }
    }
  }

  return parts;
}

QgsDelimitedTextProvider::QgsDelimitedTextProvider( QString uri )
    : QgsVectorDataProvider( uri ),
    mXFieldIndex( -1 ), mYFieldIndex( -1 ),
    mShowInvalidLines( true )
{
  // Get the file name and mDelimiter out of the uri
  mFileName = uri.left( uri.indexOf( "?" ) );
  // split the string up on & to get the individual parameters
  QStringList parameters = uri.mid( uri.indexOf( "?" ) ).split( "&", QString::SkipEmptyParts );

  QgsDebugMsg( "Parameter count after split on &" + QString::number( parameters.size() ) );

  // get the individual parameters and assign values
  QStringList temp = parameters.filter( "delimiter=" );
  mDelimiter = temp.size() ? temp[0].mid( temp[0].indexOf( "=" ) + 1 ) : "";
  temp = parameters.filter( "delimiterType=" );
  mDelimiterType = temp.size() ? temp[0].mid( temp[0].indexOf( "=" ) + 1 ) : "";
  temp = parameters.filter( "xField=" );
  QString xField = temp.size() ? temp[0].mid( temp[0].indexOf( "=" ) + 1 ) : "";
  temp = parameters.filter( "yField=" );
  QString yField = temp.size() ? temp[0].mid( temp[0].indexOf( "=" ) + 1 ) : "";
  // Decode the parts of the uri. Good if someone entered '=' as a delimiter, for instance.
  mFileName  = QUrl::fromPercentEncoding( mFileName.toUtf8() );
  mDelimiter = QUrl::fromPercentEncoding( mDelimiter.toUtf8() );
  mDelimiterType = QUrl::fromPercentEncoding( mDelimiterType.toUtf8() );
  xField    = QUrl::fromPercentEncoding( xField.toUtf8() );
  yField    = QUrl::fromPercentEncoding( yField.toUtf8() );

  QgsDebugMsg( "Data source uri is " + uri );
  QgsDebugMsg( "Delimited text file is: " + mFileName );
  QgsDebugMsg( "Delimiter is: " + mDelimiter );
  QgsDebugMsg( "Delimiter type is: " + mDelimiterType );
  QgsDebugMsg( "xField is: " + xField );
  QgsDebugMsg( "yField is: " + yField );

  // if delimiter contains some special characters, convert them
  if ( mDelimiterType == "regexp" )
    mDelimiterRegexp = QRegExp( mDelimiter );
  else
    mDelimiter.replace( "\\t", "\t" ); // replace "\t" with a real tabulator

  // assume the layer is invalid until proven otherwise
  mValid = false;
  if ( mFileName.isEmpty() || mDelimiter.isEmpty() || xField.isEmpty() || yField.isEmpty() )
  {
    // uri is invalid so the layer must be too...
    QString( "Data source is invalid" );
    return;
  }

  // check to see that the file exists and perform some sanity checks
  if ( !QFile::exists( mFileName ) )
  {
    QgsDebugMsg( "Data source " + dataSourceUri() + " doesn't exist" );
    return;
  }

  // Open the file and get number of rows, etc. We assume that the
  // file has a header row and process accordingly. Caller should make
  // sure the the delimited file is properly formed.
  mFile = new QFile( mFileName );
  if ( !mFile->open( QIODevice::ReadOnly ) )
  {
    QgsDebugMsg( "Data source " + dataSourceUri() + " could not be opened" );
    delete mFile;
    return;
  }

  // now we have the file opened and ready for parsing

  // set the initial extent
  mExtent = QgsRectangle();

  QMap<int, bool> couldBeInt;
  QMap<int, bool> couldBeDouble;

  mStream = new QTextStream( mFile );
  QString line;
  mNumberFeatures = 0;
  int lineNumber = 0;
  bool firstPoint = true;
  bool hasFields = false;
  while ( !mStream->atEnd() )
  {
    lineNumber++;
    line = mStream->readLine(); // line of text excluding '\n', default local 8 bit encoding.
    if ( !hasFields )
    {
      // Get the fields from the header row and store them in the
      // fields vector
      QStringList fieldList = splitLine( line );

      // We don't know anything about a text based field other
      // than its name. All fields are assumed to be text
      int fieldPos = 0;
      for ( QStringList::Iterator it = fieldList.begin();
            it != fieldList.end(); ++it, ++fieldPos )
      {
        QString field = *it;
        if ( field.length() == 0 )
          continue; // ignore

        // for now, let's set field type as text
        mAttributeVector.append( QgsField( *it, QVariant::String, "Text" ) );

        // check to see if this field matches either the x or y field
        if ( xField == *it )
        {
          QgsDebugMsg( "Found x field: " + ( *it ) );
          mXFieldIndex = fieldPos;
        }
        else if ( yField == *it )
        {
          QgsDebugMsg( "Found y field: " + ( *it ) );
          mYFieldIndex = fieldPos;
        }

        QgsDebugMsg( "Adding field: " + ( *it ) );
        // assume that the field could be integer or double
        couldBeInt.insert( fieldPos, true );
        couldBeDouble.insert( fieldPos, true );
      }
      QgsDebugMsg( "Field count for the delimited text file is " + QString::number( mAttributeVector.size() ) );
      hasFields = true;
    }
    else if ( mXFieldIndex != -1 && mYFieldIndex != -1 )
    {
      mNumberFeatures++;

      // split the line on the delimiter
      QStringList parts = splitLine( line );

      // Skip malformed lines silently. Report line number with nextFeature()
      if ( mAttributeVector.size() != parts.size() )
      {
        continue;
      }

      // Get the x and y values, first checking to make sure they
      // aren't null.
      QString sX = parts[mXFieldIndex];
      QString sY = parts[mYFieldIndex];

      bool xOk = true;
      bool yOk = true;
      double x = sX.toDouble( &xOk );
      double y = sY.toDouble( &yOk );

      if ( xOk && yOk )
      {
        if ( !firstPoint )
        {
          mExtent.combineExtentWith( x, y );
        }
        else
        { // Extent for the first point is just the first point
          mExtent.set( x, y, x, y );
          firstPoint = false;
        }
      }

      int i = 0;
      for ( QStringList::iterator it = parts.begin(); it != parts.end(); ++it, ++i )
      {
        // try to convert attribute values to integer and double
        if ( couldBeInt[i] && !it->isEmpty() )
        {
          it->toInt( &couldBeInt[i] );
        }
        if ( couldBeDouble[i] && !it->isEmpty() )
        {
          it->toDouble( &couldBeDouble[i] );
        }
      }
    }
  }

  // now it's time to decide the types for the fields
  for ( int i = 0; i < mAttributeVector.size(); i++ )
  {
    if ( couldBeInt[i] )
    {
      mAttributeVector[i].setType( QVariant::Int );
      mAttributeVector[i].setTypeName( "integer" );
    }
    else if ( couldBeDouble[i] )
    {
      mAttributeVector[i].setType( QVariant::Double );
      mAttributeVector[i].setTypeName( "double" );
    }

    // convert mAttributeVector items to field map (legacy)
    mAttributeFields[i] = mAttributeVector[i];
  }

  if ( mXFieldIndex != -1 && mYFieldIndex != -1 )
  {
    QgsDebugMsg( "Data store is valid" );
    QgsDebugMsg( "Number of features " + QString::number( mNumberFeatures ) );
    QgsDebugMsg( "Extents " + mExtent.toString() );

    mValid = true;
  }
  else
  {
    QgsDebugMsg( "Data store is invalid. Specified x,y fields do not match those in the database" );
  }
  QgsDebugMsg( "Done checking validity" );

}

QgsDelimitedTextProvider::~QgsDelimitedTextProvider()
{
  mOldApiIter.close();

  mFile->close();
  delete mFile;
  delete mStream;
}


QString QgsDelimitedTextProvider::storageType() const
{
  return "Delimited text file";
}

QgsFeatureIterator QgsDelimitedTextProvider::getFeatures( QgsAttributeList fetchAttributes,
                                                          QgsRectangle rect,
                                                          bool fetchGeometry,
                                                          bool useIntersect )
{
  return QgsFeatureIterator( new QgsDelimitedTextFeatureIterator(this, fetchAttributes, rect, fetchGeometry, useIntersect));
}


void QgsDelimitedTextProvider::showInvalidLinesErrors()
{
  // TODO: this should be modified to only set an error string that can be read from provider

  if ( mShowInvalidLines && !mInvalidLines.isEmpty() )
  {
    mShowInvalidLines = false;
    QgsMessageOutput* output = QgsMessageOutput::createMessageOutput();
    output->setTitle( tr( "Error" ) );
    output->setMessage( tr( "Note: the following lines were not loaded because Qgis was "
                            "unable to determine values for the x and y coordinates:\n" ),
                        QgsMessageOutput::MessageText );

    output->appendMessage( "Start of invalid lines." );
    for ( int i = 0; i < mInvalidLines.size(); ++i )
      output->appendMessage( mInvalidLines.at( i ) );
    output->appendMessage( "End of invalid lines." );

    output->showMessage();

    // We no longer need these lines.
    mInvalidLines.empty();
  }
}



// Return the extent of the layer
QgsRectangle QgsDelimitedTextProvider::extent()
{
  return mExtent;
}

/**
 * Return the feature type
 */
QGis::WkbType QgsDelimitedTextProvider::geometryType() const
{
  return QGis::WKBPoint;
}

/**
 * Return the feature type
 */
long QgsDelimitedTextProvider::featureCount() const
{
  return mNumberFeatures;
}

/**
 * Return the number of fields
 */
uint QgsDelimitedTextProvider::fieldCount() const
{
  return mAttributeVector.size();
}


const QgsFieldMap & QgsDelimitedTextProvider::fields() const
{
  return mAttributeFields;
}

bool QgsDelimitedTextProvider::isValid()
{
  return mValid;
}


int QgsDelimitedTextProvider::capabilities() const
{
  return NoCapabilities;
}


QgsCoordinateReferenceSystem QgsDelimitedTextProvider::crs()
{
  // TODO: make provider projection-aware
  return QgsCoordinateReferenceSystem(); // return default CRS
}




QString  QgsDelimitedTextProvider::name() const
{
  return TEXT_PROVIDER_KEY;
} // ::name()



QString  QgsDelimitedTextProvider::description() const
{
  return TEXT_PROVIDER_DESCRIPTION;
} //  QgsDelimitedTextProvider::name()


/**
 * Class factory to return a pointer to a newly created
 * QgsDelimitedTextProvider object
 */
QGISEXTERN QgsDelimitedTextProvider *classFactory( const QString *uri )
{
  return new QgsDelimitedTextProvider( *uri );
}

/** Required key function (used to map the plugin to a data store type)
*/
QGISEXTERN QString providerKey()
{
  return TEXT_PROVIDER_KEY;
}

/**
 * Required description function
 */
QGISEXTERN QString description()
{
  return TEXT_PROVIDER_DESCRIPTION;
}

/**
 * Required isProvider function. Used to determine if this shared library
 * is a data provider plugin
 */
QGISEXTERN bool isProvider()
{
  return true;
}
