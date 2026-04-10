Title: Accessibility
Slug: bobgui-accessibility

## The standard accessibility interface

The `BobguiAccessible` interface provides the accessibility information about
an application's user interface elements. Assistive technology (AT)
applications, like Orca, convey this information to users with disabilities,
or reduced abilities, to help them use the application.

Standard BOBGUI controls implement the `BobguiAccessible` interface and are thus
accessible to ATs by default. This means that if you use BOBGUI controls such
as `BobguiButton`, `BobguiEntry`, or `BobguiListView`, you only need to supply
application-specific details when the defaults values are incomplete. You
can do this by setting the appropriate properties in your `BobguiBuilder`
template and UI definition files, or by setting the properties defined by
the `BobguiAccessible` interface.

If you are implementing your own `BobguiWidget` derived type, you will need to
set the `BobguiAccessible` properties yourself, and provide an implementation
of the `BobguiAccessible` virtual functions.

## Accessible roles and attributes

The fundamental concepts of an accessible widget are *roles* and
*attributes*; each BOBGUI control has a role, while its functionality is
described by a set of *attributes*.

### Roles

Roles define the taxonomy and semantics of a UI control to any assistive
technology application; for instance, a button will have a role of
`BOBGUI_ACCESSIBLE_ROLE_BUTTON`; an entry will have a role of
`BOBGUI_ACCESSIBLE_ROLE_TEXTBOX`; a check button will have a role of
`BOBGUI_ACCESSIBLE_ROLE_CHECKBOX`; etc.

Each role is part of the widget's instance, and **cannot** be changed over
time or as the result of a user action. Roles allows assistive technology
applications to identify a UI control and decide how to present it to a
user; if a part of the application's UI changes role, the control needs to
be removed and replaced with another one with the appropriate role.

#### List of accessible roles

Each role name is part of the #BobguiAccessibleRole enumeration.

| Role name | Description | Related BOBGUI widget |
|-----------|-------------|--------------------|
| `WINDOW` | An application window | [class@Bobgui.Window] |
| `BUTTON` | A control that performs an action when pressed | [class@Bobgui.Button], [class@Bobgui.LinkButton], [class@Bobgui.Expander] |
| `CHECKBOX` | A control that has three possible value: `true`, `false`, or `undefined` | [class@Bobgui.CheckButton] |
| `COMBOBOX` | A control that can be expanded to show a list of possible values to select | [class@Bobgui.ComboBox] |
| `COLUMN_HEADER` | A header in a columned list | [class@Bobgui.ColumnView] |
| `DIALOG` | A dialog that prompts the user to enter information or require a response | [class@Bobgui.Dialog] and subclasses |
| `GRID` | A grid of items | [class@Bobgui.FlowBox], [class@Bobgui.GridView] |
| `GRID_CELL` | An item in a grid | [class@Bobgui.FlowBoxChild], [class@Bobgui.GridView], [class@Bobgui.ColumnView] |
| `IMG` | An image | [class@Bobgui.Image], [class@Bobgui.Picture] |
| `LABEL` | A visible name or caption for a user interface component | [class@Bobgui.Label] |
| `LINK` | A clickable hyperlink | [class@Bobgui.LinkButton] |
| `LIST` | A list of items | [class@Bobgui.ListBox] |
| `LIST_ITEM` | An item in a list | [class@Bobgui.ListBoxRow] |
| `MENU` | A menu | [class@Bobgui.PopoverMenu] |
| `MENU_BAR` | A menubar | [class@Bobgui.PopoverMenuBar] |
| `MENU_ITEM` | A menu item | Items in [class@Bobgui.PopoverMenu] |
| `MENU_ITEM_CHECKBOX` | Check menu item | Items in [class@Bobgui.PopoverMenu] |
| `MENU_ITEM_RADIO` | Radio menu item | Items in [class@Bobgui.PopoverMenu] |
| `METER` | Represents a value within a known range | [class@Bobgui.LevelBar] |
| `NONE` | Not represented in the accessibility tree | the slider of a [class@Bobgui.Scale] |
| `PROGRESS_BAR` | An element that display progress | [class@Bobgui.ProgressBar] |
| `RADIO` | A checkable input in a group of radio roles | [class@Bobgui.CheckButton] |
| `ROW` | A row in a columned list | [class@Bobgui.ColumnView] |
| `SCROLLBAR` | A graphical object controlling the scrolling of content | [class@Bobgui.Scrollbar] |
| `SEARCH_BOX` | A text box for entering search criteria | [class@Bobgui.SearchEntry] |
| `SEPARATOR` | A divider that separates sections of content or groups of items | [class@Bobgui.Separator] |
| `SPIN_BUTTON` | A range control that allows seelcting among discrete choices | [class@Bobgui.SpinButton] |
| `SWITCH` | A control that represents on/off values | [class@Bobgui.Switch] |
| `TAB` | A tab in a list of tabs for switching pages | [class@Bobgui.StackSwitcher], [class@Bobgui.Notebook] |
| `TAB_LIST` | A list of tabs for switching pages | [class@Bobgui.StackSwitcher], [class@Bobgui.Notebook] |
| `TAB_PANEL` | A page in a notebook or stack | [class@Bobgui.Stack] |
| `TEXT_BOX` | A type of input that allows free-form text as its value. | [class@Bobgui.Entry], [class@Bobgui.PasswordEntry], [class@Bobgui.TextView] |
| `TREE_GRID` | A treeview-like columned list | [class@Bobgui.ColumnView] |
| `...` | … |

See the [WAI-ARIA](https://www.w3.org/WAI/PF/aria/appendices#quickref) list
of roles for additional information.

### Attributes

Attributes provide specific information about an accessible UI
control, and describe it for the assistive technology applications. BOBGUI
divides the accessible attributes into three categories:

 - *properties*, described by the values of the `BobguiAccessibleProperty` enumeration
 - *relations*, described by the values of the `BobguiAccessibleRelation` enumeration
 - *states*, described by the values of the `BobguiAccessibleState` enumeration

Each attribute accepts a value of a specific type.

Unlike roles, attributes may change over time, or in response to user action;
for instance:

 - a toggle button will change its `BOBGUI_ACCESSIBLE_STATE_CHECKED` state every
   time it is toggled, either by the user or programmatically
 - setting the mnemonic widget on a `BobguiLabel` will update the
   `BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY` relation on the widget with a
   reference to the label
 - changing the `BobguiAdjustment` instance on a `BobguiScrollbar` will change the
   `BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX`, `BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN`,
   and `BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW` properties with the upper, lower,
   and value properties of the `BobguiAdjustment`

See the [WAI-ARIA](https://www.w3.org/WAI/PF/aria/appendices#quickref) list
of attributes for additional information.

#### List of accessible states

Each state name is part of the `BobguiAccessibleState` enumeration.

| State name | ARIA attribute | Value type | Notes |
|------------|----------------|------------|-------|
| `BOBGUI_ACCESSIBLE_STATE_BUSY` | “aria-busy” | boolean |
| `BOBGUI_ACCESSIBLE_STATE_CHECKED` | “aria-checked” | `BobguiAccessibleTristate` | Indicates the current state of a [class@Bobgui.CheckButton] |
| `BOBGUI_ACCESSIBLE_STATE_DISABLED` | “aria-disabled” | boolean | Corresponds to the [property@Bobgui.Widget:sensitive] property on [class@Bobgui.Widget] |
| `BOBGUI_ACCESSIBLE_STATE_EXPANDED` | “aria-expanded” | boolean or undefined | Corresponds to the  [property@Bobgui.Expander:expanded] property on [class@Bobgui.Expander] |
| `BOBGUI_ACCESSIBLE_STATE_HIDDEN` | “aria-hidden” | boolean | Corresponds to the [property@Bobgui.Widget:visible] property on [class@Bobgui.Widget] |
| `BOBGUI_ACCESSIBLE_STATE_INVALID` | “aria-invalid” | `BobguiAccessibleInvalidState` | Set when a widget is showing an error |
| `BOBGUI_ACCESSIBLE_STATE_PRESSED` | “aria-pressed” | `BobguiAccessibleTristate` | Indicates the current state of a [class@Bobgui.ToggleButton] |
| `BOBGUI_ACCESSIBLE_STATE_SELECTED` | “aria-selected” | boolean or undefined | Set when a widget is selected |
| `BOBGUI_ACCESSIBLE_STATE_VISITED` | N/A | boolean or undefined | Set when a link-like widget is visited |

#### List of accessible properties

Each property name is part of the `BobguiAccessibleProperty` enumeration.

| State name | ARIA attribute | Value type |
|------------|----------------|------------|
| `BOBGUI_ACCESSIBLE_PROPERTY_AUTOCOMPLETE` | “aria-autocomplete” | `BobguiAccessibleAutocomplete` |
| `BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION` | “aria-description” | translatable string |
| `BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP` | “aria-haspopup” | boolean |
| `BOBGUI_ACCESSIBLE_PROPERTY_KEY_SHORTCUTS` | “aria-keyshortcuts” | string |
| `BOBGUI_ACCESSIBLE_PROPERTY_LABEL` | “aria-label” | translatable string |
| `BOBGUI_ACCESSIBLE_PROPERTY_LEVEL` | “aria-level” | integer |
| `BOBGUI_ACCESSIBLE_PROPERTY_MODAL` | “aria-modal” | boolean |
| `BOBGUI_ACCESSIBLE_PROPERTY_MULTI_LINE` | “aria-multiline” | boolean |
| `BOBGUI_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE` | “aria-multiselectable” | boolean |
| `BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION` | “aria-orientation” | `BobguiOrientation` |
| `BOBGUI_ACCESSIBLE_PROPERTY_PLACEHOLDER` | “aria-placeholder” | translatable string |
| `BOBGUI_ACCESSIBLE_PROPERTY_READ_ONLY` | “aria-readonly” | boolean |
| `BOBGUI_ACCESSIBLE_PROPERTY_REQUIRED` | “aria-required” | boolean |
| `BOBGUI_ACCESSIBLE_PROPERTY_ROLE_DESCRIPTION` | “aria-roledescription” | translatable string |
| `BOBGUI_ACCESSIBLE_PROPERTY_SORT` | “aria-sort” | `BobguiAccessibleSort` |
| `BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX` | “aria-valuemax” | double |
| `BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN` | “aria-valuemin” | double |
| `BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW` | “aria-valuenow” | double |
| `BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT` | “aria-valuetext” | translatable string |
| `BOBGUI_ACCESSIBLE_PROPERTY_HELP_TEXT` | N/A | translatable string |

#### List of accessible relations

Each relation name is part of the `BobguiAccessibleRelation` enumeration.

| State name | ARIA attribute | Value type |
|------------|----------------|------------|
| `BOBGUI_ACCESSIBLE_RELATION_ACTIVE_DESCENDANT` | “aria-activedescendant” | `BobguiAccessible` |
| `BOBGUI_ACCESSIBLE_RELATION_COL_COUNT` | “aria-colcount” | integer |
| `BOBGUI_ACCESSIBLE_RELATION_COL_INDEX` | “aria-colindex” | integer |
| `BOBGUI_ACCESSIBLE_RELATION_COL_INDEX_TEXT` | “aria-colindextext” | translatable string |
| `BOBGUI_ACCESSIBLE_RELATION_COL_SPAN` | “aria-colspan” | integer |
| `BOBGUI_ACCESSIBLE_RELATION_CONTROLS` | “aria-controls” | a list of `BobguiAccessible` |
| `BOBGUI_ACCESSIBLE_RELATION_DESCRIBED_BY` | “aria-describedby” | a list of `BobguiAccessible` |
| `BOBGUI_ACCESSIBLE_RELATION_DETAILS` | “aria-details” | a list of `BobguiAccessible` |
| `BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE` | “aria-errormessage” | a list of `BobguiAccessible` |
| `BOBGUI_ACCESSIBLE_RELATION_FLOW_TO` | “aria-flowto” | a list of `BobguiAccessible` |
| `BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY` | “aria-labelledby” | a list of `BobguiAccessible` |
| `BOBGUI_ACCESSIBLE_RELATION_OWNS` | “aria-owns” | a list of `BobguiAccessible` |
| `BOBGUI_ACCESSIBLE_RELATION_POS_IN_SET` | “aria-posinset” | integer |
| `BOBGUI_ACCESSIBLE_RELATION_ROW_COUNT` | “aria-rowcount” | integer |
| `BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX` | “aria-rowindex” | integer |
| `BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX_TEXT` | “aria-rowindextext” | translatable string |
| `BOBGUI_ACCESSIBLE_RELATION_ROW_SPAN` | “aria-rowspan” | integer |
| `BOBGUI_ACCESSIBLE_RELATION_SET_SIZE` | “aria-setsize” | integer |

*Note*: When using bobgui_accessible_update_relation() with a relation that
requires a list of `BobguiAccessible` instances, you should pass every
accessible object separately, followed by `NULL`. 

## Application development rules

Even if standard UI controls provided by BOBGUI have accessibility information
out of the box, there are some additional properties and considerations for
application developers. For instance, if your application presents the user
with a form to fill out, you should ensure that:

 * the container of the form has a `BOBGUI_ACCESSIBLE_ROLE_FORM` role
 * each text entry widget in the form has the `BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY`
   relation pointing to the label widget that describes it

Another example: if you create a toolbar containing buttons with only icons,
you should ensure that:

 * the container has a `BOBGUI_ACCESSIBLE_ROLE_TOOLBAR` role
 * each button has a `BOBGUI_ACCESSIBLE_PROPERTY_LABEL` property set with the user
   readable and localised action performed when pressed; for instance "Copy",
   "Paste", "Add layer", or "Remove"

BOBGUI will try to fill in some information by using ancillary UI control properties,
for instance the accessible name will be taken from the label used by the UI control,
or from its tooltip, if the `BOBGUI_ACCESSIBLE_PROPERTY_LABEL` property or the
`BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY` relation are unset. Similarly for the accessible
description. Nevertheless, it is good practice and project hygiene to explicitly specify
the accessible properties, just like it's good practice to specify tooltips and style classes.

Application developers using BOBGUI **should** ensure that their UI controls
are accessible as part of the development process. The BOBGUI Inspector shows
the accessible attributes of each widget, and also provides an overlay that
can highlight accessibility issues.

If you support some non-standard keyboard interactions for a widget, you
**should** set an appropriate `BOBGUI_ACCESSIBLE_PROPERTY_HELP_TEXT` to help
discoverability of the behavior.

It is possible to set accessible attributes in UI files as well:
```xml
<object class="BobguiButton" id="button1">
  <accessibility>
    <property name="label">Download</property>
    <relation name="labelled-by">label1</relation>
  </accessibility>
</object>
```

## Implementations

Each UI control implements the `BobguiAccessible` interface to allow widget and
application developers to specify the roles, state, and relations between UI
controls. This API is purely descriptive.

Each `BobguiAccessible` implementation must provide a `BobguiATContext` instance,
which acts as a proxy to the specific platform's accessibility API:

 * AT-SPI on Linux/BSD
 * NSAccessibility on macOS
 * Active Accessibility on Windows

Additionally, an ad hoc accessibility backend is available for the BOBGUI
testsuite, to ensure reproducibility of issues in the CI pipeline.

## Authoring practices

The authoring practices are aimed at application developers, as well as
developers of GUI elements based on BOBGUI.

Functionally, `BobguiAccessible` roles, states, properties, and relations are
analogous to a CSS for assistive technologies. For screen reader users, for
instance, the various accessible attributes control the rendering of their
non-visual experience. Incorrect roles and attributes may result in a
completely inaccessible user interface.

### A role is a promise

The following code:

```c
bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_BUTTON);
```

is a promise that the widget being created will provide the same keyboard
interactions expected for a button. An accessible role of a button will not
turn automatically any widget into a `BobguiButton`; but if your widget behaves
like a button, using the `BOBGUI_ACCESSIBLE_ROLE_BUTTON` will allow any
assistive technology to handle it like they would a `BobguiButton`.

For widgets that act as containers of other widgets, you should use
`BOBGUI_ACCESSIBLE_ROLE_GROUP` if the grouping of the children is semantic
in nature; for instance, the children of a [class@Bobgui.HeaderBar] are
grouped together on the header of a window. For generic containers that
only impose a layout on their children, you should use
`BOBGUI_ACCESSIBLE_ROLE_GENERIC` instead.

### Attributes can both hide and enhance

Accessible attributes can be used to override the content of a UI element,
for instance:

```c
bobgui_label_set_text (BOBGUI_LABEL (label), "Some text");
bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (label),
				BOBGUI_ACCESSIBLE_PROPERTY_LABEL,
				"Assistive technologies users will perceive "
				"this text, not the contents of the label",
				-1);
```

In the example above, the "label" property will override the contents of the
label widget.

The attributes can also enhance the UI:

```c
bobgui_button_set_label (BOBGUI_BUTTON (button), "Download");
bobgui_box_append (BOBGUI_BOX (box), button);

bobgui_label_set_text (BOBGUI_LABEL (label), "Final report.pdf");
bobgui_box_append (BOBGUI_BOX (box), label);

bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (button),
				BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY,
				g_list_append (NULL, label),
				-1);
```

In the example above, an assistive technology will read the button's
accessible label as "Download Final report.pdf".

The power of hiding and enhancing can be a double-edged sword, as it can
lead to inadvertently overriding the accessible semantics of existing
widgets.

## Hiding UI elements from the accessible tree

The accessibility API is mainly used to express semantics useful for
assistive technologies, but it can also be used to hide elements. The
canonical way to do so is to use the `BOBGUI_ACCESSIBLE_ROLE_PRESENTATION`,
which declares that a UI element is purely meant for presentation purposes,
and as such it has no meaningful impact on the accessibility of the
interface.

A "presentation" role should not be confused with the
`BOBGUI_ACCESSIBLE_STATE_HIDDEN` state; the "hidden" state is transient, and is
typically controlled by showing and hiding a widget using the `BobguiWidget`
API.

## Design patterns and custom widgets

When creating custom widgets, following established patterns can help
ensuring that the widgets work well for users of accessible technologies
as well.

### Buttons

A button is a widget that enables users to trigger an action. While it is
recommended you use `BobguiButton` for anything that looks and behaves like a
button, it is possible to apply a button behavior to UI elements like images
by using a `BobguiGestureClick` gesture. When doing so, you should:

  - Give your widget the role `BOBGUI_ACCESSIBLE_ROLE_BUTTON`
  - Install an action with no parameters, which will activate the widget

### Custom entries

For custom entries, it is highly recommended that you implement the
`BobguiEditable` interface by using a `BobguiText` widget as delegate. If you
do this, BOBGUI will make your widgets text editing functionality accessible
in the same way as a `BobguiSpinButton` or `BobguiSearchEntry`.

### Tab-based UI

If you make a tab-based interface, you should consider using `BobguiStack`
as the core, and just make a custom tab widget to control the active
stack page. When doing so, the following extra steps will ensure that
your tabs are accessible in the same way as `BobguiStackSwitcher` or `BobguiNotebook`:

- Give your tab container the role `BOBGUI_ACCESSIBLE_ROLE_TAB_LIST`
- Give your tab widgets the role `BOBGUI_ACCESSIBLE_ROLE_TAB`
- Set up the `BOBGUI_ACCESSIBLE_RELATION_CONTROLS` relation between each
  tab and the `BobguiStackPage` object for its page
- Set the `BOBGUI_ACCESSIBLE_PROPERTY_SELECTED` property on each tab, with
  the active tab getting the value true, all others false

To allow changing the active tab via accessible technologies, you can
export actions. Since the accessibility interfaces only support actions
without parameters, you can either provide `previous-tab` and `next-tab`
actions on the tab container that let users step through the tabs one-by-one,
or add a `activate-tab` action on each tab.

### Value controls

A value control (ie a widget that controls a one-dimensional quantity
that can be represented by a `BobguiAdjustment`) can be represented to
accessible technologies by setting the `BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN`,
`BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX` and `BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW`
properties.

To allow changing the value via accessible technologies, you can export
actions.  Since the accessibility interfaces only support actions
without parameters, you should provide actions such as `increase-value`
and `decrease-value`.

Since BOBGUI 4.10, the best way to suppose changing the value is by implementing
the [iface@Bobgui.AccessibleRange] interface.

## Out-of-process embedding

Some types of applications commonly split their UI into multiple processes.
A typical example would be web browsers with one process for each web page that
is loaded in a tab. In this case, there is a need to 'graft' the accessible
tree of the out-of-process components into the main accessible tree of the
application.

The AT-SPI accessibility backend of BOBGUI supports this, using the
`BobguiAtSpiSocket` interface, which implements the org.a11y.atspi.Socket
D-Bus interface on the accessibility bus.

## Other resources

The Orca screenreader has [Tips for Application Developers](https://gitlab.gnome.org/GNOME/orca/-/blob/main/README-APPLICATION-DEVELOPERS.md).

The [Web Content Accessibility Guidelines](https://www.w3.org/TR/WCAG21/) are Web-focused,
but useful nevertheless.
