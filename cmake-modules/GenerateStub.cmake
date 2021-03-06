find_package(PkgConfig REQUIRED)

pkg_check_modules(Glibmm REQUIRED glibmm-2.4>=2.42.0)
pkg_check_modules(Giomm REQUIRED giomm-2.4>=2.42.0)

function(generate_stub INTROSPECTION_XML GENERATED_DIR GENERATED_PREFIX)
    set (CODEGEN "gdbus-codegen-glibmm3")
    set (GENERATED_DIR "generated")

    set (GENERATED_COMMON_SOURCES
        "${CMAKE_BINARY_DIR}/${GENERATED_DIR}/${GENERATED_PREFIX}_common.h"
        "${CMAKE_BINARY_DIR}/${GENERATED_DIR}/${GENERATED_PREFIX}_common.cpp"
        )

    set(GENERATED_PROXY_SOURCES
        "${CMAKE_BINARY_DIR}/${GENERATED_DIR}/${GENERATED_PREFIX}_proxy.h"
        "${CMAKE_BINARY_DIR}/${GENERATED_DIR}/${GENERATED_PREFIX}_proxy.cpp"
        )

    set (GENERATED_STUB_SOURCES
        "${CMAKE_BINARY_DIR}/${GENERATED_DIR}/${GENERATED_PREFIX}_stub.h"
        "${CMAKE_BINARY_DIR}/${GENERATED_DIR}/${GENERATED_PREFIX}_stub.cpp"
        )

    set (GENERATED_ALL_SOURCES
        ${GENERATED_COMMON_SOURCES}
        ${GENERATED_PROXY_SOURCES}
        ${GENERATED_STUB_SOURCES}
        )

    set (${GENERATED_PREFIX}_GENERATED_PROXY_SOURCES
        ${GENERATED_COMMON_SOURCES}
        ${GENERATED_PROXY_SOURCES}
        )

    set (${GENERATED_PREFIX}_GENERATED_STUB_SOURCES
        ${GENERATED_COMMON_SOURCES}
        ${GENERATED_STUB_SOURCES}
        )

    ADD_CUSTOM_COMMAND (OUTPUT ${GENERATED_ALL_SOURCES}
        COMMAND mkdir -p ${CMAKE_BINARY_DIR}/${GENERATED_DIR}/
        COMMAND ${CODEGEN} --generate-cpp-code=${CMAKE_BINARY_DIR}/${GENERATED_DIR}/${GENERATED_PREFIX} ${INTROSPECTION_XML}
        DEPENDS ${INTROSPECTION_XML})

    add_library(${GENERATED_PREFIX}-stub OBJECT ${${GENERATED_PREFIX}_GENERATED_STUB_SOURCES})
    add_library(${GENERATED_PREFIX}-proxy OBJECT ${${GENERATED_PREFIX}_GENERATED_PROXY_SOURCES})

    target_include_directories(${GENERATED_PREFIX}-stub PRIVATE ${Glibmm_INCLUDE_DIRS} ${Giomm_INCLUDE_DIRS})
    target_include_directories(${GENERATED_PREFIX}-proxy PRIVATE ${Glibmm_INCLUDE_DIRS} ${Giomm_INCLUDE_DIRS})

    target_link_libraries(${GENERATED_PREFIX}-stub PRIVATE ${Glibmm_LIBRARIES} ${Giomm_LIBRARIES})
    target_link_libraries(${GENERATED_PREFIX}-proxy PRIVATE ${Glibmm_LIBRARIES} ${Giomm_LIBRARIES})

endfunction()
