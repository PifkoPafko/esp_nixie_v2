idf_component_register( SRCS "pp_nixie_display_manager.c"
	INCLUDE_DIRS "."
	REQUIRES main PP_I2C PP_PCA9698)