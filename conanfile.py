import os, json, uuid
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, CMakeDeps
from conan.tools.system import package_manager
from conan.tools.files import copy

# DO NOT use cmake_layout(self) HERE!
# ------------------------------------------------- --
    # DotNameCpp is using self layout               --
    # to define build ouput layout!                 --
    # ├── Build                                     --
    #     ├── Artefacts - tarballs of installation  --
    #     ├── Install - final installation          --
    #     ├── Library - library build               --
    #     └── Standalone - standalone build         --
# ------------------------------------------------- --

class DotNameCppRecipe(ConanFile):
    name = "ibot"
    version = "1.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    def imports(self):
        self.copy("license*", dst="licenses", folder=True, ignore_case=True)

    def generate(self): 
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_BUILD_TYPE"] = str(self.settings.build_type)
        tc.generate()

        self.dotnameintegrated_update_cmake_presets()
        self.dotnameintegrated_patch_remove_stdcpp_from_system_libs()        

    # Consuming recipe
    def configure(self):
        self.options["*"].shared = False # this replaced shared flag from SolutionController.py and works

    def requirements(self):
        # self.requires("gtest/1.16.0")           # Google Test (if CPM not used)
        self.requires("fmt/[11.2.0]") # required by cpm package
        self.requires("zlib/[~1.3]")
        self.requires("nlohmann_json/[~3.11]")
        self.requires("opus/1.5.2")
        self.requires("openssl/3.4.1")
        self.requires("libcurl/8.12.1")
        self.requires("tinyxml2/11.0.0")
        
    # def build_requirements(self):
    #     self.tool_requires("cmake/[>3.14]")



    # ---------------------------------------------------------------------
    # Utility Functions - no need to change
    # ---------------------------------------------------------------------

    def dotnameintegrated_update_cmake_presets(self):
        """Dynamic change of names in CMakePresets.json to avoid name conflicts"""
        preset_file = "CMakePresets.json"
        if not os.path.exists(preset_file):
            return
            
        try:
            with open(preset_file, "r", encoding="utf-8") as f:
                data = json.load(f)
                
            preset_name = (f"{str(self.settings.build_type).lower()}-"
                          f"{str(self.settings.os).lower()}-"
                          f"{self.settings.arch}-"
                          f"{self.settings.compiler}-"
                          f"{self.settings.compiler.version}")
            
            # Collect old names from configurePresets for mapping
            name_mapping = {}
            for preset in data.get("configurePresets", []):
                old_name = preset["name"]
                preset["name"] = preset["displayName"] = preset_name
                name_mapping[old_name] = preset_name
                
            # Update buildPresets and testPresets in one pass
            for preset_type in ["buildPresets", "testPresets"]:
                for preset in data.get(preset_type, []):
                    if preset.get("configurePreset") in name_mapping:
                        preset["name"] = preset["configurePreset"] = preset_name
                        
            with open(preset_file, "w", encoding="utf-8") as f:
                json.dump(data, f, indent=4)
                
        except (json.JSONDecodeError, IOError) as e:
            self.output.warn(f"Failed to update CMake presets: {e}")

    def dotnameintegrated_patch_remove_stdcpp_from_system_libs(self):
        """Remove stdc++ from SYSTEM_LIBS in generated Conan CMake files"""
        import glob
        import re
        
        # Find all *-data.cmake files for all platforms and architectures
        generators_path = self.generators_folder or "."
        patterns = [
            "*-data.cmake",          # General pattern for all files
            "*-*-*-data.cmake",      # Pattern for specific platforms (name-os-arch-data.cmake)
        ]
        
        cmake_files = []
        for pattern in patterns:
            cmake_files.extend(glob.glob(os.path.join(generators_path, pattern)))
            
        # Remove duplicates
        cmake_files = list(set(cmake_files))
        
        if not cmake_files:
            return
            
        # Compile regex pattern once for better performance
        system_libs_pattern = re.compile(
            r'(set\([^_]*_SYSTEM_LIBS(?:_[A-Z]+)?\s+[^)]*?)stdc\+\+([^)]*\))', 
            re.MULTILINE
        )
        
        patched_count = 0
        for cmake_file in cmake_files:
            try:
                with open(cmake_file, 'r', encoding='utf-8') as f:
                    content = f.read()
                
                # Replace all occurrences of stdc++ in SYSTEM_LIBS
                modified_content = system_libs_pattern.sub(r'\1\2', content)
                
                if modified_content != content:
                    with open(cmake_file, 'w', encoding='utf-8') as f:
                        f.write(modified_content)
                    self.output.info(f"Patched {cmake_file} - removed stdc++ from SYSTEM_LIBS")
                    patched_count += 1
                    
            except (IOError, OSError) as e:
                self.output.warn(f"Could not patch {cmake_file}: {e}")
        
        if patched_count > 0:
            self.output.info(f"Successfully patched {patched_count} CMake files")