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


find_package(KDE4 REQUIRED)
include_directories( ${KDE4_INCLUDES} )

set(SCATE_SOURCES src/scate.cpp )
kde4_add_plugin(katescateplugin ${SCATE_SOURCES})
target_link_libraries(katescateplugin  ${KDE4_KDEUI_LIBS} kateinterfaces )

########### install files ###############
install(TARGETS katescateplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES share/ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katescate )
install( FILES share/supercollider.png  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katescate )
install( FILES share/supercollider.xml DESTINATION ${DATA_INSTALL_DIR}/katepart/syntax )
install( FILES share/scate.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
