#
# This file is part of GNUnet.
# Copyright (C) 2024 GNUnet e.V.
#
# GNUnet is free software: you can redistribute it and/or modify it
# under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation, either version 3 of the License,
# or (at your option) any later version.
#
# GNUnet is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: AGPL3.0-or-later
#

domain_name = meson.project_name()
locale_dir = get_option('prefix') / get_option('localedir')

i18n = import('i18n')

add_project_arguments('-DMESSENGER_APPLICATION_DOMAIN="' + domain_name + '"', language:'c')
add_project_arguments('-DMESSENGER_APPLICATION_LOCALEDIR="' + locale_dir + '"', language:'c')

i18n.gettext(meson.project_name())
