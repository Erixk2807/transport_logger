set(srcs "main.c"
	     "handle_command.c"
         "crc.c"
         "read_data.c"
         "globals.c")
idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS ".")
spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)