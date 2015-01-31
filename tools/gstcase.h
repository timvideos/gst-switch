/* GstSwitch							    -*- c -*-
 * Copyright (C) 2012,2013 Duzy Chan <code@duzy.info>
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

/*! @file */

#ifndef __GST_CASE_H__
#define __GST_CASE_H__

#include "gstworker.h"
#include <gio/gio.h>

#define GST_TYPE_CASE (gst_case_get_type ())
#define GST_CASE(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GST_TYPE_CASE, GstCase))
#define GST_CASE_CLASS(class) (G_TYPE_CHECK_CLASS_CAST ((class), GST_TYPE_CASE, GstCaseClass))
#define GST_IS_CASE(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), GST_TYPE_CASE))
#define GST_IS_CASE_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class), GST_TYPE_CASE))

typedef struct _GstCase GstCase;
typedef struct _GstCaseClass GstCaseClass;

/**
 *  @brief The type of GstCase.
 */
typedef enum
{
  GST_CASE_UNKNOWN,             /*!< unknown case */
  GST_CASE_COMPOSITE_VIDEO_A,   /*!< special case for composite channel A */
  GST_CASE_COMPOSITE_VIDEO_B,   /*!< special case for composite channel B */
  GST_CASE_COMPOSITE_AUDIO,     /*!< special case for composite channel audio */
  GST_CASE_PREVIEW,             /*!< special case for previews */
  GST_CASE_INPUT_AUDIO,         /*!< Audio input from TCP socket */
  GST_CASE_INPUT_VIDEO,         /*!< Video input from TCP socket */
  GST_CASE_BRANCH_VIDEO_A,      /*!< special case for branching channel A to output */
  GST_CASE_BRANCH_VIDEO_B,      /*!< special case for branching channel B to output */
  GST_CASE_BRANCH_AUDIO,        /*!< special case for branching active audio to output */
  GST_CASE_BRANCH_PREVIEW,      /*!< special case for branching preview to output */
  GST_CASE__LAST_TYPE = GST_CASE_BRANCH_PREVIEW
} GstCaseType;

/**
 *  @brief Stream type in GstSwitch.
 */
typedef enum
{
  /**
   * @brief The case is serving nothing.
   */
  GST_SERVE_NOTHING,

  /**
   * @brief The case is serving video stream.
   */
  GST_SERVE_VIDEO_STREAM,

  /**
   * @brief The case is serving audio stream.
   */
  GST_SERVE_AUDIO_STREAM,
} GstSwitchServeStreamType;

/**
 *  @class GstCase
 *  @struct _GstCase
 *  @brief Special purpose of workers for switching streams.
 */
typedef struct _GstCase
{
  GstWorker base;               /*!< The parent object. */
  GstCaseType type;             /*!< Case type @see GstCaseType */
  GInputStream *stream;
  GstCase *input;
  GstCase *branch;
  GstSwitchServeStreamType serve_type;  /*!< Stream type. @see GstSwitchServeStreamType */
  gboolean switching;
  gint sink_port;
  guint width;
  guint height;
  guint a_width;
  guint a_height;
  guint b_width;
  guint b_height;
} GstCase;

/**
 *  @class GstCaseClass
 *  @struct _GstCaseClass
 *  @brief The class of GstCase.
 *  @param base_class the parent class
 *  @see GstCase
 */
typedef struct _GstCaseClass
{
  GstWorkerClass base_class;    /*!< The base class. */
} GstCaseClass;

GType gst_case_get_type (void);

#endif //__GST_CASE_H__
