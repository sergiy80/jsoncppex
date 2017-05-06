ADD_EXECUTABLE( jsoncppex_test main.cpp )

INCLUDE_DIRECTORIES( ${jsoncpp_SOURCE_DIR}/include )

IF(BUILD_SHARED_LIBS)
    ADD_DEFINITIONS( -DJSON_DLL )
    TARGET_LINK_LIBRARIES(jsoncppex_test jsoncpp_lib)
ELSE(BUILD_SHARED_LIBS)
    TARGET_LINK_LIBRARIES(jsoncppex_test jsoncpp_lib_static)
ENDIF()
