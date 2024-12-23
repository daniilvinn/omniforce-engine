import os
import subprocess
import platform
import urllib.request
import bs4  # BeautifulSoup for parsing HTML
import tempfile
import shutil
import sys

def check_and_install_vulkan_sdk():
    vulkan_sdk_path = os.getenv('VULKAN_SDK')
    if vulkan_sdk_path:
        print(f"Vulkan SDK is installed at: {vulkan_sdk_path}")
        return True
    else:
        print("Vulkan SDK is not installed.")
        install_vulkan_sdk()
        return False

def get_latest_vulkan_sdk_url():
    # Scrape the Vulkan SDK download page
    url = "https://vulkan.lunarg.com/sdk/home"
    print("Fetching latest Vulkan SDK version...")
    response = urllib.request.urlopen(url)
    html = response.read()
    soup = bs4.BeautifulSoup(html, "html.parser")

    # Search for the download link for the Windows installer
    download_link = None
    for a_tag in soup.find_all('a', href=True):
        if 'Windows' in a_tag['href'] and 'VulkanSDK' in a_tag['href']:
            download_link = a_tag['href']
            break

    if not download_link:
        print("Failed to find the download link for Vulkan SDK.")
        sys.exit(1)
    
    return "https://vulkan.lunarg.com" + download_link

def install_vulkan_sdk():
    # Get the latest Vulkan SDK download URL
    vulkan_sdk_url = get_latest_vulkan_sdk_url()
    print(f"Downloading Vulkan SDK from {vulkan_sdk_url}...")
    
    # Temporary file to download the SDK installer
    temp_file = tempfile.NamedTemporaryFile(delete=False)
    temp_file.close()

    # Download the Vulkan SDK installer
    urllib.request.urlretrieve(vulkan_sdk_url, temp_file.name)

    print("Installing Vulkan SDK...")
    subprocess.run([temp_file.name], check=True)

    # Clean up
    os.remove(temp_file.name)

def check_and_install_cmake():
    cmake_path = shutil.which("cmake")
    if cmake_path:
        print(f"CMake is installed at: {cmake_path}")
        return True
    else:
        print("CMake is not installed.")
        install_cmake()
        return False

def install_cmake():
    cmake_url = "https://cmake.org/download/CMake-3.25.0-windows-x86_64.msi"
    temp_file = tempfile.NamedTemporaryFile(delete=False)
    temp_file.close()

    print("Downloading CMake...")
    urllib.request.urlretrieve(cmake_url, temp_file.name)

    print("Installing CMake...")
    subprocess.run([temp_file.name], check=True)

    os.remove(temp_file.name)

def run_generate_projects():
    batch_file = os.path.join(os.getcwd(), "GenerateProjects.bat")
    if os.path.exists(batch_file):
        print(f"Running {batch_file}...")
        subprocess.run([batch_file], check=True)
    else:
        print("GenerateProjects.bat not found in the current directory.")
        sys.exit(1)

def main():
    print("Checking for Vulkan SDK and CMake...")

    vulkan_installed = check_and_install_vulkan_sdk()
    cmake_installed = check_and_install_cmake()

    if vulkan_installed and cmake_installed:
        run_generate_projects()
    else:
        print("Could not install necessary tools.")

if __name__ == "__main__":
    main()
