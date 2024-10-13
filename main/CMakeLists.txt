# Znajdź wszystkie pliki źródłowe w katalogu main
FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.*)

# Zarejestruj komponent z plikami źródłowymi oraz dołączanymi plikami
idf_component_register(SRCS ${app_sources}
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES duckdns-org.pem)

# Tworzenie obrazu SPIFFS z plików w katalogu spiffs_image
set(WEB_SRC_DIR2 "${CMAKE_CURRENT_SOURCE_DIR}/spiffs_image")
spiffs_create_partition_image(storage ${WEB_SRC_DIR2} FLASH_IN_PROJECT)

# Sprawdzenie konfiguracji i wczytywanie plików z www
if(CONFIG_EXAMPLE_WEB_DEPLOY_SF)
    set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../front/web-demo")
    if(EXISTS ${WEB_SRC_DIR}/dist)
        spiffs_create_partition_image(www ${WEB_SRC_DIR}/dist FLASH_IN_PROJECT)
    else()
        message(FATAL_ERROR "${WEB_SRC_DIR}/dist doesn't exist. Please run 'npm run build' in ${WEB_SRC_DIR}")
    endif()
endif()

