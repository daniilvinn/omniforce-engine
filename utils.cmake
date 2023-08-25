function(omni_set_project_ide_folder TARGET_NAME PROJECT_SOURCE_DIR)
  # globally enable sorting targets into folders in IDEs
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  get_filename_component(PARENT_FOLDER ${PROJECT_SOURCE_DIR} PATH)
  get_filename_component(FOLDER_NAME ${PARENT_FOLDER} NAME)

  set(IDE_FOLDER "${FOLDER_NAME}")

  if(${PROJECT_SOURCE_DIR} MATCHES "${CMAKE_SOURCE_DIR}/")
    set(IDE_FOLDER "")
    string(REPLACE "${CMAKE_SOURCE_DIR}/" "" PARENT_FOLDER ${PROJECT_SOURCE_DIR})

    get_filename_component(PARENT_FOLDER "${PARENT_FOLDER}" PATH)
    get_filename_component(FOLDER_NAME "${PARENT_FOLDER}" NAME)

    get_filename_component(PARENT_FOLDER2 "${PARENT_FOLDER}" PATH)

    while(NOT ${PARENT_FOLDER2} STREQUAL "")
      set(IDE_FOLDER "${FOLDER_NAME}/${IDE_FOLDER}")

      get_filename_component(PARENT_FOLDER "${PARENT_FOLDER}" PATH)
      get_filename_component(FOLDER_NAME "${PARENT_FOLDER}" NAME)

      get_filename_component(PARENT_FOLDER2 "${PARENT_FOLDER}" PATH)
    endwhile()
  endif()
  
  set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER ${IDE_FOLDER})
  
endfunction()