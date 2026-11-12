add_definitions(
        -Wno-stringop-truncation
        -Wno-format
        -Wno-restrict
)
include_directories(
    .
    $(CMAKE_CURRENT_SOURCE_DIR)/../mxml-3.3.1/
    $(CMAKE_CURRENT_SOURCE_DIR)/../base64/include/
)
add_library(onvif OBJECT
        onvif.c
    #    onvif_io.c
        onvif_user_token.c
        onvif_resource_xml.c
 )