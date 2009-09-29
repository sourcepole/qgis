class QgsPluginLayer : QgsMapLayer
{
%TypeHeaderCode
#include "qgspluginlayer.h"
%End


  /** Constructor */
  QgsPluginLayer( QString lyrname = QString::null, QString source = QString::null );

  /** Destructor */
  virtual ~QgsPluginLayer();

signals:

  void layerDeleted();

private:                       // Private methods

  /** plugin layers are not copyable */
  QgsPluginLayer( const QgsPluginLayer & rhs );

};

