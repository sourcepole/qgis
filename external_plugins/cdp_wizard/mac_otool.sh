#!/bin/bash
install_name_tool -change libqt.3.dylib @executable_path/lib/libqt.3.dylib cdp_wizard 
install_name_tool -change /Library/Frameworks/GDAL.framework/Versions/1.4/GDAL @executable_path/lib/GDAL.framework/Versions/1.4/GDAL cdp_wizard
mkdir cdp_wizard.app/Contents/Resources/
cp application.icns cdp_wizard.app/Contents/Resources/
