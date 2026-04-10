#pragma once

#include <bobgui/bobgui.h>

#define BOBGUI_TYPE_PAGE_THUMBNAIL (bobgui_page_thumbnail_get_type ())
G_DECLARE_FINAL_TYPE (BobguiPageThumbnail, bobgui_page_thumbnail, BOBGUI, PAGE_THUMBNAIL, BobguiWidget)

BobguiPageThumbnail * bobgui_page_thumbnail_new          (void);
void               bobgui_page_thumbnail_set_page_num (BobguiPageThumbnail *self,
                                                    int               page_num);
int                bobgui_page_thumbnail_get_page_num (BobguiPageThumbnail *self);

