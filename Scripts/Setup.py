import os
import subprocess
import platform
import urllib.request
import json
import tempfile
import shutil
import sys
import ctypes
import requests


def is_admin():
    """Проверяет, запущен ли скрипт от имени администратора."""
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except:
        return False


def run_as_admin():
    """Перезапускает скрипт с правами администратора, если они отсутствуют."""
    if not is_admin():
        print("Re-running script as administrator...")
        ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable, " " + " ".join(sys.argv), None, 1)
        sys.exit()


def check_and_install_vulkan_sdk():
    """Проверяет наличие Vulkan SDK и устанавливает его при необходимости."""
    vulkan_sdk_path = os.getenv('VULKAN_SDK')
    if vulkan_sdk_path:
        print(f"Vulkan SDK is installed at: {vulkan_sdk_path}")
        return True
    else:
        print("Vulkan SDK is not installed.")
        install_vulkan_sdk()
        return False


def get_latest_vulkan_sdk_url():
    """Получает ссылку на последнюю версию Vulkan SDK."""
    url = "https://vulkan.lunarg.com/sdk/home"
    print("Fetching latest Vulkan SDK version...")
    response = urllib.request.urlopen(url)
    html = response.read().decode('utf-8')

    download_link = None
    start_index = html.find("https://sdk.lunarg.com/sdk/home")
    if start_index != -1:
        end_index = html.find("\"", start_index)
        download_link = html[start_index:end_index]

    if not download_link:
        print("Failed to find the download link for Vulkan SDK.")
        sys.exit(1)

    return download_link


def install_vulkan_sdk():
    """Скачивает и устанавливает Vulkan SDK."""
    vulkan_sdk_url = get_latest_vulkan_sdk_url()
    print(f"Downloading Vulkan SDK from {vulkan_sdk_url}...")

    temp_file = tempfile.NamedTemporaryFile(delete=False)
    temp_file.close()

    urllib.request.urlretrieve(vulkan_sdk_url, temp_file.name)

    print("Installing Vulkan SDK...")
    subprocess.run([temp_file.name], check=True)

    os.remove(temp_file.name)


def check_and_install_cmake():
    """Проверяет наличие CMake и устанавливает его при необходимости."""
    cmake_path = shutil.which("cmake")
    if cmake_path:
        print(f"CMake is installed at: {cmake_path}")
        return True
    else:
        print("CMake is not installed.")
        install_cmake()
        return False


def install_cmake():
    """Скачивает и устанавливает CMake."""
    cmake_url = "https://cmake.org/download/CMake-3.25.0-windows-x86_64.msi"
    temp_file = tempfile.NamedTemporaryFile(delete=False)
    temp_file.close()

    print("Downloading CMake...")
    urllib.request.urlretrieve(cmake_url, temp_file.name)

    print("Installing CMake...")
    subprocess.run(["msiexec", "/i", temp_file.name, "/quiet", "/norestart"], check=True)

    os.remove(temp_file.name)


def get_latest_clang_url():
    """Получает ссылку на последнюю версию Clang с GitHub."""
    try:
        api_url = "https://api.github.com/repos/llvm/llvm-project/releases/latest"
        print("Fetching latest Clang version using GitHub API...")

        response = requests.get(api_url)
        response.raise_for_status()  # This will raise an error for a bad HTTP response

        data = response.json()
        for asset in data.get("assets", []):
            if "win64.exe" in asset["name"]:  # Ensure the name matches a .exe file for Windows
                return asset["browser_download_url"]

        print("No Clang installer found in the release assets.")
        sys.exit(1)

    except requests.RequestException as e:
        print(f"Error fetching Clang release: {e}")
        sys.exit(1)


def download_file(url, temp_file):
    """Скачивает файл и проверяет его корректность."""
    try:
        print(f"Downloading file from {url}...")
        with requests.get(url, stream=True) as r:
            r.raise_for_status()
            with open(temp_file.name, 'wb') as f:
                for chunk in r.iter_content(chunk_size=8192):
                    f.write(chunk)

        print(f"Downloaded to {temp_file.name}")
        return temp_file.name
    except requests.RequestException as e:
        print(f"Error downloading file: {e}")
        sys.exit(1)


def install_clang(install_dir):
    """Скачивает и устанавливает Clang в указанную директорию."""
    clang_url = get_latest_clang_url()
    print(f"Downloading Clang from {clang_url}...")

    # Скачиваем .exe установщик Clang
    temp_file = tempfile.NamedTemporaryFile(delete=False, suffix=".exe")
    temp_file.close()

    download_file(clang_url, temp_file)

    # Запускаем установщик в тихом режиме
    print("Running the Clang installer...")
    subprocess.run([temp_file.name, "/silent", "/install"], check=True)

    os.remove(temp_file.name)
    print("Clang installation complete.")

    # Теперь Clang должен быть установлен, добавим его в PATH
    clang_bin = os.path.join(install_dir, "bin")
    os.environ["PATH"] += os.pathsep + clang_bin
    print(f"Clang added to PATH: {clang_bin}")


def check_and_install_clang():
    """Проверяет наличие Clang в PATH или Program Files, при необходимости устанавливает."""
    # Проверяем наличие Clang в PATH
    clang_path = shutil.which("clang")
    if clang_path:
        print(f"Clang is installed and available in PATH at: {clang_path}")
        return True

    # Проверяем наличие Clang в Program Files
    install_dir = os.path.join(os.environ["ProgramFiles"], "Clang")
    clang_bin = os.path.join(install_dir, "bin", "clang.exe")
    if os.path.isfile(clang_bin):
        print(f"Clang is installed at: {clang_bin}, but not in PATH.")
        # Добавляем Clang в PATH
        os.environ["PATH"] += os.pathsep + os.path.join(install_dir, "bin")
        print(f"Clang added to PATH: {os.path.join(install_dir, 'bin')}")
        return True

    # Если Clang нигде не найден, устанавливаем его
    print("Clang is not installed.")
    install_clang(install_dir)
    return False


def main():
    """Основная логика скрипта."""
    run_as_admin()
    print("Checking for Vulkan SDK, CMake, and Clang...")

    vulkan_installed = check_and_install_vulkan_sdk()
    cmake_installed = check_and_install_cmake()
    clang_installed = check_and_install_clang()

    if vulkan_installed and cmake_installed and clang_installed:
        print("Successfully installed all dependencies.")
    else:
        print("Some dependencies could not be installed.")


if __name__ == "__main__":
    main()
    input("Press any key to continue")
