
APPS_DIR = $(PROJ_DIR)/apps

QUILTDB_APP_DY_LIBS = -pthread -Wl,-Bdynamic -ltbb
QUILTDB_APP_ST_LIBS = -Wl,-Bstatic -lzmq -lboost_thread -lboost_system \
		 -lglog -lgflags -lrt -lnsl -luuid -ltcmalloc -lunwind \
		 -lyaml-cpp

QUILTDB_SRC_CPP = $(QUILTDB_SRC)/include/quiltdb.cpp \
	$(QUILTDB_SRC)/internal_table/internal_table.cpp \
	$(QUILTDB_SRC)/propagator/propagator.cpp \
	$(QUILTDB_SRC)/utils/timer_thr.cpp \
	$(QUILTDB_SRC)/receiver/receiver.cpp \
	$(QUILTDB_SRC)/utils/memstruct.cpp \
	$(QUILTDB_SRC)/utils/zmq_util.cpp

include $(APPS_DIR)/helloworld/helloworld.mk