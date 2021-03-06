# Install M-PIN module

if(BUILD_MPIN)
  if(${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    INSTALL(FILES ${PROJECT_SOURCE_DIR}/wrappers/python/mpin.py DESTINATION "${PYTHON_SITE_PACKAGES}" PERMISSIONS
            OWNER_WRITE OWNER_READ OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE)

    if (USE_ANONYMOUS)
        INSTALL(FILES ${PROJECT_SOURCE_DIR}/wrappers/python/TestMPINAnonInstall.py DESTINATION bin PERMISSIONS
                OWNER_WRITE OWNER_READ OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE)
    else (USE_ANONYMOUS)
        INSTALL(FILES ${PROJECT_SOURCE_DIR}/wrappers/python/TestMPINInstall.py DESTINATION bin PERMISSIONS
                OWNER_WRITE OWNER_READ OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE)
    endif (USE_ANONYMOUS)

  endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

  if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Windows specific code
    INSTALL(FILES ${PROJECT_SOURCE_DIR}/wrappers/python/mpin.py DESTINATION lib)
    if (USE_ANONYMOUS)
        INSTALL(FILES ${PROJECT_SOURCE_DIR}/wrappers/python/TestMPINAnonInstall.py DESTINATION lib)
    else (USE_ANONYMOUS)
        INSTALL(FILES ${PROJECT_SOURCE_DIR}/wrappers/python/TestMPINInstall.py DESTINATION lib)
    endif (USE_ANONYMOUS)
  endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

  file(COPY mpin.py DESTINATION "${PROJECT_BINARY_DIR}/wrappers/python/")
  file(COPY TimeMPIN.py DESTINATION "${PROJECT_BINARY_DIR}/wrappers/python/")
  if (USE_ANONYMOUS)
      file(COPY TestMPINAnonInstall.py DESTINATION "${PROJECT_BINARY_DIR}/wrappers/python/")
      add_test (test_python_mpin_anon_install python TestMPINAnonInstall.py) 
  else (USE_ANONYMOUS)
      file(COPY TestMPINInstall.py DESTINATION "${PROJECT_BINARY_DIR}/wrappers/python/")   
      add_test (test_python_mpin_install python TestMPINInstall.py) 
  endif (USE_ANONYMOUS) 
  

  if(AMCL_CHOICE STREQUAL "BN254_CX")
      if (USE_ANONYMOUS)
          file(COPY TestMPINAnon.py DESTINATION "${PROJECT_BINARY_DIR}/wrappers/python/")
          add_test (test_python_mpin_anon_BN254_CX python TestMPINAnon.py)
      else (USE_ANONYMOUS)
          file(COPY TestMPIN.py DESTINATION "${PROJECT_BINARY_DIR}/wrappers/python/")
          add_test (test_python_mpin_BN254_CX python TestMPIN.py)
      endif (USE_ANONYMOUS) 
  endif(AMCL_CHOICE STREQUAL "BN254_CX")

endif(BUILD_MPIN)

if(BUILD_WCC)
  if(${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    INSTALL(FILES ${PROJECT_SOURCE_DIR}/wrappers/python/wcc.py DESTINATION "${PYTHON_SITE_PACKAGES}" PERMISSIONS
            OWNER_WRITE OWNER_READ OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE)
  endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

  if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Windows specific code
    INSTALL(FILES ${PROJECT_SOURCE_DIR}/wrappers/python/wcc.py DESTINATION lib)
  endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

  file(COPY wcc.py DESTINATION "${PROJECT_BINARY_DIR}/wrappers/python/")
endif(BUILD_WCC)
