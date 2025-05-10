# Executable target name
TARGET = goodbyedpi.exe

ifeq ($(BIT64),1)
    # 64-bit için tanımlamalar
    CFLAGS += -m64 -I../include -std=c99 -pie -fPIE -pipe -O2 -D_FORTIFY_SOURCE=2 \
              -Wall -Wextra -Wpedantic -Wformat=2 -Wformat-security -Wshadow
    LDFLAGS += -m64 -fstack-protector -Wl,-O1,--sort-common,--as-needed -Wl,--disable-auto-image-base \
               -Wl,--dynamicbase,--nxcompat -Wl,--high-entropy-va -Wl,--pic-executable,-e,mainCRTStartup
    CCPREFIX = x86_64-w64-mingw32-
    CPREFIX = x86_64-w64-mingw32-
    DIR = x86_64
    # 64-bit için doğru WinDivert yolunu kullan
    WINDIVERT_DIR = ../binary/amd64
    # Windres için mimari seçeneği
    WINDRES_ARCH = --target=pe-x86-64
    DEST_DIR = 64_exe
else
    # 32-bit için tanımlamalar
    CFLAGS += -m32 -I../include -std=c99 -pie -fPIE -pipe -O2 -D_FORTIFY_SOURCE=2 \
              -Wall -Wextra -Wpedantic -Wformat=2 -Wformat-security -Wshadow
    LDFLAGS += -m32 -fstack-protector -Wl,-O1,--sort-common,--as-needed -Wl,--disable-auto-image-base \
               -Wl,--dynamicbase,--nxcompat -Wl,--pic-executable,-e,_mainCRTStartup
    CCPREFIX = i686-w64-mingw32-
    CPREFIX = i686-w64-mingw32-
    DIR = x86
    # 32-bit için doğru WinDivert yolunu kullan
    WINDIVERT_DIR = ../binary/x86
    # Windres için mimari seçeneği
    WINDRES_ARCH = --target=pe-i386
    DEST_DIR = 32_exe
endif

# Derleyici ve bağlayıcı için önekler
CC = $(CCPREFIX)gcc
LD = $(CC)
# Windres için daha uyumlu tanım
WINDRES = $(CPREFIX)windres
# If CPREFIX windres is not available, fall back to plain windres
ifeq (, $(shell which $(CPREFIX)windres))
	WINDRES = windres
endif

# Add libraries
LIBS = -L$(WINDIVERT_DIR) -lWinDivert -lws2_32 -lole32 -lshell32 -luuid -lcomctl32 -luxtheme

.PHONY: default all clean ensure_dir clean_logs

default: ensure_dir clean_logs $(TARGET)
all: default

# Define source files excluding the resource file
SOURCES = $(wildcard *.c utils/*.c)
# Generate object files list from sources
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))
# Add resource object file separately
RESOURCE_OBJ = goodbyedpi-rc.o
HEADERS = $(wildcard *.h utils/*.h)

ensure_dir:
	@mkdir -p $(DIR)
	@mkdir -p $(DEST_DIR)

# Log dosyalarını temizleyen hedef
clean_logs:
	@echo "Cleaning log files for build..."
	@echo "=== Log started - $(shell date '+%d-%m-%Y') ===" > $(DEST_DIR)/goodbyedpi_log.txt
	@echo "=== Error log started - $(shell date '+%d-%m-%Y') ===" > $(DEST_DIR)/goodbyedpi_error.log

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

goodbyedpi-rc.o: goodbyedpi-rc.rc icon.ico icon_running.ico icon_paused.ico
	$(WINDRES) $(WINDRES_ARCH) -I../include $< $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS) $(RESOURCE_OBJ)
	@mkdir -p $(DIR)
	$(LD) $(OBJECTS) $(RESOURCE_OBJ) $(LDFLAGS) -mwindows $(LIBS) -s -o $(DEST_DIR)/$(TARGET)
	@echo "Executable built successfully at $(DEST_DIR)/$(TARGET)"
	@echo "Build completed with flags: $(CFLAGS)"

clean:
	-rm -f *.o utils/*.o
	-rm -f $(DIR)/$(TARGET)
	-rm -f $(DEST_DIR)/$(TARGET)
