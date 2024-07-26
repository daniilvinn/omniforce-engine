file(GLOB_RECURSE BC7ENC_RDO_FILES
	"include/*.h"
	"src/*.cpp"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${BC7ENC_RDO_FILES})

add_library(bc7enc_rdo ${BC7ENC_RDO_FILES})

target_include_directories(bc7enc_rdo PRIVATE "include/")