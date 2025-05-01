# Executable target name
TARGET = goodbyedpi.exe

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
# Windres için doğrudan sabit yol tanımlanıyor
ifeq ($(BIT64),1)
    WINDRES = C:/msys64/mingw64/bin/windres.exe
else
    WINDRES = C:/msys64/mingw32/bin/windres.exe
endif

.PHONY: default all clean ensure_dir clean_logs

default: ensure_dir clean_logs $(TARGET) copy_to_dest
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

# Her derleme işleminde log dosyalarını temizleyen hedef
clean_logs:
	@echo "Cleaning log files for build..."
	@echo "=== Log başlatıldı - $(shell date '+%d-%m-%Y') ===" > $(DEST_DIR)/goodbyedpi_log.txt
	@echo "=== Hata logu başlatıldı - $(shell date '+%d-%m-%Y') ===" > $(DEST_DIR)/goodbyedpi_error.log

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

goodbyedpi-rc.o: goodbyedpi-rc.rc icon.ico icon_running.ico icon_paused.ico
	$(WINDRES) $(WINDRES_ARCH) -I../include $< $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS) $(RESOURCE_OBJ)
	@mkdir -p $(DIR)
	$(LD) $(OBJECTS) $(RESOURCE_OBJ) $(LDFLAGS) -mwindows -L$(WINDIVERT_DIR) -lWinDivert -lws2_32 -lole32 -lshell32 -luuid -lcomctl32 -luxtheme -s -o $(DIR)/$(TARGET)

# Sadece exe dosyasını ilgili klasöre kopyala
copy_to_dest:
	@echo "Copying executable to $(DEST_DIR) directory"
	@cp $(DIR)/$(TARGET) $(DEST_DIR)/

clean:
	-rm -f *.o utils/*.o
	-rm -f $(DIR)/$(TARGET)
	-rm -f $(DEST_DIR)/$(TARGET)
