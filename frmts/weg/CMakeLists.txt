add_gdal_driver(
    TARGET gdal_WEG
	SOURCES 
	    wegdataset.cpp compmeth.cpp compddesc.cpp ucmpmeth.cpp
        compmet0.cpp compmet1.cpp compmet2.cpp compmet3.cpp compmet4.cpp compmet5.cpp
		wsitypedefs.h
		compmeth.h compddesc.h ucmpmeth.h
        compmet0.h compmet1.h compmet2.h compmet3.h compmet4.h compmet5.h
	    PLUGIN_CAPABLE NO_DEPS)
gdal_standard_includes(gdal_WEG)
