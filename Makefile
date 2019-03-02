#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := esp32_wifi_io

include $(IDF_PATH)/make/project.mk

ota: all
ifeq ($(strip $(IP_ADDR)),)
	@echo "\nERROR: Please specify IP_ADDR."
else
	curl $(IP_ADDR)/ota/ --write-out 'Elapsed Time: %{time_total}s (speed: %{speed_upload} bytes/sec)\n' \
		--no-buffer --data-binary @- < build/$(PROJECT_NAME).bin
endif
