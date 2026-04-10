
#pragma once

#include "bobguiwidget.h"
#include "bobguienums.h"

#define BOBGUI_TYPE_PANED_HANDLE                 (bobgui_paned_handle_get_type ())
#define BOBGUI_PANED_HANDLE(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PANED_HANDLE, BobguiPanedHandle))
#define BOBGUI_PANED_HANDLE_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_PANED_HANDLE, BobguiPanedHandleClass))
#define BOBGUI_IS_PANED_HANDLE(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PANED_HANDLE))
#define BOBGUI_IS_PANED_HANDLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_PANED_HANDLE))
#define BOBGUI_PANED_HANDLE_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_PANED_HANDLE, BobguiPanedHandleClass))

typedef struct _BobguiPanedHandle             BobguiPanedHandle;
typedef struct _BobguiPanedHandleClass        BobguiPanedHandleClass;

struct _BobguiPanedHandle
{
  BobguiWidget parent_instance;
};

struct _BobguiPanedHandleClass
{
  BobguiWidgetClass parent_class;
};

GType      bobgui_paned_handle_get_type (void) G_GNUC_CONST;

BobguiWidget *bobgui_paned_handle_new (void);

