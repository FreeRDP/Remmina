# desktop/remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
# Copyright (C) 2016-2019 Antenore Gatta, Giovanni Panozzo
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA  02110-1301, USA.
#
# In addition, as a special exception, the copyright holders give
# permission to link the code of portions of this program with the
# OpenSSL library under certain conditions as described in each
# individual source file, and distribute linked combinations
# including the two.
# You must obey the GNU General Public License in all respects
# for all of the code used other than OpenSSL. If you modify
# file(s) with this exception, you may extend this exception to your
# version of the file(s), but you are not obligated to do so. If you
# do not wish to do so, delete this exception statement from your
# version. If you delete this exception statement from all source
# files in the program, then also delete it here.


install(FILES remmina_about.glade DESTINATION "${REMMINA_UIDIR}")
install(FILES remmina_main.glade DESTINATION "${REMMINA_UIDIR}")
install(FILES remmina_news.glade DESTINATION "${REMMINA_UIDIR}")
install(FILES remmina_spinner.glade DESTINATION "${REMMINA_UIDIR}")
install(FILES remmina_preferences.glade DESTINATION "${REMMINA_UIDIR}")
install(FILES remmina_key_chooser.glade DESTINATION "${REMMINA_UIDIR}")
install(FILES remmina_string_list.glade DESTINATION "${REMMINA_UIDIR}")
install(FILES remmina_mpc.glade DESTINATION "${REMMINA_UIDIR}")
install(FILES remmina_snap_info_dialog.glade DESTINATION "${REMMINA_UIDIR}")
install(FILES remmina_unlock.glade DESTINATION "${REMMINA_UIDIR}")
install(FILES remmina_xinput_env_dialog.glade DESTINATION "${REMMINA_UIDIR}")
