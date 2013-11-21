# This Makefile provides targets for building third_party libraries to $(LIB).

$(THIRD_PARTY_INSTALLED):
	mkdir -p $(THIRD_PARTY_INSTALLED)
	make all_third_party

all_third_party: gflags libunwind glog zmq-3.2 boost-1.54 \
	gperftools tbb-4.2 yaml-cpp

gflags:
	cd $(THIRD_PARTY)/gflags-2.0;\
		./configure --prefix=$(THIRD_PARTY_INSTALLED);\
		make check install clean distclean

# Needed by glog and gperftools
libunwind:
	cd $(THIRD_PARTY)/libunwind-1.1;\
		./configure --prefix=$(THIRD_PARTY_INSTALLED);\
		make install clean distclean

# glog depends on gflags and libunwind.
glog:
	cd $(THIRD_PARTY)/glog-0.3.3;\
                LDFLAGS=-L$(THIRD_PARTY_INSTALLED)/lib LIBS="-lgflags -lunwind" \
                LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:$(THIRD_PARTY_INSTALLED)/lib \
		./configure CPPFLAGS=-I$(THIRD_PARTY_INSTALLED)/include \
		--prefix=$(THIRD_PARTY_INSTALLED);\
		make install #clean distclean

zmq-3.2:
	cd $(THIRD_PARTY)/zeromq-3.2.3;\
		./configure --prefix=$(THIRD_PARTY_INSTALLED);\
		make install #clean distclean
	cp $(THIRD_PARTY)/zmq.hpp $(THIRD_PARTY_INSTALLED)/include

boost-1.54:
	cd $(THIRD_PARTY);\
		rm -rf boost_1_54_0;\
		tar xzf boost_1_54_0.tar.gz;\
		sh install_boost.sh $(THIRD_PARTY_INSTALLED)

# This depends on libunwind-0.99 as well
gperftools:
	cd $(THIRD_PARTY);\
		rm -rf gperftools-2.1;\
		tar xzf gperftools-2.1.tar.gz;\
		cd gperftools-2.1;\
		LDFLAGS=-L$(THIRD_PARTY_INSTALLED)/lib LIBS=-lunwind \
                LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:$(THIRD_PARTY_INSTALLED)/lib \
		./configure CPPFLAGS=-I$(THIRD_PARTY_INSTALLED)/include \
		--prefix=$(THIRD_PARTY_INSTALLED);\
		make install #clean distclean

tbb-4.2_build:
	cd $(THIRD_PARTY)/tbb42; make

tbb-4.2: tbb-4.2_build
	# Find the release folder. There's no easy way to pass Makefile variables to
	# shell environment, so just hard code 'third_party/tbb42'. See
	# http://www.linuxquestions.org/questions/programming-9/exporting-makefile-variables-to-$-shell-environment-807422/
	cp $(shell find third_party/tbb42 -name '*_release')/lib* \
		$(THIRD_PARTY_INSTALLED)/lib/
	cp -r $(THIRD_PARTY)/tbb42/include/tbb \
		$(THIRD_PARTY_INSTALLED)/include/

cmake: $(THIRD_PARTY)/cmake-2.8.12.1.tar.gz
	cd $(THIRD_PARTY); \
	rm -rf cmake-2.8.12.1; \
	tar -xvzf cmake-2.8.12.1.tar.gz; \
	cd cmake-2.8.12.1; \
	./bootstrap --prefix=$(THIRD_PARTY_INSTALLED); \
	make; \
	make install

yaml-cpp: $(THIRD_PARTY)/yaml-cpp-0.5.1.tar.gz cmake
	cd $(THIRD_PARTY); \
	rm -rf yaml-cpp-0.5.1; \
	tar -xvzf yaml-cpp-0.5.1.tar.gz; \
	cd yaml-cpp-0.5.1; \
	mkdir build; \
	cd build; \
	$(THIRD_PARTY_INSTALLED)/bin/cmake ..; \
	make
	cp -r $(THIRD_PARTY)/yaml-cpp-0.5.1/include/* \
	$(THIRD_PARTY_INSTALLED)/include
	cp -r $(THIRD_PARTY)/yaml-cpp-0.5.1/build/*.a \
	$(THIRD_PARTY_INSTALLED)/lib
