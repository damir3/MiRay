from conan import ConanFile
from conan.tools.files import get, copy
from conan.errors import ConanInvalidConfiguration
import os

class OidnConan(ConanFile):
    name = "oidn"
    description = "Intel Open Image Denoise library"
    license = "Apache-2.0"
    url = "https://github.com/RenderKit/oidn"
    homepage = "https://www.openimagedenoise.org"
    topics = ("denoiser", "raytracing", "image", "neural-network")
    package_type = "shared-library"
    settings = "os", "arch", "compiler", "build_type"

    def validate(self):
        os_name = str(self.settings.os)
        arch_name = str(self.settings.arch)
        sources = self.conan_data.get("sources", {}).get(self.version, {})
        if os_name not in sources or arch_name not in sources[os_name]:
            raise ConanInvalidConfiguration(
                f"Prebuilt binary for oidn/{self.version} on {os_name} {arch_name} is not available"
            )

    def package_id(self):
        del self.info.settings.compiler
        del self.info.settings.build_type

    def build(self):
        os_name = str(self.settings.os)
        arch_name = str(self.settings.arch)
        source_data = self.conan_data["sources"][self.version][os_name][arch_name]
        get(self, **source_data, strip_root=True)

    def package(self):
        # Headers
        copy(self, "*", os.path.join(self.build_folder, "include"), os.path.join(self.package_folder, "include"))

        # Libraries & Binaries
        if self.settings.os == "Windows":
            copy(self, "*.lib", os.path.join(self.build_folder, "lib"), os.path.join(self.package_folder, "lib"))
            copy(self, "*.dll", os.path.join(self.build_folder, "bin"), os.path.join(self.package_folder, "bin"))
        elif self.settings.os == "Macos":
            copy(self, "*.dylib*", os.path.join(self.build_folder, "lib"), os.path.join(self.package_folder, "lib"))
        else:
            copy(self, "*.so*", os.path.join(self.build_folder, "lib"), os.path.join(self.package_folder, "lib"))

        # Licenses
        copy(self, "LICENSE*", os.path.join(self.build_folder, "doc"), os.path.join(self.package_folder, "licenses"))

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "OpenImageDenoise")
        self.cpp_info.set_property("cmake_target_name", "OpenImageDenoise")
        self.cpp_info.set_property("cmake_target_aliases", ["OpenImageDenoise::OpenImageDenoise"])

        if self.settings.os == "Windows":
            self.cpp_info.libs = ["OpenImageDenoise", "OpenImageDenoise_core"]
        else:
            self.cpp_info.libs = ["OpenImageDenoise"]
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs = ["m", "pthread", "dl"]
