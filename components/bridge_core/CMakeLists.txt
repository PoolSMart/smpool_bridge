idf_component_register(SRCS "bridge_core.c"
                    INCLUDE_DIRS "include"
                    REQUIRES  mqtt
                        nvs_flash 
                        freertos
                        esp-tls
                    PRIV_REQUIRES
                        my_espnow
                        my_mqtt
                    )

