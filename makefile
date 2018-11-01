OBJS_DIR=../build/

SOURCES = paroduscl.c       \
          paroduscl_utils.c

OBJS=$(addprefix $(OBJS_DIR), $(addsuffix .o, $(basename $(SOURCES))))

PARODUS_CLIENT_LIB=$(OBJS_DIR)libparoduscl.so

.phony: all clean

all: $(OBJS_DIR) $(PARODUS_CLIENT_LIB)

$(OBJS_DIR):
		if [ ! -d $(OBJS_DIR) ]; then \
			mkdir $(OBJS_DIR); \
		fi

$(OBJS_DIR)%.o: %.c
		@echo Compiling $<...
		$(CC) -c $< $(CFLAGS) -o $@

$(PARODUS_CLIENT_LIB): $(OBJS)
		@echo Creating shared library...
		$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(INCLUDE_LIBS) -shared -o $(PARODUS_CLIENT_LIB)

clean:
		@echo Cleaning...
		rm -f $(OBJS) $(PARODUS_CLIENT_LIB)
