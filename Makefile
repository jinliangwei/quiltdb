# Assuming this Makefile lives in project root directory
# Do not use CURDIR in case make is run from elsewhere.

PROJ_DIR := $(shell readlink $(dir $(lastword $(MAKEFILE_LIST))) -f)

SRC = $(PROJ_DIR)/src
QUILTDB_SRC = $(SRC)/quiltdb
THIRD_PARTY = $(PROJ_DIR)/third_party
THIRD_PARTY_INSTALLED = $(PROJ_DIR)/third_party_installed
BIN = $(PROJ_DIR)/bin

CXX = g++
INCFLAGS = -I$(THIRD_PARTY_INSTALLED)/include -I$(SRC)
LIBFLAGS = -L$(THIRD_PARTY_INSTALLED)/lib

CPPFLAGS = -g -O3

# all builds third party libraries and petuum
all: quiltdb

mk_bin:
	mkdir -p $(BIN)

quiltdb: mk_bin
	touch $(BIN)/fake_quiltdb

# Compile third party libraries.
third_party: $(THIRD_PARTY_INSTALLED)

clean:
	rm -rf $(BIN)

veryclean:
	rm -rf $(BIN)
	rm -rf $(THIRD_PARTY_INSTALLED)

include $(THIRD_PARTY)/third_party.mk

include $(PROJ_DIR)/apps/apps.mk

.PHONY: all petuum_ps
