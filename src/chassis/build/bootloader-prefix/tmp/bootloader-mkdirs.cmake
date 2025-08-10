# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/issaimaru/esp/esp-idf/components/bootloader/subproject"
  "/home/issaimaru/esp/2025-SSR-Team5/Firmware/src/chassis/build/bootloader"
  "/home/issaimaru/esp/2025-SSR-Team5/Firmware/src/chassis/build/bootloader-prefix"
  "/home/issaimaru/esp/2025-SSR-Team5/Firmware/src/chassis/build/bootloader-prefix/tmp"
  "/home/issaimaru/esp/2025-SSR-Team5/Firmware/src/chassis/build/bootloader-prefix/src/bootloader-stamp"
  "/home/issaimaru/esp/2025-SSR-Team5/Firmware/src/chassis/build/bootloader-prefix/src"
  "/home/issaimaru/esp/2025-SSR-Team5/Firmware/src/chassis/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/issaimaru/esp/2025-SSR-Team5/Firmware/src/chassis/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/issaimaru/esp/2025-SSR-Team5/Firmware/src/chassis/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
