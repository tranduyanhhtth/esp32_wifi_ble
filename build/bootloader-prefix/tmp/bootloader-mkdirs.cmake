# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/HP/esp/v5.3/esp-idf/components/bootloader/subproject"
  "C:/HUST/2024.2/Nhap_esp/hello_world/build/bootloader"
  "C:/HUST/2024.2/Nhap_esp/hello_world/build/bootloader-prefix"
  "C:/HUST/2024.2/Nhap_esp/hello_world/build/bootloader-prefix/tmp"
  "C:/HUST/2024.2/Nhap_esp/hello_world/build/bootloader-prefix/src/bootloader-stamp"
  "C:/HUST/2024.2/Nhap_esp/hello_world/build/bootloader-prefix/src"
  "C:/HUST/2024.2/Nhap_esp/hello_world/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/HUST/2024.2/Nhap_esp/hello_world/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/HUST/2024.2/Nhap_esp/hello_world/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
