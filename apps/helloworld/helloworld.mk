
helloworld: $(APPS_DIR)/helloworld/helloworld.cpp mk_bin
	echo $(INCFLAGS)
	$(CXX) $(INCFLAGS) $(LIBFLAGS) $(CPPFLAGS) $< \
	 $(QUILTDB_SRC_CPP) $(QUILTDB_APP_ST_LIBS) $(QUILTDB_APP_DY_LIBS) -o $(BIN)/$@
