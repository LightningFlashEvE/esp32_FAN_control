idf_component_register(SRCS "temp_sensor.c" 
                    INCLUDE_DIRS "." 
                    REQUIRES driver ds18b20 onewire_bus)
