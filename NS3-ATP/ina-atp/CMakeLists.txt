check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

set(examples_as_tests_sources)
if(${ENABLE_EXAMPLES})
    set(examples_as_tests_sources
        #test/ina-examples-test-suite.cc
        )
endif()

build_lib(
    LIBNAME ina
    SOURCE_FILES helper/simple-ina-helper.cc
                 helper/ina-helper.cc
                 model/ina-switch-net-device.cc
                 model/ina-switch-channel.cc
                 model/ina-switch.cc
                 model/ina-header.cc
                 model/simple-worker-application.cc
                 model/simple-aggregator-application.cc
                 model/ina-worker-application.cc
                 model/ina-aggregator-application.cc
                 model/ina-ps.cc
                  
                 
    HEADER_FILES helper/simple-ina-helper.h
                 helper/ina-helper.h
                 model/ina-switch-net-device.h
                 model/ina-switch-channel.h
                 model/ina-switch.h
                 model/ina-header.h
                 model/simple-worker-application.h
                 model/simple-aggregator-application.h
                 model/ina-worker-application.h
                 model/ina-aggregator-application.h
                 model/ina-ps.h
                
                 
    LIBRARIES_TO_LINK ${libcore}
                      ${libnetwork}  
                      ${libinternet}
                      ${libpoint-to-point}
                      ${libapplications}
    TEST_SOURCES test/ina-test-suite.cc
                 ${examples_as_tests_sources}
)

