# Project Name
TARGET = zic

# Sources
CPP_SOURCES = zic.cpp

# Library Locations
DAISYSP_DIR ?= ./DaisySP
LIBDAISY_DIR ?= ./libDaisy
# DAISYSP_DIR ?= ../DaisyExamples/DaisySP
# LIBDAISY_DIR ?= ../DaisyExamples/libDaisy

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

push:
	git add . && git commit -m "$(m)" && git push