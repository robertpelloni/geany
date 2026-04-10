#pragma once

#include <bobgui.h>

G_DECLARE_FINAL_TYPE (FakeScope, fake_scope, FAKE, SCOPE, BobguiBuilderCScope)

FakeScope * fake_scope_new           (void);
GPtrArray * fake_scope_get_types     (FakeScope *self);
GPtrArray * fake_scope_get_callbacks (FakeScope *self);
