/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file rev.h declaration of OTTD revision dependent variables */

#ifndef REV_H
#define REV_H

extern "c" const char _openttd_revision[];
extern "c" const char _openttd_build_date[];
extern "c" const char _openttd_revision_hash[];
extern "c" const char _openttd_revision_year[];
extern "c" const uint8_t _openttd_revision_modified;
extern "c" const uint8_t _openttd_revision_tagged;
extern "c" const char _openttd_content_version[];
extern "c" const uint32_t _openttd_newgrf_version;

bool IsReleasedVersion();

#endif /* REV_H */
