#
# This file is part of GNUnet.
# Copyright (C) 2023--2024 GNUnet e.V.
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

project(
    'messenger-gtk',
    'c',
    license: 'AGPL3.0-or-later',
    version: run_command('contrib/get_version.sh').stdout().strip(),
)

messenger_gtk_id = 'org.gnunet.Messenger'

gnome = import('gnome')

src_resources = include_directories('src', 'resources')

subdir('submodules')

messenger_gtk_deps = [
    dependency('gnunetchat'),
    dependency('gnunetutil'),
    dependency('glib-2.0'),
    dependency('gtk+-3.0'),
    dependency('libhandy-1'),
    dependency('gstreamer-1.0'),
    dependency('libportal'),
    dependency('libportal-gtk3'),
    dependency('libnotify'),
    dependency('libqrencode'),
    declare_dependency(link_args: '-lunistring'),
]

subdir('resources')
subdir('src')

messenger_gtk_exec = executable(
    meson.project_name(),
    messenger_gtk_resources + messenger_gtk_sources,
    c_args : [
        '-DMESSENGER_APPLICATION_BINARY="@0@"'.format(meson.project_name()),
        '-DMESSENGER_APPLICATION_ID="@0@"'.format(messenger_gtk_id),
        '-DMESSENGER_APPLICATION_VERSION="@0@"'.format(meson.project_version()),
    ],
    install: true,
    dependencies: messenger_gtk_deps,
    extra_files: submodules_headers,
    include_directories: [
        src_resources, 
        submodules_includes,
    ],
)

gnome.post_install(
    gtk_update_icon_cache: true,
    update_desktop_database: true,
)
