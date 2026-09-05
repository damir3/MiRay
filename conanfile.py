from conan import ConanFile
from conan.tools.files import copy
from conan.tools.microsoft import is_msvc
import os

class MiRayRecipe(ConanFile):
    name = "miray"
    version = "2.0.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("gtest/1.15.0")
        self.requires("qt/5.15.18")
        self.requires("glm/1.0.1")
        self.requires("freeimage/3.18.0")
        self.requires("assimp/6.0.5")
        self.requires("embree/4.4.0")
        self.requires("oidn/2.5.0")
        self.requires("zlib/1.3.1", override=True)

        if self.settings.os == "Windows":
            self.requires("openssl/1.1.1w")

    def configure(self):
        self.options["freeimage"].shared = False
        self.options["hwloc"].shared = True

        # Qt configuration
        self.options["qt"].shared = True
        self.options["qt"].with_mysql = False
        self.options["qt"].with_pq = False
        self.options["qt"].with_odbc = False
        self.options["qt"].qtdeclarative = True
        self.options["qt"].qtquickcontrols = True
        self.options["qt"].qtquickcontrols2 = True
        self.options["qt"].qtgraphicaleffects = True
        self.options["qt"].qtsvg = True
        self.options["qt"].qttools = True

        # fine-tune embree features
        self.options["embree"].shared = True
        self.options["embree"].geometry_curve = False
        self.options["embree"].geometry_grid = False
        self.options["embree"].geometry_quad = False
        self.options["embree"].geometry_subdivision = False
        self.options["embree"].ray_packets = False
        self.options["embree"].sse2 = True
        self.options["embree"].sse42 = True
        self.options["embree"].avx = True
        self.options["embree"].avx2 = True
        self.options["embree"].avx512 = not is_msvc(self)
        self.options["embree"].neon = True
        self.options["embree"].neon2x = True

    def layout(self):
        target_folder = os.environ.get("OVERRIDE_BUILD_FOLDER", "tmp")
        self.folders.generators = target_folder
        self.folders.build = target_folder

    def generate(self):
        if self.settings.os == "Windows":
            arch = "arm64" if str(self.settings.arch) in ["armv8", "arm64"] else "64"
        else:
            arch = "arm" if str(self.settings.arch) in ["armv8", "arm64"] else "64"
        bin_dir = os.path.join(self.recipe_folder, "bin", arch)

        # qt QML
        qt_dir = os.path.join(self.dependencies["qt"].package_folder)
        copy(self, "*", os.path.join(qt_dir, "qml", "QtQuick"), os.path.join(bin_dir, "qml", "QtQuick"))
        copy(self, "*", os.path.join(qt_dir, "qml", "QtQuick.2"), os.path.join(bin_dir, "qml", "QtQuick.2"))
        copy(self, "*", os.path.join(qt_dir, "qml", "QtGraphicalEffects"), os.path.join(bin_dir, "qml", "QtGraphicalEffects"))

        if self.settings.os == "Windows":
            copy(self, "*", os.path.join(qt_dir, "plugins", "platforms"), os.path.join(bin_dir, "platforms"))
            copy(self, "*", os.path.join(qt_dir, "plugins", "imageformats"), os.path.join(bin_dir, "imageformats"))
            copy(self, "*", os.path.join(qt_dir, "plugins", "styles"), os.path.join(bin_dir, "styles"))
            qt_files = [
                "Qt5Core", "Qt5Gui", "Qt5Network", "Qt5Widgets", "Qt5OpenGL",
                "libEGL", "libGLESv2", "Qt5Quick", "Qt5QuickWidgets", "Qt5Qml",
                "Qt5QmlModels", "Qt5QmlWorkerScript", "Qt5QuickControls2",
                "Qt5QuickTemplates2", "Qt5Xml", "Qt5Concurrent", "Qt5Svg"
            ]
            dlls_dir = os.path.join(qt_dir, "bin")
            for f in qt_files:
                copy(self, f"*{f}*.dll", dlls_dir, bin_dir)

            modules = ["openssl", "embree", "oidn"]
            for m in modules:
                if m in self.dependencies:
                    module_bin = os.path.join(self.dependencies[m].package_folder, "bin")
                    if os.path.exists(module_bin):
                        copy(self, "*.dll", module_bin, bin_dir)
