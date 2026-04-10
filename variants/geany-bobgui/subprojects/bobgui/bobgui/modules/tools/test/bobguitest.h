#ifndef BOBGUI_TEST_H
#define BOBGUI_TEST_H

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_TEST_RUNNER (bobgui_test_runner_get_type ())
G_DECLARE_FINAL_TYPE (BobguiTestRunner, bobgui_test_runner, BOBGUI, TEST_RUNNER, GObject)

BobguiTestRunner * bobgui_test_runner_new (BobguiWidget *root);

G_END_DECLS

#endif
