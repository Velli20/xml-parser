

CC           = gcc
CFLAGS       = -O3
LDFLAGS      = -Wl,-Map,$(PROGRAM).map -Wl,--heap=10

SRC_DIRS     += src
INC_DIRS     += inc
OBJ_DIR      := BUILD
PROGRAM      ?= xml_parser_test.exe

SOURCE_FILES := $(wildcard $(SRC_DIRS)/*.c)
OBJECT_FILES := $(patsubst %.c, $(OBJ_DIR)/%.o, $(SOURCE_FILES))
SOURCE_DEPS  := $(OBJECT_FILES:.o=.d)

$(OBJ_DIR):
	mkdir $@

$(PROGRAM): $(OBJECT_FILES)
	@echo 'Building targets: $<'
	$(CC) $(LDFLAGS) -o $(OBJ_DIR)/$(PROGRAM) $(OBJECT_FILES)

$(OBJECT_FILES): $(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@echo 'compile $<'
	@mkdir -p $(@D)
	$(CC) -I$(INC_DIRS) -c $(CFLAGS) -o  $@ $<

all: $(PROGRAM)

run: 
	$(OBJ_DIR)/$(PROGRAM)

.PHONY: all run