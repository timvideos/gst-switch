/* GstSwitchSrv
 * Copyright (C) 2012 Duzy Chan <code@duzy.info>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
