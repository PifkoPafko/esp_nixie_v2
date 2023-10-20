set(COMPONENT_SRCDIRS "." ObjectManagerIdList)
set(COMPONENT_ADD_INCLUDEDIRS "." ObjectManagerIdList)
set(COMPONENT_REQUIRES spiffs bt ObjectTransferGattServer freertos FilterOrder)
register_component()
