/* GstSwitch
 * Copyright (C) 2012,2013 Duzy Chan <code@duzy.info>
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

#include "../tools/gstworker.c"

gboolean verbose = FALSE;
GMainLoop *mainloop = NULL;
GstWorker *worker0 = NULL;
GstWorker *worker1 = NULL;
GstWorker *worker2 = NULL;
gboolean transition = TRUE;
GMutex trans_lock;

static GString *
test_worker_pipeline (GstWorker * worker, gpointer data)
{
  GString *desc;

  desc = g_string_new ("");

  g_string_append_printf (desc, "videotestsrc name=source ");
  g_string_append_printf (desc, "! video/x-raw,width=180,height=100 ");
  g_string_append_printf (desc, "! fakesink ");
  //g_string_append_printf (desc, "! xvimagesink ");

  return desc;
}

static gpointer
test_worker_pulse (GstWorker * worker)
{
  //sleep (1);

  while (GST_IS_WORKER (worker)) {
    usleep (1000 * 1);
    if (!transition) {
      g_mutex_lock (&trans_lock);
      if (!transition) {
        GstState state;
        GstStateChangeReturn ret;
        INFO ("stop: %s %p", worker->name, worker);
        transition = gst_worker_stop_force (worker, TRUE);
        g_assert (transition);
        ret = gst_element_get_state (worker->pipeline, &state, NULL,
            GST_CLOCK_TIME_NONE);
        g_assert (ret == GST_STATE_CHANGE_SUCCESS);
        g_assert (state == GST_STATE_NULL);
        INFO ("stopped: %s %p", worker->name, worker);
      }
      g_mutex_unlock (&trans_lock);
    }

    if (transition)
      usleep (1000 * 10);

    //INFO ("tick: %d", transition);
  }

  INFO ("end pulse");
  return NULL;
}

static gboolean
test_worker_close_transition (GstWorker * worker)
{
  if (transition) {
    g_mutex_lock (&trans_lock);
    if (transition) {
      transition = FALSE;
    }
    g_mutex_unlock (&trans_lock);
  }
  return FALSE;
}

static gboolean
test_worker_reset (GstWorker * worker)
{
  GstWorkerClass *worker_class;
  worker_class = GST_WORKER_CLASS (G_OBJECT_GET_CLASS (worker));
  return worker_class->reset (worker);
}

static void
test_worker_prepare (GstWorker * worker, gpointer data)
{
  g_return_if_fail (GST_IS_WORKER (worker));

  INFO ("%s: %s", worker->name, __FUNCTION__);

  /*
     if (worker1 == NULL) {
     worker1 = GST_WORKER (g_object_new (GST_TYPE_WORKER,
     "name", "test-worker-1", NULL));
     worker1->pipeline_func = test_worker_pipeline;
     }

     if (worker2 == NULL) {
     worker2 = GST_WORKER (g_object_new (GST_TYPE_WORKER,
     "name", "test-worker-2", NULL));
     worker2->pipeline_func = test_worker_pipeline;
     }
   */
}

static void
test_worker_start (GstWorker * worker, gpointer data)
{
  g_return_if_fail (GST_IS_WORKER (worker));

  INFO ("%s: %s", worker->name, __FUNCTION__);

  if (transition) {
    g_mutex_lock (&trans_lock);
    if (transition) {
      /*
         gst_worker_start (worker1);
         gst_worker_start (worker2);
       */

      g_timeout_add (10, (GSourceFunc) test_worker_close_transition, worker);
    }
    g_mutex_unlock (&trans_lock);
  }
}

static void
test_worker_end (GstWorker * worker, gpointer data)
{
  gboolean worker_reset_ok;

  //INFO ("%s: %s, %d", worker->name, __FUNCTION__, transition);

  g_return_if_fail (GST_IS_WORKER (worker));

  if (transition) {
    g_mutex_lock (&trans_lock);
    if (transition) {
      worker_reset_ok = test_worker_reset (worker);
      g_assert (worker_reset_ok);

      INFO ("%s: %s", worker->name, __FUNCTION__);

      /*
         worker_reset_ok = test_worker_reset (worker1);
         g_assert (worker_reset_ok);

         worker_reset_ok = test_worker_reset (worker2);
         g_assert (worker_reset_ok);
       */

      gst_element_set_state (worker->pipeline, GST_STATE_READY);
      /*
         gst_element_set_state (worker1->pipeline, GST_STATE_READY);
         gst_element_set_state (worker2->pipeline, GST_STATE_READY);
       */
    }
    g_mutex_unlock (&trans_lock);
  }
}

int
main (int argc, char **argv)
{
  gst_init (&argc, &argv);

  g_mutex_init (&trans_lock);

  mainloop = g_main_loop_new (NULL, TRUE);

  worker0 = GST_WORKER (g_object_new (GST_TYPE_WORKER,
          "name", "test-worker-0", NULL));

  worker0->pipeline_func = test_worker_pipeline;

  g_signal_connect (worker0, "prepare-worker",
      G_CALLBACK (test_worker_prepare), NULL);
  g_signal_connect (worker0, "start-worker",
      G_CALLBACK (test_worker_start), NULL);
  g_signal_connect (worker0, "end-worker", G_CALLBACK (test_worker_end), NULL);

  g_thread_new ("test-pulse", (GThreadFunc) test_worker_pulse, worker0);

  gst_worker_start (worker0);

  g_main_loop_run (mainloop);

  g_mutex_clear (&trans_lock);

  INFO ("end");
  return 0;
}
