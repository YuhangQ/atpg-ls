# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/pub/netdisk1/qianyh/atpg-ls/tg-pro/cadical06w/src/cadical06w"
  "/pub/netdisk1/qianyh/atpg-ls/tg-pro/cadical06w/src/cadical06w-build"
  "/pub/netdisk1/qianyh/atpg-ls/tg-pro/cadical06w"
  "/pub/netdisk1/qianyh/atpg-ls/tg-pro/cadical06w/tmp"
  "/pub/netdisk1/qianyh/atpg-ls/tg-pro/cadical06w/src/cadical06w-stamp"
  "/pub/netdisk1/qianyh/atpg-ls/tg-pro/cadical06w/src"
  "/pub/netdisk1/qianyh/atpg-ls/tg-pro/cadical06w/src/cadical06w-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/pub/netdisk1/qianyh/atpg-ls/tg-pro/cadical06w/src/cadical06w-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/pub/netdisk1/qianyh/atpg-ls/tg-pro/cadical06w/src/cadical06w-stamp${cfgdir}") # cfgdir has leading slash
endif()
