/* GstSwitchSrv
 * Copyright (C) 2012 Duzy Chan <code@duzy.info>
 *
 * This file is part of gst-switch.
 *
 * gst-switch is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LOG_UTILS_H__
#define DEBUG 1
#define ENABLE_LOW_RESOLUTION 1
#define ENABLE_ASSESSMENT 0
#if DEBUG
#define INFO_PREFIX LOG_PREFIX"/%s:%d:info:"
#define WARN_PREFIX LOG_PREFIX"/%s:%d:warning:"
#define ERROR_PREFIX LOG_PREFIX"/%s:%d:error:"
#define INFO(S, ...) g_print (INFO_PREFIX" "S"\n", __FILE__, __LINE__, ## __VA_ARGS__)
#define WARN(S, ...) g_print (WARN_PREFIX" "S"\n", __FILE__, __LINE__, ## __VA_ARGS__)
#define ERROR(S, ...) g_print (ERROR_PREFIX" "S"\n", __FILE__, __LINE__, ## __VA_ARGS__)
#else
#define INFO(S, ...) ((void) FALSE)
#define WARN(S, ...) ((void) FALSE)
#define ERROR(S, ...) ((void) FALSE)
#endif //DEBUG
#if ENABLE_ASSESSMENT
extern guint assess_number;
#define ASSESS(name, ...) (g_string_append_printf (desc, "! assess n=%d name="#name " ", assess_number++, ##__VA_ARGS__))
#else
#define ASSESS(name, ...) ((void) FALSE);
#endif //ENABLE_ASSESSMENT
#define LOW_RES_W 300 /* 100 */ /* 160 */       /* 320 */
#define LOW_RES_H 200 /* 56 */ /* 120 */        /* 240 */
#endif //__LOG_UTILS_H__
