# Executable target name
TARGET = goodbyedpi_fixed.exe

ifeq ($(BIT64),1)
    # 64-bit için tanımlamalar
    CFLAGS += -m64 -I../include
    LDFLAGS += -m64 -fstack-protector -Wl,-O1,--sort-common,--as-needed -Wl,--disable-auto-image-base
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
    CFLAGS += -m32 -I../include
    LDFLAGS += -m32 -fstack-protector -Wl,-O1,--sort-common,--as-needed -Wl,--disable-auto-image-base
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
# Windres için daha uyumlu tanım - yerel ortamda windres komutunu kullan
WINDRES = windres

.PHONY: default all clean ensure_dir clean_logs

default: ensure_dir clean_logs $(TARGET)
all: default

# Define source files excluding the resource file and goodbyedpi_simple.c
SOURCES = $(filter-out goodbyedpi_simple.c, $(wildcard *.c utils/*.c))
# Generate object files list from sources
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))
# Add resource object file separately
RESOURCE_OBJ = goodbyedpi-rc.o
HEADERS = $(wildcard *.h utils/*.h)

ensure_dir:
	@mkdir -p $(DIR)
	@mkdir -p $(DEST_DIR)

# Log dosyalarını temizleyen hedef - Turkce karakter kullanmadan
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
	@mkdir -p $(DEST_DIR)
ifeq ($(BIT64),1)
	$(LD) $(OBJECTS) $(RESOURCE_OBJ) $(LDFLAGS) -mwindows $(WINDIVERT_DIR)/WinDivert.lib -lws2_32 -lole32 -lshell32 -luuid -lcomctl32 -luxtheme -s -o $(DEST_DIR)/$(TARGET)
	@echo "Copying 64-bit WinDivert files..."
	@cp ../binary/amd64/WinDivert*.dll $(DEST_DIR)/ 2>/dev/null || true
	@cp ../binary/amd64/WinDivert*.sys $(DEST_DIR)/ 2>/dev/null || true
else
	$(LD) $(OBJECTS) $(RESOURCE_OBJ) $(LDFLAGS) -mwindows -L$(WINDIVERT_DIR) -lWinDivert -lws2_32 -lole32 -lshell32 -luuid -lcomctl32 -luxtheme -s -o $(DEST_DIR)/$(TARGET)
	@echo "Copying 32-bit WinDivert files..."
	@cp ../binary/x86/WinDivert*.dll $(DEST_DIR)/ 2>/dev/null || true
	@cp ../binary/x86/WinDivert*.sys $(DEST_DIR)/ 2>/dev/null || true
endif
	@echo "Executable built successfully at $(DEST_DIR)/$(TARGET)"

clean:
	-rm -f *.o utils/*.o
	-rm -f $(DIR)/$(TARGET)
	-rm -f $(DEST_DIR)/$(TARGET)
