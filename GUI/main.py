import sys
import os
import pystray
from PIL import Image, ImageDraw
import subprocess
import winshell

# Global değişkenler
status = "paused"
selected_mode = "2_any_country_dnsredir.cmd"  
process = None

# PyInstaller çalışırken dosya yollarını düzelt
def resource_path(relative_path):
    """PyInstaller ile çalışırken dosya yollarını düzeltir ve EXE'ye gömülü dosyaları bulur."""
    if hasattr(sys, '_MEIPASS'):
        return os.path.join(sys._MEIPASS, relative_path)
    return os.path.join(os.path.abspath("."), relative_path)

# GoodbyeDPI klasörünün yolu
GOODBYE_DPI_PATH = resource_path("goodbyedpi-0.2.2")


commands = {
    "1_russia_blacklist_dnsredir.cmd": os.path.join(GOODBYE_DPI_PATH, "1_russia_blacklist_dnsredir.cmd"),
    "2_any_country_dnsredir.cmd": os.path.join(GOODBYE_DPI_PATH, "2_any_country_dnsredir.cmd"),
}

# İkon dosyaları
ICON_RUNNING = resource_path("icon_running.png")
ICON_PAUSED = resource_path("icon_paused.png")

# **Başlangıç klasörüne kısayol ekleyen fonksiyon**
def add_to_startup():
    """Programı Windows başlangıcına ekler."""
    startup_folder = winshell.startup()
    exe_path = os.path.abspath(sys.argv[0])  # EXE'nin tam yolu
    shortcut_path = os.path.join(startup_folder, "GoodbyeDPI.lnk")

    # Kısayol yoksa oluştur
    if not os.path.exists(shortcut_path):
        with winshell.shortcut(shortcut_path) as shortcut:
            shortcut.path = exe_path
            shortcut.description = "GoodbyeDPI GUI"
            shortcut.working_directory = os.path.dirname(exe_path)
            shortcut.icon_location = exe_path, 0
        print("GoodbyeDPI başlangıçta otomatik çalışacak.")

# **İkonu yükleme fonksiyonu**
def load_icon_image():
    """Duruma göre ikonları yükler."""
    icon_file = ICON_RUNNING if status == "running" else ICON_PAUSED
    if os.path.exists(icon_file):
        return Image.open(icon_file)
    return create_image()

# **Yedek ikon oluştur**
def create_image():
    """Eğer ikonlar bulunamazsa, yedek ikon oluşturur."""
    width, height = 64, 64
    color = (76, 175, 80) if status == "running" else (244, 67, 54)
    image = Image.new('RGB', (width, height), color)
    d = ImageDraw.Draw(image)
    d.ellipse((width//2-20, height//2-20, width//2+20, height//2+20), fill=(255, 255, 255))
    return image

# **Başlatma fonksiyonu**
def start(icon, item=None):
    global status, process
    if status != "running":
        cmd = commands[selected_mode]
        process = subprocess.Popen(cmd, shell=True, cwd=GOODBYE_DPI_PATH)
        status = "running"
        icon.icon = load_icon_image()
        icon.update_menu()

# **Durdurma fonksiyonu**
def pause(icon, item=None):
    global status, process
    if status == "running" and process:
        process.terminate()
        process = None
        status = "paused"
        icon.icon = load_icon_image()
        icon.update_menu()

# **Programı kapatma fonksiyonu**
def exit_program(icon, item=None):
    global process
    if process:
        process.terminate()
    icon.stop()

# **Mod değiştirme fonksiyonu**
def set_mode(icon, item):
    global selected_mode
    selected_mode = item.text
    icon.update_menu()

# **Modlar alt menüsü**
modes_submenu = pystray.Menu(
    pystray.MenuItem('1_russia_blacklist_dnsredir.cmd', set_mode, checked=lambda item: selected_mode == "1_russia_blacklist_dnsredir.cmd"),
    pystray.MenuItem('2_any_country_dnsredir.cmd', set_mode, checked=lambda item: selected_mode == "2_any_country_dnsredir.cmd"),
)

# **Sistem tepsisi (Tray) menüsü**
icon = pystray.Icon("GoodbyeDPI", load_icon_image(), "GoodbyeDPI", menu=pystray.Menu(
    pystray.MenuItem('Start', lambda item: start(icon, item)),
    pystray.MenuItem('Pause', lambda item: pause(icon, item)),
    pystray.Menu.SEPARATOR,
    pystray.MenuItem('Modes', modes_submenu),
    pystray.Menu.SEPARATOR,
    pystray.MenuItem('Exit', lambda item: exit_program(icon, item))
))

# **Otomatik başlatmayı ayarla**
add_to_startup()

# **Simge tepsisinde çalıştır**
icon.run()
