set(srcs "test_app_main.c"
         "test_i2s.c"
         "test_i2s_iram.c")

idf_component_register(SRCS ${srcs}
                       PRIV_REQUIRES unity esp_driver_pcnt driver spi_flash
                       WHOLE_ARCHIVE)
