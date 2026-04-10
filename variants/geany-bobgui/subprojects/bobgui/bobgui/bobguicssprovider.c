/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "bobguicssproviderprivate.h"

#include <bobgui/css/bobguicss.h>
#include "bobgui/css/bobguicsstokenizerprivate.h"
#include "bobgui/css/bobguicssparserprivate.h"
#include "bobgui/css/bobguicssvariablevalueprivate.h"
#include "bobguibitmaskprivate.h"
#include "bobguicssarrayvalueprivate.h"
#include "bobguicsscolorvalueprivate.h"
#include "bobguicsscustompropertypoolprivate.h"
#include "bobguicsskeyframesprivate.h"
#include "bobguicssmediaqueryprivate.h"
#include "bobguicssreferencevalueprivate.h"
#include "bobguicssselectorprivate.h"
#include "bobguicssshorthandpropertyprivate.h"
#include "bobguisettingsprivate.h"
#include "bobguistyleprovider.h"
#include "bobguistylepropertyprivate.h"
#include "bobguistyleproviderprivate.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"
#include "bobguiversion.h"
#include "bobguitypebuiltins.h"

#include <string.h>
#include <stdlib.h>

#include "gdk/gdkprofilerprivate.h"
#include <cairo-gobject.h>

#define GDK_ARRAY_NAME bobgui_css_selectors
#define GDK_ARRAY_TYPE_NAME BobguiCssSelectors
#define GDK_ARRAY_ELEMENT_TYPE BobguiCssSelector *
#define GDK_ARRAY_PREALLOC 64
#include "gdk/gdkarrayimpl.c"

/* For lack of a better place, assert here that these two definitions match */
G_STATIC_ASSERT (BOBGUI_DEBUG_CSS == BOBGUI_CSS_PARSER_DEBUG_CSS);

/**
 * BobguiCssProvider:
 *
 * A style provider for CSS.
 *
 * It is able to parse CSS-like input in order to style widgets.
 *
 * An application can make BOBGUI parse a specific CSS style sheet by calling
 * [method@Bobgui.CssProvider.load_from_file] or
 * [method@Bobgui.CssProvider.load_from_resource]
 * and adding the provider with [method@Bobgui.StyleContext.add_provider] or
 * [func@Bobgui.StyleContext.add_provider_for_display].

 * In addition, certain files will be read when BOBGUI is initialized.
 * First, the file `$XDG_CONFIG_HOME/bobgui-4.0/bobgui.css` is loaded if it
 * exists. Then, BOBGUI loads the first existing file among
 * `XDG_DATA_HOME/themes/THEME/bobgui-VERSION/bobgui-VARIANT.css`,
 * `$HOME/.themes/THEME/bobgui-VERSION/bobgui-VARIANT.css`,
 * `$XDG_DATA_DIRS/themes/THEME/bobgui-VERSION/bobgui-VARIANT.css` and
 * `DATADIR/share/themes/THEME/bobgui-VERSION/bobgui-VARIANT.css`,
 * where `THEME` is the name of the current theme (see the
 * [property@Bobgui.Settings:bobgui-theme-name] setting), `VARIANT` is the
 * variant to load (see the
 * [property@Bobgui.Settings:bobgui-application-prefer-dark-theme] setting),
 * `DATADIR` is the prefix configured when BOBGUI was compiled (unless
 * overridden by the `BOBGUI_DATA_PREFIX` environment variable), and
 * `VERSION` is the BOBGUI version number. If no file is found for the
 * current version, BOBGUI tries older versions all the way back to 4.0.
 *
 * To track errors while loading CSS, connect to the
 * [signal@Bobgui.CssProvider::parsing-error] signal.
 */

#define MAX_SELECTOR_LIST_LENGTH 64

struct _BobguiCssProviderClass
{
  GObjectClass parent_class;

  void (* parsing_error)                        (BobguiCssProvider  *provider,
                                                 BobguiCssSection   *section,
                                                 const GError *   error);
};

typedef struct BobguiCssRuleset BobguiCssRuleset;
typedef struct _BobguiCssScanner BobguiCssScanner;
typedef struct _PropertyValue PropertyValue;
typedef enum ParserScope ParserScope;
typedef enum ParserSymbol ParserSymbol;

struct _PropertyValue {
  BobguiCssStyleProperty *property;
  BobguiCssValue         *value;
  BobguiCssSection       *section;
};

struct BobguiCssRuleset
{
  BobguiCssSelector *selector;
  BobguiCssSelectorTree *selector_match;
  PropertyValue *styles;
  guint n_styles;
  guint owns_styles : 1;
  GHashTable *custom_properties;
};

struct _BobguiCssScanner
{
  BobguiCssProvider *provider;
  BobguiCssParser *parser;
  BobguiCssScanner *parent;
  GArray *media_features;
  guint skip_count;
};

struct _BobguiCssProviderPrivate
{
  GScanner *scanner;

  BobguiInterfaceColorScheme prefers_color_scheme;
  BobguiInterfaceContrast prefers_contrast;
  BobguiReducedMotion prefers_reduced_motion;

  GHashTable *symbolic_colors;
  GHashTable *keyframes;

  GArray *rulesets;
  BobguiCssSelectorTree *tree;

  GBytes *source;
  GFile *source_file;
  gboolean needs_rerender;

  GResource *resource;
  char *path;
  GBytes *bytes; /* *no* reference */
};

enum {
  PARSING_ERROR,
  LAST_SIGNAL
};

enum {
   PROP_PREFERS_COLOR_SCHEME = 1,
   PROP_PREFERS_CONTRAST,
   PROP_PREFERS_REDUCED_MOTION,
   NUM_PROPERTIES
};

static GParamSpec *pspecs[NUM_PROPERTIES] = { NULL, };

static gboolean bobgui_keep_css_sections = FALSE;

static guint css_provider_signals[LAST_SIGNAL] = { 0 };

static void bobgui_css_provider_finalize         (GObject *object);
static void bobgui_css_provider_get_property     (GObject               *object,
                                               guint                  property_id,
                                               GValue                *value,
                                               GParamSpec            *pspec);
static void bobgui_css_provider_set_property     (GObject               *object,
                                               guint                  property_id,
                                               const GValue          *value,
                                               GParamSpec            *pspec);
static void bobgui_css_provider_notify           (GObject               *object,
                                               GParamSpec            *pspec);
static void bobgui_css_style_provider_iface_init (BobguiStyleProviderInterface *iface);
static void bobgui_css_style_provider_emit_error (BobguiStyleProvider *provider,
                                               BobguiCssSection    *section,
                                               const GError     *error);
static void bobgui_css_provider_reset            (BobguiCssProvider        *css_provider);


static void bobgui_css_provider_load_internal (BobguiCssProvider *css_provider,
                                            BobguiCssScanner  *scanner,
                                            GFile          *file,
                                            GBytes         *bytes);
static void parse_statement                (BobguiCssScanner  *scanner);

G_DEFINE_TYPE_EXTENDED (BobguiCssProvider, bobgui_css_provider, G_TYPE_OBJECT, 0,
                        G_ADD_PRIVATE (BobguiCssProvider)
                        G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_STYLE_PROVIDER,
                                               bobgui_css_style_provider_iface_init));

static void
bobgui_css_provider_parsing_error (BobguiCssProvider  *provider,
                                BobguiCssSection   *section,
                                const GError    *error)
{
  /* Only emit a warning when we have no error handlers. This is our
   * default handlers. And in this case erroneous CSS files are a bug
   * and should be fixed.
   * Note that these warnings can also be triggered by a broken theme
   * that people installed from some weird location on the internets.
   */
  if (!g_signal_has_handler_pending (provider,
                                     css_provider_signals[PARSING_ERROR],
                                     0,
                                     TRUE))
    {
      char *s = bobgui_css_section_to_string (section);

      if (BOBGUI_DEBUG_CHECK (CSS) ||
          !g_error_matches (error, BOBGUI_CSS_PARSER_WARNING, BOBGUI_CSS_PARSER_WARNING_DEPRECATED))
        g_warning ("Theme parser %s: %s: %s",
                   error->domain == BOBGUI_CSS_PARSER_WARNING ? "warning" : "error",
                   s,
                   error->message);

      g_free (s);
    }
}

/* This is exported privately for use in BobguiInspector.
 * It is the callers responsibility to reparse the current theme.
 */
void
bobgui_css_provider_set_keep_css_sections (void)
{
  bobgui_keep_css_sections = TRUE;
}

static void
bobgui_css_provider_class_init (BobguiCssProviderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  if (g_getenv ("BOBGUI_CSS_DEBUG"))
    bobgui_css_provider_set_keep_css_sections ();

  object_class->finalize = bobgui_css_provider_finalize;
  object_class->get_property = bobgui_css_provider_get_property;
  object_class->set_property = bobgui_css_provider_set_property;
  object_class->notify = bobgui_css_provider_notify;

  klass->parsing_error = bobgui_css_provider_parsing_error;

  /**
   * BobguiCssProvider::parsing-error:
   * @provider: the provider that had a parsing error
   * @section: section the error happened in
   * @error: The parsing error
   *
   * Signals that a parsing error occurred.
   *
   * The expected error values are in the [error@Bobgui.CssParserError]
   * and [enum@Bobgui.CssParserWarning] enumerations.
   *
   * The @path, @line and @position describe the actual location of
   * the error as accurately as possible.
   *
   * Parsing errors are never fatal, so the parsing will resume after
   * the error. Errors may however cause parts of the given data or
   * even all of it to not be parsed at all. So it is a useful idea
   * to check that the parsing succeeds by connecting to this signal.
   *
   * Errors in the [enum@Bobgui.CssParserWarning] enumeration should not
   * be treated as fatal errors.
   *
   * Note that this signal may be emitted at any time as the css provider
   * may opt to defer parsing parts or all of the input to a later time
   * than when a loading function was called.
   */
  css_provider_signals[PARSING_ERROR] =
    g_signal_new (I_("parsing-error"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiCssProviderClass, parsing_error),
                  NULL, NULL,
                  _bobgui_marshal_VOID__BOXED_BOXED,
                  G_TYPE_NONE, 2, BOBGUI_TYPE_CSS_SECTION, G_TYPE_ERROR);
  g_signal_set_va_marshaller (css_provider_signals[PARSING_ERROR],
                              G_TYPE_FROM_CLASS (object_class),
                              _bobgui_marshal_VOID__BOXED_BOXEDv);

  /**
   * BobguiCssProvider:prefers-color-scheme:
   *
   * Define the color scheme used for rendering the user interface.
   *
   * The UI can be set to either [enum@Bobgui.InterfaceColorScheme.LIGHT],
   * or [enum@Bobgui.InterfaceColorScheme.DARK] mode. Other values will
   * be interpreted the same as [enum@Bobgui.InterfaceColorScheme.LIGHT].
   *
   * This setting is be available for media queries in CSS:
   *
   * ```css
   * @media (prefers-color-scheme: dark) {
   *   // some dark mode styling
   * }
   * ```
   *
   * Changing this setting will reload the style sheet.
   *
   * Since: 4.20
   */
  pspecs[PROP_PREFERS_COLOR_SCHEME] = g_param_spec_enum ("prefers-color-scheme", NULL, NULL,
                                                         BOBGUI_TYPE_INTERFACE_COLOR_SCHEME,
                                                         BOBGUI_INTERFACE_COLOR_SCHEME_DEFAULT,
                                                         BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiCssProvider:prefers-contrast:
   *
   * Define the contrast mode to use for the user interface.
   *
   * When set to [enum@Bobgui.InterfaceContrast.MORE] or
   * [enum@Bobgui.InterfaceContrast.LESS], the UI is rendered in
   * high or low contrast.
   *
   * When set to [enum@Bobgui.InterfaceContrast.NO_PREFERENCE] (the default),
   * the user interface will be rendered in default mode.
   *
   * This setting is be available for media queries in CSS:
   *
   * ```css
   * @media (prefers-contrast: more) {
   *   // some style with high contrast
   * }
   * ```
   *
   * Changing this setting will reload the style sheet.
   *
   * Since: 4.20
   */
  pspecs[PROP_PREFERS_CONTRAST] = g_param_spec_enum ("prefers-contrast", NULL, NULL,
                                                     BOBGUI_TYPE_INTERFACE_CONTRAST,
                                                     BOBGUI_INTERFACE_CONTRAST_NO_PREFERENCE,
                                                     BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiCssProvider:prefers-reduced-motion:
   *
   * Define the type of reduced motion to use for the user interface.
   *
   * When set to [enum@Bobgui.ReducedMotion.REDUCE] the UI is rendered in
   * with reduced motion animations.
   *
   * When set to [enum@Bobgui.ReducedMotion.NO_PREFERENCE] (the default),
   * the user interface will be rendered in default mode.
   *
   * This setting is be available for media queries in CSS:
   *
   * ```css
   * @media (prefers-reduced-motion: reduce) {
   *   // some style with reduced motion
   * }
   * ```
   *
   * Changing this setting will reload the style sheet.
   *
   * Since: 4.22
   */
  pspecs[PROP_PREFERS_REDUCED_MOTION] = g_param_spec_enum ("prefers-reduced-motion", NULL, NULL,
                                                           BOBGUI_TYPE_REDUCED_MOTION,
                                                           BOBGUI_REDUCED_MOTION_NO_PREFERENCE,
                                                           BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, pspecs);
}

static void
bobgui_css_ruleset_init_copy (BobguiCssRuleset       *new,
                           BobguiCssRuleset       *ruleset,
                           BobguiCssSelector      *selector)
{
  memcpy (new, ruleset, sizeof (BobguiCssRuleset));

  new->selector = selector;
  /* First copy takes over ownership */
  if (ruleset->owns_styles)
    ruleset->owns_styles = FALSE;
}

static void
bobgui_css_ruleset_clear (BobguiCssRuleset *ruleset)
{
  if (ruleset->owns_styles)
    {
      guint i;

      for (i = 0; i < ruleset->n_styles; i++)
        {
          bobgui_css_value_unref (ruleset->styles[i].value);
	  ruleset->styles[i].value = NULL;
	  if (ruleset->styles[i].section)
	    bobgui_css_section_unref (ruleset->styles[i].section);
        }
      g_free (ruleset->styles);
      if (ruleset->custom_properties)
        g_hash_table_unref (ruleset->custom_properties);
    }
  if (ruleset->selector)
    _bobgui_css_selector_free (ruleset->selector);

  memset (ruleset, 0, sizeof (BobguiCssRuleset));
}

static void
bobgui_css_ruleset_add (BobguiCssRuleset       *ruleset,
                     BobguiCssStyleProperty *property,
                     BobguiCssValue         *value,
                     BobguiCssSection       *section)
{
  guint i;

  g_return_if_fail (ruleset->owns_styles || (ruleset->n_styles == 0 && ruleset->custom_properties == NULL));

  ruleset->owns_styles = TRUE;

  for (i = 0; i < ruleset->n_styles; i++)
    {
      if (ruleset->styles[i].property == property)
        {
          bobgui_css_value_unref (ruleset->styles[i].value);
	  ruleset->styles[i].value = NULL;
	  if (ruleset->styles[i].section)
	    bobgui_css_section_unref (ruleset->styles[i].section);
          break;
        }
    }
  if (i == ruleset->n_styles)
    {
      ruleset->n_styles++;
      ruleset->styles = g_realloc (ruleset->styles, ruleset->n_styles * sizeof (PropertyValue));
      ruleset->styles[i].value = NULL;
      ruleset->styles[i].property = property;
    }

  ruleset->styles[i].value = value;
  if (bobgui_keep_css_sections)
    ruleset->styles[i].section = bobgui_css_section_ref (section);
  else
    ruleset->styles[i].section = NULL;
}

static void
unref_custom_property_name (gpointer pointer)
{
  BobguiCssCustomPropertyPool *pool = bobgui_css_custom_property_pool_get ();

  bobgui_css_custom_property_pool_unref (pool, GPOINTER_TO_INT (pointer));
}

static void
bobgui_css_ruleset_add_custom (BobguiCssRuleset       *ruleset,
                            const char          *name,
                            BobguiCssVariableValue *value)
{
  BobguiCssCustomPropertyPool *pool;
  int id;

  g_return_if_fail (ruleset->owns_styles || (ruleset->n_styles == 0 && ruleset->custom_properties == NULL));

  ruleset->owns_styles = TRUE;

  if (ruleset->custom_properties == NULL)
    {
      ruleset->custom_properties = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                          unref_custom_property_name,
                                                          (GDestroyNotify) bobgui_css_variable_value_unref);
    }

  pool = bobgui_css_custom_property_pool_get ();
  id = bobgui_css_custom_property_pool_add (pool, name);

  g_hash_table_replace (ruleset->custom_properties, GINT_TO_POINTER (id), value);
}

static void
bobgui_css_scanner_destroy (BobguiCssScanner *scanner)
{
  g_object_unref (scanner->provider);
  bobgui_css_parser_unref (scanner->parser);

  /* Discrete media features are all using static strings. */
  g_array_unref (scanner->media_features);

  g_free (scanner);
}

static void
bobgui_css_style_provider_emit_error (BobguiStyleProvider *provider,
                                   BobguiCssSection    *section,
                                   const GError     *error)
{
  g_signal_emit (provider, css_provider_signals[PARSING_ERROR], 0, section, error);
}

static void
bobgui_css_scanner_parser_error (BobguiCssParser         *parser,
                              const BobguiCssLocation *start,
                              const BobguiCssLocation *end,
                              const GError         *error,
                              gpointer              user_data)
{
  BobguiCssScanner *scanner = user_data;
  BobguiCssSection *section;

  section = bobgui_css_section_new_with_bytes (bobgui_css_parser_get_file (parser),
                                            bobgui_css_parser_get_bytes (parser),
                                            start,
                                            end);

  bobgui_css_style_provider_emit_error (BOBGUI_STYLE_PROVIDER (scanner->provider), section, error);

  bobgui_css_section_unref (section);
}

static BobguiCssScanner *
bobgui_css_scanner_new (BobguiCssProvider *provider,
                     BobguiCssScanner  *parent,
                     GFile          *file,
                     GBytes         *bytes)
{
  BobguiCssScanner *scanner;
  BobguiCssDiscreteMediaFeature feature;

  scanner = g_new0 (BobguiCssScanner, 1);

  g_object_ref (provider);
  scanner->provider = provider;
  scanner->parent = parent;

  scanner->parser = bobgui_css_parser_new_for_bytes (bytes,
                                                  file,
                                                  bobgui_css_scanner_parser_error,
                                                  scanner,
                                                  NULL);

  if (parent == NULL)
    {
      BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (provider);

      scanner->media_features = g_array_sized_new (FALSE, FALSE, sizeof (BobguiCssDiscreteMediaFeature), 3);

      feature.name = "prefers-color-scheme";

      switch (priv->prefers_color_scheme)
        {
        case BOBGUI_INTERFACE_COLOR_SCHEME_DEFAULT:
        case BOBGUI_INTERFACE_COLOR_SCHEME_LIGHT:
        case BOBGUI_INTERFACE_COLOR_SCHEME_UNSUPPORTED:
          feature.value = "light";
          break;

        case BOBGUI_INTERFACE_COLOR_SCHEME_DARK:
          feature.value = "dark";
          break;

        default:
          g_assert_not_reached ();
        }

      g_array_append_vals (scanner->media_features, &feature, 1);

      feature.name = "prefers-contrast";

      switch (priv->prefers_contrast)
        {
        case BOBGUI_INTERFACE_CONTRAST_NO_PREFERENCE:
        case BOBGUI_INTERFACE_CONTRAST_UNSUPPORTED:
          feature.value = "no-preference";
          break;

        case BOBGUI_INTERFACE_CONTRAST_MORE:
          feature.value = "more";
          break;

        case BOBGUI_INTERFACE_CONTRAST_LESS:
          feature.value = "less";
          break;

        default:
          g_assert_not_reached ();
        }

      g_array_append_vals (scanner->media_features, &feature, 1);

      feature.name = "prefers-reduced-motion";

      switch (priv->prefers_reduced_motion)
        {
        case BOBGUI_REDUCED_MOTION_NO_PREFERENCE:
          feature.value = "no-preference";
          break;

        case BOBGUI_REDUCED_MOTION_REDUCE:
          feature.value = "reduce";
          break;

        default:
          g_assert_not_reached ();
        }

      g_array_append_vals (scanner->media_features, &feature, 1);
    }
  else
    scanner->media_features = g_array_ref (parent->media_features);

  return scanner;
}

static gboolean
bobgui_css_scanner_would_recurse (BobguiCssScanner *scanner,
                               GFile         *file)
{
  while (scanner)
    {
      GFile *parser_file = bobgui_css_parser_get_file (scanner->parser);
      if (parser_file && g_file_equal (parser_file, file))
        return TRUE;

      scanner = scanner->parent;
    }

  return FALSE;
}

static gboolean
bobgui_css_scanner_should_commit (BobguiCssScanner *scanner)
{
  gboolean commit = (scanner->skip_count == 0);

  if (scanner->parent)
    commit &= bobgui_css_scanner_should_commit (scanner->parent);

  return commit;
}

static void
bobgui_css_provider_init (BobguiCssProvider *css_provider)
{
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (css_provider);

  priv->prefers_color_scheme = BOBGUI_INTERFACE_COLOR_SCHEME_DEFAULT;
  priv->prefers_contrast = BOBGUI_INTERFACE_CONTRAST_NO_PREFERENCE;
  priv->needs_rerender = FALSE;

  priv->rulesets = g_array_new (FALSE, FALSE, sizeof (BobguiCssRuleset));

  priv->symbolic_colors = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                 (GDestroyNotify) g_free,
                                                 (GDestroyNotify) bobgui_css_value_unref);
  priv->keyframes = g_hash_table_new_full (g_str_hash, g_str_equal,
                                           (GDestroyNotify) g_free,
                                           (GDestroyNotify) _bobgui_css_keyframes_unref);
}

static void
verify_tree_match_results (BobguiCssProvider        *provider,
                           BobguiCssNode            *node,
                           BobguiCssSelectorMatches *tree_rules)
{
#ifdef VERIFY_TREE
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (provider);
  BobguiCssRuleset *ruleset;
  gboolean should_match;
  int i, j;

  for (i = 0; i < priv->rulesets->len; i++)
    {
      gboolean found = FALSE;

      ruleset = &g_array_index (priv->rulesets, BobguiCssRuleset, i);

      for (j = 0; j < bobgui_css_selector_matches_get_size (tree_rules); j++)
	{
	  if (ruleset == bobgui_css_selector_matches_get (tree_rules, j))
	    {
	      found = TRUE;
	      break;
	    }
	}
      should_match = bobgui_css_selector_matches (ruleset->selector, node);
      if (found != !!should_match)
	{
	  g_error ("expected rule '%s' to %s, but it %s",
		   _bobgui_css_selector_to_string (ruleset->selector),
		   should_match ? "match" : "not match",
		   found ? "matched" : "didn't match");
	}
    }
#endif
}

static BobguiCssValue *
bobgui_css_style_provider_get_color (BobguiStyleProvider *provider,
                                  const char       *name)
{
  BobguiCssProvider *css_provider = BOBGUI_CSS_PROVIDER (provider);
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (css_provider);

  return g_hash_table_lookup (priv->symbolic_colors, name);
}

static BobguiCssKeyframes *
bobgui_css_style_provider_get_keyframes (BobguiStyleProvider *provider,
                                      const char       *name)
{
  BobguiCssProvider *css_provider = BOBGUI_CSS_PROVIDER (provider);
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (css_provider);

  return g_hash_table_lookup (priv->keyframes, name);
}

static void
bobgui_css_style_provider_lookup (BobguiStyleProvider             *provider,
                               const BobguiCountingBloomFilter *filter,
                               BobguiCssNode                   *node,
                               BobguiCssLookup                 *lookup,
                               BobguiCssChange                 *change)
{
  BobguiCssProvider *css_provider = BOBGUI_CSS_PROVIDER (provider);
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (css_provider);
  BobguiCssRuleset *ruleset;
  guint j;
  int i;
  BobguiCssSelectorMatches tree_rules;

  if (_bobgui_css_selector_tree_is_empty (priv->tree))
    return;

  bobgui_css_selector_matches_init (&tree_rules);
  _bobgui_css_selector_tree_match_all (priv->tree, filter, node, &tree_rules);

  if (!bobgui_css_selector_matches_is_empty (&tree_rules))
    {
      verify_tree_match_results (css_provider, node, &tree_rules);

      for (i = bobgui_css_selector_matches_get_size (&tree_rules) - 1; i >= 0; i--)
        {
          ruleset = bobgui_css_selector_matches_get (&tree_rules, i);

          if (ruleset->styles == NULL && ruleset->custom_properties == NULL)
            continue;

          for (j = 0; j < ruleset->n_styles; j++)
            {
              BobguiCssStyleProperty *prop = ruleset->styles[j].property;
              guint id = _bobgui_css_style_property_get_id (prop);

              if (!_bobgui_css_lookup_is_missing (lookup, id))
                continue;

              _bobgui_css_lookup_set (lookup,
                                   id,
                                   ruleset->styles[j].section,
                                   ruleset->styles[j].value);
            }

          if (ruleset->custom_properties)
            {
              GHashTableIter iter;
              gpointer id;
              BobguiCssVariableValue *value;

              g_hash_table_iter_init (&iter, ruleset->custom_properties);

              while (g_hash_table_iter_next (&iter, &id, (gpointer) &value))
                _bobgui_css_lookup_set_custom (lookup, GPOINTER_TO_INT (id), value);
            }
        }
    }
  bobgui_css_selector_matches_clear (&tree_rules);

  if (change)
    *change = bobgui_css_selector_tree_get_change_all (priv->tree, filter, node);
}

static gboolean
bobgui_css_style_provider_has_section (BobguiStyleProvider *provider,
                                    BobguiCssSection    *section)
{
  BobguiCssProvider *self = BOBGUI_CSS_PROVIDER (provider);
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (self);

  return priv->bytes == bobgui_css_section_get_bytes (section);
}

static void
bobgui_css_style_provider_iface_init (BobguiStyleProviderInterface *iface)
{
  iface->get_color = bobgui_css_style_provider_get_color;
  iface->get_keyframes = bobgui_css_style_provider_get_keyframes;
  iface->lookup = bobgui_css_style_provider_lookup;
  iface->emit_error = bobgui_css_style_provider_emit_error;
  iface->has_section = bobgui_css_style_provider_has_section;
}

static void
bobgui_css_provider_finalize (GObject *object)
{
  BobguiCssProvider *css_provider = BOBGUI_CSS_PROVIDER (object);
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (css_provider);
  guint i;

  for (i = 0; i < priv->rulesets->len; i++)
    bobgui_css_ruleset_clear (&g_array_index (priv->rulesets, BobguiCssRuleset, i));

  g_array_free (priv->rulesets, TRUE);
  _bobgui_css_selector_tree_free (priv->tree);

  g_hash_table_destroy (priv->symbolic_colors);
  g_hash_table_destroy (priv->keyframes);

  g_clear_pointer (&priv->source, g_bytes_unref);
  g_clear_object (&priv->source_file);

  if (priv->resource)
    {
      g_resources_unregister (priv->resource);
      g_resource_unref (priv->resource);
      priv->resource = NULL;
    }

  g_free (priv->path);

  G_OBJECT_CLASS (bobgui_css_provider_parent_class)->finalize (object);
}

static void
bobgui_css_provider_get_property (GObject         *object,
                               guint            prop_id,
                               GValue          *value,
                               GParamSpec      *pspec)
{
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (BOBGUI_CSS_PROVIDER (object));

  switch (prop_id)
    {
    case PROP_PREFERS_COLOR_SCHEME:
      g_value_set_enum (value, priv->prefers_color_scheme);
      break;
    case PROP_PREFERS_CONTRAST:
      g_value_set_enum (value, priv->prefers_contrast);
      break;
    case PROP_PREFERS_REDUCED_MOTION:
      g_value_set_enum (value, priv->prefers_reduced_motion);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_css_provider_set_property (GObject         *object,
                               guint            prop_id,
                               const GValue    *value,
                               GParamSpec      *pspec)
{
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (BOBGUI_CSS_PROVIDER (object));
  int enum_value;

  switch (prop_id)
    {
    case PROP_PREFERS_COLOR_SCHEME:
      enum_value = g_value_get_enum (value);
      if (priv->prefers_color_scheme != enum_value)
        {
          priv->prefers_color_scheme = enum_value;
          priv->needs_rerender = TRUE;
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_PREFERS_CONTRAST:
      enum_value = g_value_get_enum (value);
      if (priv->prefers_contrast != enum_value)
        {
          priv->prefers_contrast = enum_value;
          priv->needs_rerender = TRUE;
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_PREFERS_REDUCED_MOTION:
      enum_value = g_value_get_enum (value);
      if (priv->prefers_reduced_motion != enum_value)
        {
          priv->prefers_reduced_motion = enum_value;
          priv->needs_rerender = TRUE;
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
maybe_rerender_style_sheet (BobguiCssProvider *css_provider)
{
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (css_provider);

  if (priv->needs_rerender && priv->source != NULL)
    {
      GBytes *source = g_bytes_ref (priv->source);
      GFile *source_file = NULL;

      if (priv->source_file != NULL)
        source_file = g_object_ref (priv->source_file);

      bobgui_css_provider_reset (css_provider);
      bobgui_css_provider_load_internal (css_provider, NULL, source_file, source);

      priv->source = source;
      priv->source_file = source_file;

      bobgui_style_provider_changed (BOBGUI_STYLE_PROVIDER (css_provider));
    }

  priv->needs_rerender = FALSE;
}

static void
bobgui_css_provider_notify (GObject    *object,
                         GParamSpec *pspec)
{
  BobguiCssProvider *css_provider = BOBGUI_CSS_PROVIDER (object);

  switch (pspec->param_id)
    {
    case PROP_PREFERS_COLOR_SCHEME:
    case PROP_PREFERS_CONTRAST:
    case PROP_PREFERS_REDUCED_MOTION:
      maybe_rerender_style_sheet (css_provider);
      break;
    default:
      break;
    }
}

/**
 * bobgui_css_provider_new:
 *
 * Returns a newly created `BobguiCssProvider`.
 *
 * Returns: A new `BobguiCssProvider`
 */
BobguiCssProvider *
bobgui_css_provider_new (void)
{
  return g_object_new (BOBGUI_TYPE_CSS_PROVIDER, NULL);
}

static void
css_provider_commit (BobguiCssProvider  *css_provider,
                     BobguiCssSelectors *selectors,
                     BobguiCssRuleset   *ruleset)
{
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (css_provider);
  guint i;

  if (ruleset->styles == NULL && ruleset->custom_properties == NULL)
    {
      for (i = 0; i < bobgui_css_selectors_get_size (selectors); i++)
        _bobgui_css_selector_free (bobgui_css_selectors_get (selectors, i));
      return;
    }

  for (i = 0; i < bobgui_css_selectors_get_size (selectors); i++)
    {
      BobguiCssRuleset *new;

      g_array_set_size (priv->rulesets, priv->rulesets->len + 1);

      new = &g_array_index (priv->rulesets, BobguiCssRuleset, priv->rulesets->len - 1);
      bobgui_css_ruleset_init_copy (new, ruleset, bobgui_css_selectors_get (selectors, i));
    }
}

static void
bobgui_css_provider_reset (BobguiCssProvider *css_provider)
{
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (css_provider);
  guint i;

  g_clear_pointer (&priv->source, g_bytes_unref);
  g_clear_object (&priv->source_file);

  if (priv->resource)
    {
      g_resources_unregister (priv->resource);
      g_resource_unref (priv->resource);
      priv->resource = NULL;
    }

  if (priv->path)
    {
      g_free (priv->path);
      priv->path = NULL;
    }

  g_hash_table_remove_all (priv->symbolic_colors);
  g_hash_table_remove_all (priv->keyframes);

  for (i = 0; i < priv->rulesets->len; i++)
    bobgui_css_ruleset_clear (&g_array_index (priv->rulesets, BobguiCssRuleset, i));
  g_array_set_size (priv->rulesets, 0);
  _bobgui_css_selector_tree_free (priv->tree);
  priv->tree = NULL;
}

static gboolean
parse_import (BobguiCssScanner *scanner)
{
  GFile *file;

  if (!bobgui_css_parser_try_at_keyword (scanner->parser, "import"))
    return FALSE;

  if (bobgui_css_parser_has_token (scanner->parser, BOBGUI_CSS_TOKEN_STRING))
    {
      char *url;

      url = bobgui_css_parser_consume_string (scanner->parser);
      if (url)
        {
          file = bobgui_css_parser_resolve_url (scanner->parser, url);
          if (file == NULL)
            {
              bobgui_css_parser_error_import (scanner->parser,
                                           "Could not resolve \"%s\" to a valid URL",
                                           url);
            }
          g_free (url);
        }
      else
        file = NULL;
    }
  else
    {
      char *url = bobgui_css_parser_consume_url (scanner->parser);
      if (url)
        {
          file = bobgui_css_parser_resolve_url (scanner->parser, url);
          g_free (url);
        }
      else
        file = NULL;
    }

  if (file == NULL)
    {
      /* nothing to do */
    }
  else if (!bobgui_css_parser_has_token (scanner->parser, BOBGUI_CSS_TOKEN_EOF))
    {
      bobgui_css_parser_error_syntax (scanner->parser, "Expected ';'");
    }
  else if (!bobgui_css_scanner_should_commit (scanner))
    {
      /* nothing to do */
    }
  else if (bobgui_css_scanner_would_recurse (scanner, file))
    {
       char *path = g_file_get_path (file);
       bobgui_css_parser_error (scanner->parser,
                             BOBGUI_CSS_PARSER_ERROR_IMPORT,
                             bobgui_css_parser_get_block_location (scanner->parser),
                             bobgui_css_parser_get_end_location (scanner->parser),
                             "Loading '%s' would recurse",
                             path);
       g_free (path);
    }
  else
    {
      GError *load_error = NULL;

      GBytes *bytes = g_file_load_bytes (file, NULL, NULL, &load_error);

      if (bytes == NULL)
        {
          bobgui_css_parser_error (scanner->parser,
                                BOBGUI_CSS_PARSER_ERROR_IMPORT,
                                bobgui_css_parser_get_block_location (scanner->parser),
                                bobgui_css_parser_get_end_location (scanner->parser),
                                "Failed to import: %s",
                                load_error->message);
          g_error_free (load_error);
        }
      else
        {
          bobgui_css_provider_load_internal (scanner->provider,
                                          scanner,
                                          file,
                                          bytes);
          g_bytes_unref (bytes);
        }
    }

  g_clear_object (&file);

  return TRUE;
}

static gboolean
parse_media_block (BobguiCssScanner *scanner)
{
  gboolean is_match = TRUE;

  if (!bobgui_css_parser_try_at_keyword (scanner->parser, "media"))
    return FALSE;

  if (!bobgui_css_parser_has_token (scanner->parser, BOBGUI_CSS_TOKEN_OPEN_CURLY))
    {
      is_match = bobgui_css_media_query_parse (scanner->parser, scanner->media_features);
    }

  if (!bobgui_css_parser_has_token (scanner->parser, BOBGUI_CSS_TOKEN_OPEN_CURLY))
    {
      bobgui_css_parser_error_syntax (scanner->parser, "Expected '{' after @media query");
      return FALSE;
    }

  bobgui_css_parser_start_block (scanner->parser);

  if (!is_match)
    scanner->skip_count += 1;

  while (!bobgui_css_parser_has_token (scanner->parser, BOBGUI_CSS_TOKEN_CLOSE_CURLY) &&
         !bobgui_css_parser_has_token (scanner->parser, BOBGUI_CSS_TOKEN_EOF))
    parse_statement (scanner);

  if (!is_match)
    scanner->skip_count -= 1;

  bobgui_css_parser_end_block (scanner->parser);

  return TRUE;
}

static gboolean
parse_color_definition (BobguiCssScanner *scanner)
{
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (scanner->provider);
  BobguiCssValue *color;
  char *name;

  if (!bobgui_css_parser_try_at_keyword (scanner->parser, "define-color"))
    return FALSE;

  name = bobgui_css_parser_consume_ident (scanner->parser);
  if (name == NULL)
    return TRUE;

  color = bobgui_css_color_value_parse (scanner->parser);
  if (color == NULL)
    {
      g_free (name);
      return TRUE;
    }

  if (!bobgui_css_parser_has_token (scanner->parser, BOBGUI_CSS_TOKEN_EOF))
    {
      g_free (name);
      bobgui_css_value_unref (color);
      bobgui_css_parser_error_syntax (scanner->parser,
                                   "Missing semicolon at end of color definition");
      return TRUE;
    }

  if (bobgui_css_scanner_should_commit (scanner))
    g_hash_table_insert (priv->symbolic_colors, name, color);
  else
    {
      bobgui_css_value_unref (color);
      g_free (name);
    }

  return TRUE;
}

static gboolean
parse_keyframes (BobguiCssScanner *scanner)
{
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (scanner->provider);
  BobguiCssKeyframes *keyframes;
  char *name;

  if (!bobgui_css_parser_try_at_keyword (scanner->parser, "keyframes"))
    return FALSE;

  name = bobgui_css_parser_consume_ident (scanner->parser);
  if (name == NULL)
    return FALSE;

  if (!bobgui_css_parser_has_token (scanner->parser, BOBGUI_CSS_TOKEN_EOF))
    {
      bobgui_css_parser_error_syntax (scanner->parser, "Expected '{' for keyframes");
      return FALSE;
    }

  bobgui_css_parser_end_block_prelude (scanner->parser);

  keyframes = _bobgui_css_keyframes_parse (scanner->parser);
  if (keyframes != NULL)
    {
      if (bobgui_css_scanner_should_commit (scanner))
        g_hash_table_insert (priv->keyframes, name, keyframes);
      else
        _bobgui_css_keyframes_unref (keyframes);
    }

  if (!bobgui_css_parser_has_token (scanner->parser, BOBGUI_CSS_TOKEN_EOF))
    bobgui_css_parser_error_syntax (scanner->parser, "Expected '}' after declarations");

  return TRUE;
}

static void
parse_at_keyword (BobguiCssScanner *scanner)
{
  if (parse_media_block (scanner))
    return;

  bobgui_css_parser_start_semicolon_block (scanner->parser, BOBGUI_CSS_TOKEN_OPEN_CURLY);

  if (!parse_import (scanner) &&
      !parse_color_definition (scanner) &&
      !parse_keyframes (scanner))
    {
      bobgui_css_parser_error_syntax (scanner->parser, "Unknown @ rule");
    }

  bobgui_css_parser_end_block (scanner->parser);
}

static void
parse_selector_list (BobguiCssScanner   *scanner,
                     BobguiCssSelectors *selectors)
{
  do {
      BobguiCssSelector *select = _bobgui_css_selector_parse (scanner->parser);

      if (select == NULL)
        {
          for (int i = 0; i < bobgui_css_selectors_get_size (selectors); i++)
            _bobgui_css_selector_free (bobgui_css_selectors_get (selectors, i));
          bobgui_css_selectors_clear (selectors);
          return;
        }

      bobgui_css_selectors_append (selectors, select);
    }
  while (bobgui_css_parser_try_token (scanner->parser, BOBGUI_CSS_TOKEN_COMMA));
}

static void
parse_declaration (BobguiCssScanner *scanner,
                   BobguiCssRuleset *ruleset)
{
  BobguiStyleProperty *property;
  char *name;

  /* advance the location over whitespace */
  bobgui_css_parser_get_token (scanner->parser);
  bobgui_css_parser_start_semicolon_block (scanner->parser, BOBGUI_CSS_TOKEN_EOF);

  if (bobgui_css_parser_has_token (scanner->parser, BOBGUI_CSS_TOKEN_EOF))
    {
      bobgui_css_parser_warn_syntax (scanner->parser, "Empty declaration");
      bobgui_css_parser_end_block (scanner->parser);
      return;
    }

  name = bobgui_css_parser_consume_ident (scanner->parser);
  if (name == NULL)
    goto out;

  /* This is a custom property */
  if (name[0] == '-' && name[1] == '-')
    {
      BobguiCssVariableValue *value;
      BobguiCssLocation start_location;
      BobguiCssSection *section;

      if (!bobgui_css_parser_try_token (scanner->parser, BOBGUI_CSS_TOKEN_COLON))
        {
          bobgui_css_parser_error_syntax (scanner->parser, "Expected ':'");
          goto out;
        }

      bobgui_css_parser_skip_whitespace (scanner->parser);

      if (bobgui_keep_css_sections)
        start_location = *bobgui_css_parser_get_start_location (scanner->parser);

      value = bobgui_css_parser_parse_value_into_token_stream (scanner->parser);
      if (value == NULL)
        goto out;

      if (bobgui_keep_css_sections)
        {
          section = bobgui_css_section_new_with_bytes (bobgui_css_parser_get_file (scanner->parser),
                                                    bobgui_css_parser_get_bytes (scanner->parser),
                                                    &start_location,
                                                    bobgui_css_parser_get_start_location (scanner->parser));
        }
      else
        section = NULL;

      if (section != NULL)
        {
          bobgui_css_variable_value_set_section (value, section);
          bobgui_css_section_unref (section);
        }

      bobgui_css_ruleset_add_custom (ruleset, name, value);

      goto out;
    }

  property = _bobgui_style_property_lookup (name);

  if (property)
    {
      BobguiCssSection *section;
      BobguiCssValue *value;

      if (!bobgui_css_parser_try_token (scanner->parser, BOBGUI_CSS_TOKEN_COLON))
        {
          bobgui_css_parser_error_syntax (scanner->parser, "Expected ':'");
          goto out;
        }

      if (bobgui_css_parser_has_references (scanner->parser))
        {
          BobguiCssLocation start_location;
          BobguiCssVariableValue *var_value;

          bobgui_css_parser_skip_whitespace (scanner->parser);

          if (bobgui_keep_css_sections)
            start_location = *bobgui_css_parser_get_start_location (scanner->parser);

          var_value = bobgui_css_parser_parse_value_into_token_stream (scanner->parser);
          if (var_value == NULL)
            goto out;

          if (bobgui_keep_css_sections)
            section = bobgui_css_section_new_with_bytes (bobgui_css_parser_get_file (scanner->parser),
                                                      bobgui_css_parser_get_bytes (scanner->parser),
                                                      &start_location,
                                                      bobgui_css_parser_get_start_location (scanner->parser));
          else
            section = NULL;

          if (section != NULL)
            {
              bobgui_css_variable_value_set_section (var_value, section);
              bobgui_css_section_unref (section);
            }

          if (BOBGUI_IS_CSS_SHORTHAND_PROPERTY (property))
            {
              BobguiCssShorthandProperty *shorthand = BOBGUI_CSS_SHORTHAND_PROPERTY (property);
              guint i, n;
              BobguiCssValue **values;

              n = _bobgui_css_shorthand_property_get_n_subproperties (shorthand);

              values = g_new (BobguiCssValue *, n);

              for (i = 0; i < n; i++)
                {
                  BobguiCssValue *child =
                    _bobgui_css_reference_value_new (property,
                                                  var_value,
                                                  bobgui_css_parser_get_file (scanner->parser));
                  _bobgui_css_reference_value_set_subproperty (child, i);

                  values[i] = _bobgui_css_array_value_get_nth (child, i);
                }

              value = _bobgui_css_array_value_new_from_array (values, n);
              g_free (values);
            }
          else
            {
              value = _bobgui_css_reference_value_new (property,
                                                    var_value,
                                                    bobgui_css_parser_get_file (scanner->parser));
            }

          bobgui_css_variable_value_unref (var_value);
        }
      else
        {
          value = _bobgui_style_property_parse_value (property, scanner->parser);

          if (value == NULL)
            goto out;

          if (!bobgui_css_parser_has_token (scanner->parser, BOBGUI_CSS_TOKEN_EOF))
            {
              bobgui_css_parser_error_syntax (scanner->parser, "Junk at end of value for %s", property->name);
              bobgui_css_value_unref (value);
              goto out;
            }
        }

      if (bobgui_keep_css_sections)
        {
          section = bobgui_css_section_new_with_bytes (bobgui_css_parser_get_file (scanner->parser),
                                                    bobgui_css_parser_get_bytes (scanner->parser),
                                                    bobgui_css_parser_get_block_location (scanner->parser),
                                                    bobgui_css_parser_get_end_location (scanner->parser));
        }
      else
        section = NULL;

      if (BOBGUI_IS_CSS_SHORTHAND_PROPERTY (property))
        {
          BobguiCssShorthandProperty *shorthand = BOBGUI_CSS_SHORTHAND_PROPERTY (property);
          guint i;

          for (i = 0; i < _bobgui_css_shorthand_property_get_n_subproperties (shorthand); i++)
            {
              BobguiCssStyleProperty *child = _bobgui_css_shorthand_property_get_subproperty (shorthand, i);
              BobguiCssValue *sub = _bobgui_css_array_value_get_nth (value, i);

              bobgui_css_ruleset_add (ruleset, child, bobgui_css_value_ref (sub), section);
            }

            bobgui_css_value_unref (value);
        }
      else if (BOBGUI_IS_CSS_STYLE_PROPERTY (property))
        {

          bobgui_css_ruleset_add (ruleset, BOBGUI_CSS_STYLE_PROPERTY (property), value, section);
        }
      else
        {
          g_assert_not_reached ();
          bobgui_css_value_unref (value);
        }

      g_clear_pointer (&section, bobgui_css_section_unref);
    }
  else
    {
      bobgui_css_parser_error_value (scanner->parser, "No property named \"%s\"", name);
    }

out:
  g_free (name);

  bobgui_css_parser_end_block (scanner->parser);
}

static void
parse_declarations (BobguiCssScanner *scanner,
                    BobguiCssRuleset *ruleset)
{
  while (!bobgui_css_parser_has_token (scanner->parser, BOBGUI_CSS_TOKEN_EOF))
    {
      parse_declaration (scanner, ruleset);
    }
}

static void
parse_ruleset (BobguiCssScanner *scanner)
{
  BobguiCssSelectors selectors;
  BobguiCssRuleset ruleset = { 0, };

  bobgui_css_selectors_init (&selectors);

  parse_selector_list (scanner, &selectors);
  if (bobgui_css_selectors_get_size (&selectors) == 0)
    {
      bobgui_css_parser_skip_until (scanner->parser, BOBGUI_CSS_TOKEN_OPEN_CURLY);
      bobgui_css_parser_skip (scanner->parser);
      goto out;
    }

  if (!bobgui_css_parser_has_token (scanner->parser, BOBGUI_CSS_TOKEN_OPEN_CURLY))
    {
      guint i;
      bobgui_css_parser_error_syntax (scanner->parser, "Expected '{' after selectors");
      for (i = 0; i < bobgui_css_selectors_get_size (&selectors); i++)
        _bobgui_css_selector_free (bobgui_css_selectors_get (&selectors, i));
      bobgui_css_parser_skip_until (scanner->parser, BOBGUI_CSS_TOKEN_OPEN_CURLY);
      bobgui_css_parser_skip (scanner->parser);
      goto out;
    }

  bobgui_css_parser_start_block (scanner->parser);

  parse_declarations (scanner, &ruleset);

  bobgui_css_parser_end_block (scanner->parser);

  if (bobgui_css_scanner_should_commit (scanner))
    css_provider_commit (scanner->provider, &selectors, &ruleset);
  else
    {
      guint i;

      for (i = 0; i < bobgui_css_selectors_get_size (&selectors); i++)
        _bobgui_css_selector_free (bobgui_css_selectors_get (&selectors, i));
    }

  bobgui_css_ruleset_clear (&ruleset);

out:
  bobgui_css_selectors_clear (&selectors);
}

static void
parse_statement (BobguiCssScanner *scanner)
{
  if (bobgui_css_parser_has_token (scanner->parser, BOBGUI_CSS_TOKEN_AT_KEYWORD))
    parse_at_keyword (scanner);
  else
    parse_ruleset (scanner);
}

static void
parse_stylesheet (BobguiCssScanner *scanner)
{
  while (!bobgui_css_parser_has_token (scanner->parser, BOBGUI_CSS_TOKEN_EOF))
    {
      if (bobgui_css_parser_has_token (scanner->parser, BOBGUI_CSS_TOKEN_CDO) ||
          bobgui_css_parser_has_token (scanner->parser, BOBGUI_CSS_TOKEN_CDC))
        {
          bobgui_css_parser_consume_token (scanner->parser);
          continue;
        }

      parse_statement (scanner);
    }
}

static int
bobgui_css_provider_compare_rule (gconstpointer a_,
                               gconstpointer b_)
{
  const BobguiCssRuleset *a = (const BobguiCssRuleset *) a_;
  const BobguiCssRuleset *b = (const BobguiCssRuleset *) b_;
  int compare;

  compare = _bobgui_css_selector_compare (a->selector, b->selector);
  if (compare != 0)
    return compare;

  return 0;
}

static void
bobgui_css_provider_postprocess (BobguiCssProvider *css_provider)
{
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (css_provider);
  BobguiCssSelectorTreeBuilder *builder;
  guint i;
  gint64 before G_GNUC_UNUSED;

  before = GDK_PROFILER_CURRENT_TIME;

  g_array_sort (priv->rulesets, bobgui_css_provider_compare_rule);

  builder = _bobgui_css_selector_tree_builder_new ();
  for (i = 0; i < priv->rulesets->len; i++)
    {
      BobguiCssRuleset *ruleset;

      ruleset = &g_array_index (priv->rulesets, BobguiCssRuleset, i);

      _bobgui_css_selector_tree_builder_add (builder,
					  ruleset->selector,
					  &ruleset->selector_match,
					  ruleset);
    }

  priv->tree = _bobgui_css_selector_tree_builder_build (builder);
  _bobgui_css_selector_tree_builder_free (builder);

#ifndef VERIFY_TREE
  for (i = 0; i < priv->rulesets->len; i++)
    {
      BobguiCssRuleset *ruleset;

      ruleset = &g_array_index (priv->rulesets, BobguiCssRuleset, i);

      _bobgui_css_selector_free (ruleset->selector);
      ruleset->selector = NULL;
    }
#endif

  gdk_profiler_end_mark (before, "Create CSS selector tree", NULL);
}

static void
bobgui_css_provider_load_internal (BobguiCssProvider *self,
                                BobguiCssScanner  *parent,
                                GFile          *file,
                                GBytes         *bytes)
{
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (self);
  gint64 before G_GNUC_UNUSED;
  BobguiCssScanner *scanner;

  before = GDK_PROFILER_CURRENT_TIME;

  priv->bytes = bytes;

  scanner = bobgui_css_scanner_new (self,
                                  parent,
                                  file,
                                  bytes);

  parse_stylesheet (scanner);

  bobgui_css_scanner_destroy (scanner);

  if (parent == NULL)
    bobgui_css_provider_postprocess (self);

  if (GDK_PROFILER_IS_RUNNING)
    {
      const char *uri G_GNUC_UNUSED;
      uri = file ? g_file_peek_path (file) : NULL;
      gdk_profiler_end_mark (before, "CSS theme load", uri);
    }
}

/**
 * bobgui_css_provider_load_from_data:
 * @css_provider: a `BobguiCssProvider`
 * @data: CSS data to be parsed
 * @length: the length of @data in bytes, or -1 for NUL terminated strings
 *
 * Loads @data into @css_provider.
 *
 * This clears any previously loaded information.
 *
 * Deprecated: 4.12: Use [method@Bobgui.CssProvider.load_from_string]
 *   or [method@Bobgui.CssProvider.load_from_bytes] instead
 */
void
bobgui_css_provider_load_from_data (BobguiCssProvider  *css_provider,
                                 const char      *data,
                                 gssize           length)
{
  GBytes *bytes;

  g_return_if_fail (BOBGUI_IS_CSS_PROVIDER (css_provider));
  g_return_if_fail (data != NULL);

  if (length < 0)
    length = strlen (data);

  bytes = g_bytes_new (data, length);

  bobgui_css_provider_load_from_bytes (css_provider, bytes);

  g_bytes_unref (bytes);
}

/**
 * bobgui_css_provider_load_from_string:
 * @css_provider: a `BobguiCssProvider`
 * @string: the CSS to load
 *
 * Loads @string into @css_provider.
 *
 * This clears any previously loaded information.
 *
 * Since: 4.12
 */
void
bobgui_css_provider_load_from_string (BobguiCssProvider *css_provider,
                                   const char     *string)
{
  GBytes *bytes;

  g_return_if_fail (BOBGUI_IS_CSS_PROVIDER (css_provider));
  g_return_if_fail (string != NULL);

  bytes = g_bytes_new (string, strlen (string));

  bobgui_css_provider_load_from_bytes (css_provider, bytes);

  g_bytes_unref (bytes);
}

/**
 * bobgui_css_provider_load_from_bytes:
 * @css_provider: a `BobguiCssProvider`
 * @data: `GBytes` containing the data to load
 *
 * Loads @data into @css_provider.
 *
 * This clears any previously loaded information.
 *
 * Since: 4.12
 */
void
bobgui_css_provider_load_from_bytes (BobguiCssProvider *css_provider,
                                  GBytes         *data)
{
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (css_provider);
  g_return_if_fail (BOBGUI_IS_CSS_PROVIDER (css_provider));
  g_return_if_fail (data != NULL);

  bobgui_css_provider_reset (css_provider);

  bobgui_css_provider_load_internal (css_provider, NULL, NULL, data);

  priv->source = g_bytes_ref (data);
  priv->source_file = NULL;

  bobgui_style_provider_changed (BOBGUI_STYLE_PROVIDER (css_provider));
}

/**
 * bobgui_css_provider_load_from_file:
 * @css_provider: a `BobguiCssProvider`
 * @file: `GFile` pointing to a file to load
 *
 * Loads the data contained in @file into @css_provider.
 *
 * This clears any previously loaded information.
 */
void
bobgui_css_provider_load_from_file (BobguiCssProvider  *css_provider,
                                 GFile           *file)
{
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (css_provider);
  GBytes *bytes;

  g_return_if_fail (BOBGUI_IS_CSS_PROVIDER (css_provider));
  g_return_if_fail (G_IS_FILE (file));

  bobgui_css_provider_reset (css_provider);

  GError *load_error = NULL;

  bytes = g_file_load_bytes (file, NULL, NULL, &load_error);

  if (load_error != NULL)
    {
      BobguiCssLocation empty = { 0, };
      BobguiCssSection *section = bobgui_css_section_new (file, &empty, &empty);

      bobgui_css_style_provider_emit_error (BOBGUI_STYLE_PROVIDER (css_provider), section, load_error);
      bobgui_css_section_unref (section);

      g_clear_pointer (&priv->bytes, g_bytes_unref);
      g_error_free (load_error);
    }
  else
    {
      bobgui_css_provider_load_internal (css_provider, NULL, file, bytes);

      priv->source = bytes;
      priv->source_file = g_object_ref (file);
    }

  bobgui_style_provider_changed (BOBGUI_STYLE_PROVIDER (css_provider));
}

/**
 * bobgui_css_provider_load_from_path:
 * @css_provider: a `BobguiCssProvider`
 * @path: (type filename): the path of a filename to load, in the GLib filename encoding
 *
 * Loads the data contained in @path into @css_provider.
 *
 * This clears any previously loaded information.
 */
void
bobgui_css_provider_load_from_path (BobguiCssProvider  *css_provider,
                                 const char      *path)
{
  GFile *file;

  g_return_if_fail (BOBGUI_IS_CSS_PROVIDER (css_provider));
  g_return_if_fail (path != NULL);

  file = g_file_new_for_path (path);

  bobgui_css_provider_load_from_file (css_provider, file);

  g_object_unref (file);
}

/**
 * bobgui_css_provider_load_from_resource:
 * @css_provider: a `BobguiCssProvider`
 * @resource_path: a `GResource` resource path
 *
 * Loads the data contained in the resource at @resource_path into
 * the @css_provider.
 *
 * This clears any previously loaded information.
 */
void
bobgui_css_provider_load_from_resource (BobguiCssProvider *css_provider,
			             const char     *resource_path)
{
  GFile *file;
  char *uri, *escaped;

  g_return_if_fail (BOBGUI_IS_CSS_PROVIDER (css_provider));
  g_return_if_fail (resource_path != NULL);

  escaped = g_uri_escape_string (resource_path,
				 G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, FALSE);
  uri = g_strconcat ("resource://", escaped, NULL);
  g_free (escaped);

  file = g_file_new_for_uri (uri);
  g_free (uri);

  bobgui_css_provider_load_from_file (css_provider, file);

  g_object_unref (file);
}

char *
_bobgui_get_theme_dir (void)
{
  const char *var;

  var = g_getenv ("BOBGUI_DATA_PREFIX");
  if (var == NULL)
    var = _bobgui_get_data_prefix ();
  return g_build_filename (var, "share", "themes", NULL);
}

/* Return the path that this providers bobgui.css was loaded from,
 * if it is part of a theme, otherwise NULL.
 */
const char *
_bobgui_css_provider_get_theme_dir (BobguiCssProvider *provider)
{
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (provider);

  return priv->path;
}

#if (BOBGUI_MINOR_VERSION % 2)
#define MINOR (BOBGUI_MINOR_VERSION + 1)
#else
#define MINOR BOBGUI_MINOR_VERSION
#endif

/*
 * Look for
 * $dir/$subdir/bobgui-4.16/$file
 * $dir/$subdir/bobgui-4.14/$file
 *  ...
 * $dir/$subdir/bobgui-4.0/$file
 * and return the first found file.
 */
static char *
_bobgui_css_find_theme_dir (const char *dir,
                         const char *subdir,
                         const char *name,
                         const char *file)
{
  char *path;
  char *base;

  if (subdir)
    base = g_build_filename (dir, subdir, name, NULL);
  else
    base = g_build_filename (dir, name, NULL);

  if (!g_file_test (base, G_FILE_TEST_IS_DIR))
    {
      g_free (base);
      return NULL;
    }

  for (int i = MINOR; i >= 0; i = i - 2)
    {
      char subsubdir[64];

      g_snprintf (subsubdir, sizeof (subsubdir), "bobgui-4.%d", i);
      path = g_build_filename (base, subsubdir, file, NULL);

      if (g_file_test (path, G_FILE_TEST_EXISTS))
        break;

      g_free (path);
      path = NULL;
    }

  g_free (base);

  return path;
}

#undef MINOR

static char *
_bobgui_css_find_theme (const char *name,
                     const char *variant)
{
  char file[256];
  char *path;
  const char *const *dirs;
  int i;
  char *dir;

  if (variant && *variant)
    g_snprintf (file, sizeof (file), "bobgui-%s.css", variant);
  else
    strcpy (file, "bobgui.css");

  /* First look in the user's data directory */
  path = _bobgui_css_find_theme_dir (g_get_user_data_dir (), "themes", name, file);
  if (path)
    return path;

  /* Next look in the user's home directory */
  path = _bobgui_css_find_theme_dir (g_get_home_dir (), ".themes", name, file);
  if (path)
    {
      BOBGUI_DEBUG (CSS, "Loading custom CSS from $HOME/.themes/ is deprecated");
      return path;
    }

  /* Look in system data directories */
  dirs = g_get_system_data_dirs ();
  for (i = 0; dirs[i]; i++)
    {
      path = _bobgui_css_find_theme_dir (dirs[i], "themes", name, file);
      if (path)
        return path;
    }

  /* Finally, try in the default theme directory */
  dir = _bobgui_get_theme_dir ();
  path = _bobgui_css_find_theme_dir (dir, NULL, name, file);
  g_free (dir);

  return path;
}

/**
 * bobgui_css_provider_load_named:
 * @provider: a `BobguiCssProvider`
 * @name: A theme name
 * @variant: (nullable): variant to load, for example, "dark", or
 *   %NULL for the default
 *
 * Loads a theme from the usual theme paths.
 *
 * The actual process of finding the theme might change between
 * releases, but it is guaranteed that this function uses the same
 * mechanism to load the theme that BOBGUI uses for loading its own theme.
 *
 * Deprecated: 4.20: Using any of the other theme loaders, combine with media queries.
 */
void
bobgui_css_provider_load_named (BobguiCssProvider *provider,
                             const char     *name,
                             const char     *variant)
{
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (provider);
  char *path;
  char *resource_path;

  g_return_if_fail (BOBGUI_IS_CSS_PROVIDER (provider));
  g_return_if_fail (name != NULL);

  bobgui_css_provider_reset (provider);

  if (variant != NULL)
    {
      if (strstr (variant, "dark") != NULL)
        priv->prefers_color_scheme = BOBGUI_INTERFACE_COLOR_SCHEME_DARK;
      else
        priv->prefers_color_scheme = BOBGUI_INTERFACE_COLOR_SCHEME_LIGHT;

      if (strstr (variant, "hc") != NULL)
        priv->prefers_contrast = BOBGUI_INTERFACE_CONTRAST_MORE;
      else
        priv->prefers_contrast = BOBGUI_INTERFACE_CONTRAST_NO_PREFERENCE;
    }
  else
    {
      priv->prefers_color_scheme = BOBGUI_INTERFACE_COLOR_SCHEME_DEFAULT;
      priv->prefers_contrast = BOBGUI_INTERFACE_CONTRAST_NO_PREFERENCE;
    }

  /* try loading the resource for the theme. This is mostly meant for built-in
   * themes.
   */
  if (variant && *variant)
    resource_path = g_strdup_printf ("/org/bobgui/libbobgui/theme/%s/bobgui-%s.css", name, variant);
  else
    resource_path = g_strdup_printf ("/org/bobgui/libbobgui/theme/%s/bobgui.css", name);

  if (g_resources_get_info (resource_path, 0, NULL, NULL, NULL))
    {
      bobgui_css_provider_load_from_resource (provider, resource_path);
      g_free (resource_path);
      return;
    }
  g_free (resource_path);

  /* Next try looking for files in the various theme directories. */
  path = _bobgui_css_find_theme (name, variant);
  if (path)
    {
      char *dir, *resource_file;
      GResource *resource;

      dir = g_path_get_dirname (path);
      resource_file = g_build_filename (dir, "bobgui.gresource", NULL);
      resource = g_resource_load (resource_file, NULL);
      g_free (resource_file);

      if (resource != NULL)
        g_resources_register (resource);

      bobgui_css_provider_load_from_path (provider, path);

      /* Only set this after load, as load_from_path will clear it */
      priv->resource = resource;
      priv->path = dir;

      g_free (path);
    }
  else
    {
      /* Things failed! Fall back! Fall back!
       *
       * We accept the names HighContrast, HighContrastInverse,
       * Adwaita and Adwaita-dark as aliases for the variants
       * of the Default theme.
       */
      if (strcmp (name, "HighContrast") == 0)
        {
          if (g_strcmp0 (variant, "dark") == 0)
            bobgui_css_provider_load_named (provider, DEFAULT_THEME_NAME, "hc-dark");
          else
            bobgui_css_provider_load_named (provider, DEFAULT_THEME_NAME, "hc");
        }
      else if (strcmp (name, "HighContrastInverse") == 0)
        bobgui_css_provider_load_named (provider, DEFAULT_THEME_NAME, "hc-dark");
      else if (strcmp (name, "Adwaita-dark") == 0)
        bobgui_css_provider_load_named (provider, DEFAULT_THEME_NAME, "dark");
      else if (strcmp (name, DEFAULT_THEME_NAME) != 0)
        bobgui_css_provider_load_named (provider, DEFAULT_THEME_NAME, variant);
      else
        {
          g_return_if_fail (variant != NULL);
          bobgui_css_provider_load_named (provider, DEFAULT_THEME_NAME, NULL);
        }
    }
}

static int
compare_properties (gconstpointer a, gconstpointer b, gpointer style)
{
  const guint *ua = a;
  const guint *ub = b;
  PropertyValue *styles = style;

  return strcmp (_bobgui_style_property_get_name (BOBGUI_STYLE_PROPERTY (styles[*ua].property)),
                 _bobgui_style_property_get_name (BOBGUI_STYLE_PROPERTY (styles[*ub].property)));
}

/* This is looking into a GPtrArray where each "pointer" is actually
 * GINT_TO_POINTER (id), so a and b are pointers to pointer-sized quantities */
static int
compare_custom_properties (gconstpointer a, gconstpointer b, gpointer user_data)
{
  BobguiCssCustomPropertyPool *pool = user_data;
  const void * const *ap = a;
  const void * const *bp = b;
  const char *name1, *name2;

  name1 = bobgui_css_custom_property_pool_get_name (pool, GPOINTER_TO_INT (*ap));
  name2 = bobgui_css_custom_property_pool_get_name (pool, GPOINTER_TO_INT (*bp));

  return strcmp (name1, name2);
}

static void
bobgui_css_ruleset_print (const BobguiCssRuleset *ruleset,
                       GString             *str)
{
  guint i;

  _bobgui_css_selector_tree_match_print (ruleset->selector_match, str);

  g_string_append (str, " {\n");

  if (ruleset->styles)
    {
      guint *sorted = g_new (guint, ruleset->n_styles);

      for (i = 0; i < ruleset->n_styles; i++)
        sorted[i] = i;

      /* so the output is identical for identical selector styles */
      g_sort_array (sorted, ruleset->n_styles, sizeof (guint), compare_properties, ruleset->styles);

      for (i = 0; i < ruleset->n_styles; i++)
        {
          PropertyValue *prop = &ruleset->styles[sorted[i]];
          g_string_append (str, "  ");
          g_string_append (str, _bobgui_style_property_get_name (BOBGUI_STYLE_PROPERTY (prop->property)));
          g_string_append (str, ": ");
          bobgui_css_value_print (prop->value, str);
          g_string_append (str, ";\n");
        }

      g_free (sorted);
    }

  if (ruleset->custom_properties)
    {
      BobguiCssCustomPropertyPool *pool = bobgui_css_custom_property_pool_get ();
      GPtrArray *keys;

      keys = g_hash_table_get_keys_as_ptr_array (ruleset->custom_properties);
      g_ptr_array_sort_with_data (keys, compare_custom_properties, pool);

      for (i = 0; i < keys->len; i++)
        {
          int id = GPOINTER_TO_INT (g_ptr_array_index (keys, i));
          const char *name = bobgui_css_custom_property_pool_get_name (pool, id);
          BobguiCssVariableValue *value = g_hash_table_lookup (ruleset->custom_properties,
                                                            GINT_TO_POINTER (id));

          g_string_append (str, "  ");
          g_string_append (str, name);
          g_string_append (str, ": ");
          bobgui_css_variable_value_print (value, str);
          g_string_append (str, ";\n");
        }

      g_ptr_array_unref (keys);
    }

  g_string_append (str, "}\n");
}

static void
bobgui_css_provider_print_colors (GHashTable *colors,
                               GString    *str)
{
  GList *keys, *walk;

  keys = g_hash_table_get_keys (colors);
  /* so the output is identical for identical styles */
  keys = g_list_sort (keys, (GCompareFunc) strcmp);

  for (walk = keys; walk; walk = walk->next)
    {
      const char *name = walk->data;
      BobguiCssValue *color = g_hash_table_lookup (colors, (gpointer) name);

      g_string_append (str, "@define-color ");
      g_string_append (str, name);
      g_string_append (str, " ");
      bobgui_css_value_print (color, str);
      g_string_append (str, ";\n");
    }

  g_list_free (keys);
}

static void
bobgui_css_provider_print_keyframes (GHashTable *keyframes,
                                  GString    *str)
{
  GList *keys, *walk;

  keys = g_hash_table_get_keys (keyframes);
  /* so the output is identical for identical styles */
  keys = g_list_sort (keys, (GCompareFunc) strcmp);

  for (walk = keys; walk; walk = walk->next)
    {
      const char *name = walk->data;
      BobguiCssKeyframes *keyframe = g_hash_table_lookup (keyframes, (gpointer) name);

      if (str->len > 0)
        g_string_append (str, "\n");
      g_string_append (str, "@keyframes ");
      g_string_append (str, name);
      g_string_append (str, " {\n");
      _bobgui_css_keyframes_print (keyframe, str);
      g_string_append (str, "}\n");
    }

  g_list_free (keys);
}

/**
 * bobgui_css_provider_to_string:
 * @provider: the provider to write to a string
 *
 * Converts the @provider into a string representation in CSS
 * format.
 *
 * Using [method@Bobgui.CssProvider.load_from_string] with the return
 * value from this function on a new provider created with
 * [ctor@Bobgui.CssProvider.new] will basically create a duplicate
 * of this @provider.
 *
 * Returns: a new string representing the @provider.
 */
char *
bobgui_css_provider_to_string (BobguiCssProvider *provider)
{
  BobguiCssProviderPrivate *priv = bobgui_css_provider_get_instance_private (provider);
  GString *str;
  guint i;

  g_return_val_if_fail (BOBGUI_IS_CSS_PROVIDER (provider), NULL);

  str = g_string_new ("");

  bobgui_css_provider_print_colors (priv->symbolic_colors, str);
  bobgui_css_provider_print_keyframes (priv->keyframes, str);

  for (i = 0; i < priv->rulesets->len; i++)
    {
      if (str->len != 0)
        g_string_append (str, "\n");
      bobgui_css_ruleset_print (&g_array_index (priv->rulesets, BobguiCssRuleset, i), str);
    }

  return g_string_free (str, FALSE);
}
