import sys
import os

project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "../.."))
bindings_dir = os.path.join(project_root, ".nix-dev/build/src/bindings")

if os.path.exists(bindings_dir):
    sys.path.insert(0, bindings_dir)
else:
    print(f"Bindings path not found at {bindings_dir}!")
