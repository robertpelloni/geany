#ifndef BOBGUI_CPP_TOOL_SURFACE_BUILDER_HPP
#define BOBGUI_CPP_TOOL_SURFACE_BUILDER_HPP

#include <bobgui/bobgui.h>

#include "action_registry.hpp"
#include "tool_surface.hpp"

#include <string>

namespace bobgui {
namespace cpp {

class ToolSurfaceBuilder
{
public:
  struct Options
  {
    bool show_section_labels;
    bool show_subtitles;
    bool show_shortcuts;
    bool show_checked_prefix;
    bool show_tooltips;
    bool frame_sections;
    int section_spacing;
    int item_spacing;

    Options ()
    : show_section_labels (true),
      show_subtitles (true),
      show_shortcuts (true),
      show_checked_prefix (true),
      show_tooltips (true),
      frame_sections (false),
      section_spacing (8),
      item_spacing (4)
    {
    }

    static Options detailed ()
    {
      Options options;

      options.show_section_labels = true;
      options.show_subtitles = true;
      options.show_shortcuts = true;
      options.show_checked_prefix = true;
      options.show_tooltips = true;
      options.frame_sections = true;
      return options;
    }
  };

  explicit ToolSurfaceBuilder (ActionRegistry &registry)
  : registry_ (registry)
  {
  }

  BobguiWidget *build_widget (const ToolSurfaceModel &model,
                              const Options          &options = Options ())
  {
    BobguiWidget *root = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, options.section_spacing);

    for (std::vector<ToolSurfaceModel::ToolSection>::const_iterator section = model.sections ().begin ();
         section != model.sections ().end ();
         ++section)
      {
        BobguiWidget *section_box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, options.item_spacing);
        BobguiWidget *items_box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, options.item_spacing);
        BobguiWidget *section_widget = section_box;

        if (options.show_section_labels)
          {
            BobguiWidget *section_label = bobgui_label_new (section->title.c_str ());
            bobgui_label_set_xalign (BOBGUI_LABEL (section_label), 0.0f);
            bobgui_box_append (BOBGUI_BOX (section_box), section_label);
          }

        for (std::vector<ToolSurfaceModel::ToolItem>::const_iterator item = section->items.begin ();
             item != section->items.end ();
             ++item)
          bobgui_box_append (BOBGUI_BOX (items_box), build_item (*item, options));

        bobgui_box_append (BOBGUI_BOX (section_box), items_box);

        if (options.frame_sections)
          {
            BobguiWidget *frame = bobgui_frame_new (options.show_section_labels ? NULL : section->title.c_str ());
            bobgui_frame_set_child (BOBGUI_FRAME (frame), section_box);
            section_widget = frame;
          }

        bobgui_box_append (BOBGUI_BOX (root), section_widget);
      }

    return root;
  }

private:
  struct ButtonBinding
  {
    ActionRegistry *registry;
    std::string action_id;
  };

  static void on_button_clicked (BobguiButton *button,
                                 gpointer      user_data)
  {
    ButtonBinding *binding = static_cast<ButtonBinding *> (user_data);

    (void) button;

    if (binding->registry != NULL)
      binding->registry->activate (binding->action_id.c_str ());
  }

  static void destroy_binding (gpointer data,
                               GClosure *closure)
  {
    (void) closure;
    delete static_cast<ButtonBinding *> (data);
  }

  BobguiWidget *build_item (const ToolSurfaceModel::ToolItem &item,
                            const Options                    &options)
  {
    BobguiWidget *item_box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 2);
    BobguiWidget *button = build_button (item, options);

    bobgui_box_append (BOBGUI_BOX (item_box), button);

    if (options.show_subtitles)
      {
        std::string details;

        if (!item.subtitle.empty ())
          details = item.subtitle;

        if (options.show_shortcuts && !item.shortcut.empty ())
          {
            if (!details.empty ())
              details += " — ";
            details += item.shortcut;
          }

        if (!details.empty ())
          {
            BobguiWidget *subtitle = bobgui_label_new (details.c_str ());
            bobgui_label_set_xalign (BOBGUI_LABEL (subtitle), 0.0f);
            bobgui_box_append (BOBGUI_BOX (item_box), subtitle);
          }
      }

    return item_box;
  }

  BobguiWidget *build_button (const ToolSurfaceModel::ToolItem &item,
                              const Options                    &options)
  {
    ButtonBinding *binding = new ButtonBinding;
    BobguiWidget *button;
    std::string label = item.title;

    binding->registry = &registry_;
    binding->action_id = item.action_id;

    if (options.show_checked_prefix && item.checkable && item.checked)
      label = std::string ("✓ ") + label;

    button = bobgui_button_new_with_label (label.c_str ());

    if (!item.icon_name.empty ())
      bobgui_button_set_icon_name (BOBGUI_BUTTON (button), item.icon_name.c_str ());

    if (options.show_tooltips)
      {
        std::string tooltip = item.title;

        if (!item.subtitle.empty ())
          tooltip += " — " + item.subtitle;

        if (!item.shortcut.empty ())
          tooltip += " (" + item.shortcut + ")";

        bobgui_widget_set_tooltip_text (button, tooltip.c_str ());
      }

    g_signal_connect_data (button,
                           "clicked",
                           G_CALLBACK (&ToolSurfaceBuilder::on_button_clicked),
                           binding,
                           &ToolSurfaceBuilder::destroy_binding,
                           GConnectFlags (0));

    return button;
  }

  ActionRegistry &registry_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
