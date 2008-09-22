SET(molo_UIS
  climateconverterbase.ui         
  climatedataprocessorguibase.ui
  )

QT4_WRAP_UI(molo_UIS_H ${molo_UIS})

ADD_CUSTOM_TARGET(ui ALL DEPENDS ${molo_UIS_H})
