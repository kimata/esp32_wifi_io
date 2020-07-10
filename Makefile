#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := esp32_wifi_io
ANGULAR_DIR  := ./angular

include $(IDF_PATH)/make/project.mk

ota: build/$(PROJECT_NAME).bin
ifeq ($(strip $(IP_ADDR)),)
	@echo "\nERROR: Please specify IP_ADDR."
else
	echo -n "\nFirmware: "
	du -h build/$(PROJECT_NAME).bin
	echo ""
	curl $(IP_ADDR)/ota/ --write-out '\nElapsed Time: %{time_total}s (speed: %{speed_upload} bytes/sec)\n' \
		--no-buffer --data-binary @- < build/$(PROJECT_NAME).bin
endif

component-main-build: $(ANGULAR_DIR)/dist/esp32-wifi-io/index.html

$(ANGULAR_DIR)/dist/esp32-wifi-io/index.html:
	$(MAKE) -C $(ANGULAR_DIR)

.PHONY: angular
