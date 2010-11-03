#
# Copyright 2010 Jakob Leben (jakob.leben@gmail.com)
#
# This file is part of Scate - a SuperCollider plugin for Kate
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

cmake_minimum_required(VERSION 2.6)

set( CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/cmake-modules
  ${CMAKE_MODULE_PATH}
)

find_package( KDE4 REQUIRED )
include_directories( ${KDE4_INCLUDES} )

find_package( KateSDK )

if( NOT KATE_SDK_FOUND )

  message( STATUS "Missing Kate development files; building with project-provided" )

  find_file( LIB_KATE_INTERFACES_4 libkateinterfaces.so.4 PATHS ${CMAKE_SYSTEM_LIBRARY_PATH} )
  if( NOT LIB_KATE_INTERFACES_4 )
    message( FATAL_ERROR "Missing: libkateinterfaces.so.4")
  endif( NOT LIB_KATE_INTERFACES_4 )

  message( STATUS "Found Kate Interfaces 4.x library : ${LIB_KATE_INTERFACES_4}" )

  add_custom_target( ln_kateinterfaces
    COMMAND mkdir -p kate/lib
    COMMAND ln -sfv ${LIB_KATE_INTERFACES_4} kate/lib/libkateinterfaces.so
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )

  include_directories( ${CMAKE_SOURCE_DIR}/kate/include/ )
  link_directories( ${CMAKE_BINARY_DIR}/kate/lib/ )

endif( NOT KATE_SDK_FOUND )

set( QT_USE_QTWEBKIT TRUE )

set( SCATE_SOURCES
  src/ScatePlugin.cpp
  src/ScateView.cpp
  src/ScateConfigPage.cpp
  src/ScateHelpBrowser.cpp
)

kde4_add_plugin( katescateplugin ${SCATE_SOURCES} )
target_link_libraries( katescateplugin QtWebKit ${KDE4_KDEUI_LIBS} kateinterfaces ktexteditor kparts )

if( NOT KATE_SDK_FOUND )
  add_dependencies( katescateplugin ln_kateinterfaces )
endif( NOT KATE_SDK_FOUND )

########### install files ###############
install( TARGETS katescateplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES share/ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katescate )
install( FILES share/supercollider.png  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katescate )
install( FILES share/supercollider.xml DESTINATION ${DATA_INSTALL_DIR}/katepart/syntax )
install( FILES share/scate.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
