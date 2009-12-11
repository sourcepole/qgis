/** callback class for creating plugin specific layers */
class QgsPluginLayerCreator
{
%TypeHeaderCode
#include "qgspluginlayerregistry.h"
%End

  public:

    QgsPluginLayerCreator();
    virtual ~QgsPluginLayerCreator();

    /** return new layer if node data corresponds to the parent plugin, else return NULL */
    virtual QgsMapLayer* createLayer(const QDomNode& layerNode);
};

/** a registry of callbacks to create plugin layers */
class QgsPluginLayerRegistry
{
%TypeHeaderCode
#include "qgspluginlayerregistry.h"
%End

  public:

    /** means of accessing canonical single instance  */
    static QgsPluginLayerRegistry* instance();

    ~QgsPluginLayerRegistry();

    /** add plugin layer creator and return unique id */
    unsigned int addCreator(QgsPluginLayerCreator* creator);
    /** remove plugin layer creator with id */
    void removeCreator(unsigned int id);

    /** return new layer if corresponding plugin has been found, else return NULL */
    QgsMapLayer* createLayer(const QDomNode& layerNode);

  private:

    /** private since instance() creates it */
    QgsPluginLayerRegistry();
};
