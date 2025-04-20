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
endif

# Derleyici ve bağlayıcı için önekler
CC = $(CCPREFIX)gcc
LD = $(CC)
# Windres için direkt yolu belirt
CCWINDRES = $(if $(BIT64),C:/msys64/mingw64/bin/windres.exe,C:/msys64/mingw32/bin/windres.exe)

.PHONY: default all clean ensure_dir

default: ensure_dir $(TARGET) copy_windivert copy_to_32exe
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
	@mkdir -p 32_exe

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

goodbyedpi-rc.o: goodbyedpi-rc.rc icon.ico icon_running.ico icon_paused.ico
	$(CCWINDRES) $(WINDRES_ARCH) -I../include $< $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS) $(RESOURCE_OBJ)
	@mkdir -p $(DIR)
	$(LD) $(OBJECTS) $(RESOURCE_OBJ) $(LDFLAGS) -mwindows -L$(WINDIVERT_DIR) -lWinDivert -lws2_32 -lole32 -lshell32 -luuid -lcomctl32 -luxtheme -s -o $(DIR)/$(TARGET)

copy_windivert:
	@echo "Copying WinDivert.dll to $(DIR)"
	@cp $(WINDIVERT_DIR)/WinDivert.dll $(DIR)/

copy_to_32exe:
	@if [ "$(BIT64)" != "1" ]; then \
		echo "Copying 32-bit executable and DLLs to 32_exe directory"; \
		cp $(DIR)/$(TARGET) 32_exe/; \
		cp $(WINDIVERT_DIR)/WinDivert.dll 32_exe/; \
		if [ -f "$(DIR)/WinDivert32.sys" ]; then cp $(DIR)/WinDivert32.sys 32_exe/; fi; \
		if [ -f "$(DIR)/WinDivert64.sys" ]; then cp $(DIR)/WinDivert64.sys 32_exe/; fi; \
	fi

clean:
	-rm -f *.o utils/*.o
	-rm -f $(DIR)/$(TARGET)
	-rm -f 32_exe/$(TARGET)
