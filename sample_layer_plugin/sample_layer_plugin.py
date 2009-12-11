"""
/***************************************************************************
Sample Layer Plugin
A QGIS plugin
Plugin using plugin layers

                             -------------------
begin                : 2009-11-30
copyright            : (C) 2009 by Mathias Walker, Sourcepole
email                : mwa at sourcepole.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
"""
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from qgis.core import *

import resources

class SamplePluginLayer(QgsMapLayer):
  def __init__(self, *args):
    QgsMapLayer.__init__(self, *args)

    crs = QgsCoordinateReferenceSystem()
    crs.createFromEpsg(4326)
    self.setCrs(crs)
    self.setCustomProperty("width", QVariant(256))

  def __del__(self):
    print "SamplePluginLayer removed"

  def draw(self, rendererContext):
    painter = rendererContext.painter()
    painter.setPen(Qt.red)
    width = self.customProperty("width", QVariant(128)).toInt()[0]
    painter.drawRect(32, 32, width, 128)
    return True

  def writeXml(self, node, doc):
    element = node.toElement();
    # write plugin layer type to project
    element.setAttribute("type", "sampleplugin");
    return True

#------------------------------------------------------------------------------

class SamplePluginLayerCreator(QgsPluginLayerCreator):
  def __init__(self, createCallback):
    QgsPluginLayerCreator.__init__(self)
    self.createCallback = createCallback

  def createLayer(self, layerNode):
    return self.createCallback(layerNode)

#------------------------------------------------------------------------------

class SampleLayerPlugin:

  def __init__(self, iface):
    # Save reference to the QGIS interface
    self.iface = iface

    self.creator = SamplePluginLayerCreator(self.createLayer)
    self.layers = []

  def initGui(self):
    # Create action that will start plugin configuration
    self.action = QAction(QIcon(":/plugins/sample_layer_plugin/icon.png"), "Add plugin layer", self.iface.mainWindow())
    # connect the action to the run method
    QObject.connect(self.action, SIGNAL("triggered()"), self.run)

    # Add toolbar button and menu item
    self.iface.addToolBarIcon(self.action)
    self.iface.addPluginToMenu("Sample layer plugin", self.action)

    self.creatorId = QgsPluginLayerRegistry.instance().addCreator(self.creator)
    # handle layer remove
    QObject.connect(QgsMapLayerRegistry.instance(), SIGNAL("layerWillBeRemoved(QString)"), self.removeLayer)

  def unload(self):
    # Remove the plugin menu item and icon
    self.iface.removePluginMenu("Sample layer plugin",self.action)
    self.iface.removeToolBarIcon(self.action)

    QgsPluginLayerRegistry.instance().removeCreator(self.creatorId)
    QObject.disconnect(QgsMapLayerRegistry.instance(), SIGNAL("layerWillBeRemoved(QString)"), self.removeLayer)

  def run(self):
    layer = SamplePluginLayer(QgsMapLayer.PluginLayer, "Plugin Layer", "")
    if layer.isValid():
      self.layers.append(layer)
      QgsMapLayerRegistry.instance().addMapLayer(layer)

  def createLayer(self, layerNode):
    layer = None

    # read layer type
    element = layerNode.toElement();
    layerType = element.attribute("type");
    if layerType == "sampleplugin":
      layer = SamplePluginLayer(QgsMapLayer.PluginLayer)
      self.layers.append(layer)

    return layer

  def removeLayer(self, layerId):
    layerToRemove = None
    for layer in self.layers:
      # check if this is one of the plugin's layer
      if layer.getLayerID() == layerId:
        layerToRemove = layer
        break

    if layerToRemove != None:
      self.layers.remove(layerToRemove)
