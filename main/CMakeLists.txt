idf_component_register(SRCS "mainApp.cpp"
                            "hidController.cpp"
                            "hidKeyboard.c"
                            "espHiddPrfApi.c"
                            "hidDeviceLePrf.c"
                            "hidDev.c"
                            "myShortcut.cpp"
                            "displayConfig.c"
                    INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable)
