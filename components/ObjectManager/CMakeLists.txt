set(COMPONENT_SRCDIRS "." ObjectManagerIdList)
set(COMPONENT_ADD_INCLUDEDIRS "." ObjectManagerIdList)
set(COMPONENT_REQUIRES spiffs bt ObjectTransferGattServer freertos FilterOrder PP_SD_CARD)
register_component()
