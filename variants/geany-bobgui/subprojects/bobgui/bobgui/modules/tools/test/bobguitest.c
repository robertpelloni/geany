#include "bobguitest.h"
G_DEFINE_TYPE (BobguiTestRunner, bobgui_test_runner, G_TYPE_OBJECT)
static void bobgui_test_runner_init (BobguiTestRunner *s) {}
static void bobgui_test_runner_class_init (BobguiTestRunnerClass *k) {}
BobguiTestRunner * bobgui_test_runner_new (BobguiWidget *r) { return g_object_new (BOBGUI_TYPE_TEST_RUNNER, NULL); }
