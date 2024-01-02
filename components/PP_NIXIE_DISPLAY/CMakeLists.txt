idf_component_register( SRCS "pp_nixie_display.c"
	INCLUDE_DIRS "."
	REQUIRES main MK_I2C PP_PCA9698)