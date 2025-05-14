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
    """Checks if script is run with admin rights"""
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except:
        return False


def run_as_admin():
    """Reloads the script if it is run without admin rights"""
    if not is_admin():
        print("Re-running script as administrator...")
        ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable, " " + " ".join(sys.argv), None, 1)
        sys.exit()


def check_and_install_vulkan_sdk():
    """Checks if Vulkan SDK is present and installs it on the system if it is not"""
    vulkan_sdk_path = os.getenv('VULKAN_SDK')
    if vulkan_sdk_path:
        print(f"Vulkan SDK is installed at: {vulkan_sdk_path}")
        return True
    else:
        print("Vulkan SDK is not installed.")
        install_vulkan_sdk()
        return False


def get_latest_vulkan_sdk_url():
    """Acquires a link to the latest Vulkan SDK"""
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
    """Downloands and installs Vulkan SDK"""
    vulkan_sdk_url = get_latest_vulkan_sdk_url()
    print(f"Downloading Vulkan SDK from {vulkan_sdk_url}...")

    temp_file = tempfile.NamedTemporaryFile(delete=False)
    temp_file.close()

    urllib.request.urlretrieve(vulkan_sdk_url, temp_file.name)

    print("Installing Vulkan SDK...")
    subprocess.run([temp_file.name], check=True)

    os.remove(temp_file.name)


def check_and_install_cmake():
    """Checks if CMake is present and installs it if it is not"""
    cmake_path = shutil.which("cmake")
    if cmake_path:
        print(f"CMake is installed at: {cmake_path}")
        return True
    else:
        print("CMake is not installed.")
        install_cmake()
        return False


def install_cmake():
    """Downloads and installs CMake"""
    cmake_url = "https://cmake.org/download/CMake-3.25.0-windows-x86_64.msi"
    temp_file = tempfile.NamedTemporaryFile(delete=False)
    temp_file.close()

    print("Downloading CMake...")
    urllib.request.urlretrieve(cmake_url, temp_file.name)

    print("Installing CMake...")
    subprocess.run(["msiexec", "/i", temp_file.name, "/quiet", "/norestart"], check=True)

    os.remove(temp_file.name)


def get_latest_clang_url():
    """Gets link to the latest release of Clang on GitHub"""
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
    """Downloads and validates the file"""
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
    """Downloads and installs Clang into the desired directory"""
    clang_url = get_latest_clang_url()
    print(f"Downloading Clang from {clang_url}...")

    # Download .exe installer of Clang
    temp_file = tempfile.NamedTemporaryFile(delete=False, suffix=".exe")
    temp_file.close()

    download_file(clang_url, temp_file)

    # Run and install in silent mode
    print("Running the Clang installer...")
    subprocess.run([temp_file.name, "/silent", "/install"], check=True)

    os.remove(temp_file.name)
    print("Clang installation complete.")

    # Now Clang must be installed, add it to the PATH
    clang_bin = os.path.join(install_dir, "bin")
    os.environ["PATH"] += os.pathsep + clang_bin
    print(f"Clang added to PATH: {clang_bin}")


def check_and_install_clang():
    """Checks if Clang is present in PATH system or in Program Files folder. Install it if it is not"""
    # Check if clang is present in PATH system
    clang_path = shutil.which("clang")
    if clang_path:
        print(f"Clang is installed and available in PATH at: {clang_path}")
        return True

    # Check if Clang is present in Program Files
    install_dir = os.path.join(os.environ["ProgramFiles"], "Clang")
    clang_bin = os.path.join(install_dir, "bin", "clang.exe")
    if os.path.isfile(clang_bin):
        print(f"Clang is installed at: {clang_bin}, but not in PATH.")
        # Add Clang to the PATH
        os.environ["PATH"] += os.pathsep + os.path.join(install_dir, "bin")
        print(f"Clang added to PATH: {os.path.join(install_dir, 'bin')}")
        return True

    # If clang is not found, install it
    print("Clang is not installed.")
    install_clang(install_dir)
    return False


def main():
    """Main logic"""
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
