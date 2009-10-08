class QgsPluginLayer : QgsMapLayer
{
%TypeHeaderCode
#include <qgspluginlayer.h>
%End

public:

  /** Constructor */
  QgsPluginLayer( QString pluginId = QString::null, QString lyrname = QString::null, QString source = QString::null );

  /** Destructor */
  virtual ~QgsPluginLayer();

  /** \brief This is called when the view on the raster layer needs to be redrawn */
  bool draw( QgsRenderContext& rendererContext );

  /** True if the layer can be edited */
  virtual bool isEditable() const;

  /** Copies the symbology settings from another layer. Returns true in case of success */
  virtual bool copySymbologySettings( const QgsMapLayer& );

  /** Returns true if this layer can be in the same symbology group with another layer */
  virtual bool hasCompatibleSymbology( const QgsMapLayer& ) const;

  /** Read the symbology for the current layer from the Dom node supplied.
   * @param QDomNode node that will contain the symbology definition for this layer.
   * @param errorMessage reference to string that will be updated with any error messages
   * @return true in case of success.
   */
  virtual bool readSymbology( const QDomNode&, QString& );

  /** Write the symbology for the layer into the document provided.
   *  @param QDomNode the node that will have the style element added to it.
   *  @param QDomDocument the document that will have the QDomNode added.
   *  @param errorMessage reference to string that will be updated with any error messages
   *  @return true in case of success.
   */
  virtual bool writeSymbology( QDomNode&, QDomDocument&, QString& ) const;

  /**
    * Get plugin ID
    */
  QString pluginId();

  /**
    * Set plugin ID
    */
  void setPluginId( const QString& pluginId );

  /**
    * Get plugin specific layer properties from project
    */
  const QDomNode& pluginProperties();

  /**
    * Set layer extent
    */
  void setExtent( const QgsRectangle& extent );

signals:

  //! emitted when layer is deleted
  void layerDeleted();

  //! emitted when drawing layer
  void drawLayer( QgsRenderContext& );

  //! emitted when saving layer
  void writePluginProperties( QDomNode& propertiesNode, QDomDocument& doc );

protected:

  /** \brief Reads layer specific state from project file Dom node */
  bool readXml( QDomNode& layerNode );

  /** \brief Write layer specific state to project file Dom node */
  bool writeXml( QDomNode & layerNode, QDomDocument & doc );

private:

  /** plugin layers are not copyable */
  QgsPluginLayer( const QgsPluginLayer & rhs );

};

