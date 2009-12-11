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
 This script initializes the plugin, making it known to QGIS.
"""
def name():
  return "Sample Layer Plugin" 
def description():
  return "Plugin using plugin layers"
def version():
  return "Version 0.1" 
def qgisMinimumVersion():
  return "1.4"
def classFactory(iface): 
  from sample_layer_plugin import SampleLayerPlugin
  return SampleLayerPlugin(iface)


