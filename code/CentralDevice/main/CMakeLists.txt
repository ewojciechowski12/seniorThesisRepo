# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS main.c         # list the source files of this component
    INCLUDE_DIRS "."      # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES esp_wifi esp_http_server nvs_flash esp_netif esp_event
    EMBED_TXTFILES "index.html"
    PRIV_REQUIRES       # optional, list the private requirements
)
