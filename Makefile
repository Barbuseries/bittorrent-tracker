DEFINES = -DDEBUG=1 -g

SRC = $(wildcard code/*.c)

OBJS = $(patsubst code/%.c, build/%.o, $(filter-out $(wildcard code/tracker*), $(SRC)))
OBJS += build/tracker_common.o

TARGET = tracker1 tracker2 tracker3 tracker4

TARGET_FULL_PATH = $(patsubst %, dist/%, $(TARGET))
all: $(TARGET_FULL_PATH)

# Still need to find how to do 'with header if it exists' instead of
# writing two rules...
build/%.o: code/%.c code/%.h
	@mkdir -p build
	$(CC) -o $@ -c $< $(DEFINES)

build/%.o: code/%.c
	@mkdir -p build
	$(CC) -o $@ -c $< $(DEFINES)

build/tracker_torrent.o: build/tracker_common.o
build/tracker_web.o: build/tracker_common.o

dist/%: build/%.o ${OBJS}
	@mkdir -p dist
	$(CC) -o $@ $^
	@echo -e '\033[1;31mDone '$@'\033[0m...\n'

dist/tracker2: build/tracker_torrent.o
dist/tracker3: build/tracker_web.o
dist/tracker4: build/tracker_torrent.o build/tracker_web.o

clean:
	@rm -f build/*

cleanf: clean
	@rm -f dist/*

.SECONDARY: $(OBJS)
