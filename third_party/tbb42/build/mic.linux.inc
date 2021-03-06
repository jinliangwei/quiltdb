# Copyright 2005-2013 Intel Corporation.  All Rights Reserved.
#
# This file is part of Threading Building Blocks.
#
# Threading Building Blocks is free software; you can redistribute it
# and/or modify it under the terms of the GNU General Public License
# version 2 as published by the Free Software Foundation.
#
# Threading Building Blocks is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Threading Building Blocks; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# As a special exception, you may use this file as part of a free software
# library without restriction.  Specifically, if other files instantiate
# templates or use macros or inline functions from this file, or you compile
# this file and link it with other files to produce an executable, this
# file does not by itself cause the resulting executable to be covered by
# the GNU General Public License.  This exception does not however
# invalidate any other reasons why the executable file might be covered by
# the GNU General Public License.

ifeq ($(tbb_os),mic)
  $(error MIC supports only cross-compilation.  Specify "target=mic" instead.)
endif

ifneq ($(BUILDING_PHASE),1)
  # The same build prefix should be used in offload.inc
  export tbb_build_prefix?=mic_icc$(CPF_SUFFIX)
endif

MAKE_VERSIONS=sh $(tbb_root)/build/version_info_linux.sh $(CPLUS) $(CPLUS_FLAGS) $(INCLUDES) >version_string.ver
MAKE_TBBVARS=sh $(tbb_root)/build/generate_tbbvars.sh MIC_ MIC_
def_prefix=lin64

TEST_LAUNCHER=
run_cmd ?= bash $(tbb_root)/build/mic.linux.launcher.sh $(largs)

# detects whether examples are being built.
ifeq ($(BUILDING_PHASE),0)
 export UI = con
 export x64 = 64
endif # examples
