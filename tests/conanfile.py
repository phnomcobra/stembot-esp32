from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps


class StembotTestsConan(ConanFile):
    name = "stembot-tests"
    version = "0.1.0"
    settings = "os", "compiler", "build_type", "arch"

    def requirements(self):
        self.requires("catch2/3.7.0")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()
