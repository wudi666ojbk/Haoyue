import subprocess
import sys
from importlib import metadata as importlib_metadata

def install(package):
    print(f"Installing {package} module...")
    subprocess.check_call(['python', '-m', 'pip', 'install', package])

def ValidatePackage(package):
    try:
        importlib_metadata.distribution(package)
    except importlib_metadata.PackageNotFoundError:
        install(package)

def ValidatePackages():
    ValidatePackage('requests')
    ValidatePackage('fake-useragent')
