/*

gcc -shared -Wall -fPIC -o libandromeda.so andromeda.c `pkg-config --cflags --libs libxfce4panel-2.0`
sudo cp libandromeda.so /usr/lib/i386-linux-gnu/xfce4/panel-plugins/libandromeda.so
sudo cp andromeda.desktop /usr/share/xfce4/panel-plugins/andromeda.desktop


зависимости:
libxfce4panel-2.0-dev

отладка:
xfce4-panel -q
PANEL_DEBUG=1 xfce4-panel


*/


#include <libxfce4panel/xfce-panel-plugin.h>

#define __WINDOW_HEIGHT 150
#define __WINDOW_WIDTH 400


//============================================================================================
typedef struct {
  GtkWidget*      mainButton;
  GtkWidget*      window;
  GtkWidget*      container;
  GtkWidget*      text;
  guint           busId;
  GSource*        timer;
} pluginStruct;
//============================================================================================
//============================================================================================
static void gtk_scrolled_window_scroll_down(GtkWidget* window)
{
  GtkAdjustment * adj;
  gdouble value;

  adj = gtk_scrolled_window_get_vadjustment((GtkScrolledWindow*)window);
  value = gtk_adjustment_get_upper (adj);
  gtk_adjustment_set_value(adj, value);
  gtk_scrolled_window_set_vadjustment((GtkScrolledWindow*)window, adj);
}
//============================================================================================
static gboolean timerFunction(pluginStruct* plugin)
{
  static int counter = 0;
  if (counter > 5)
  {
    counter = 0;
    plugin->timer = NULL;
    return G_SOURCE_REMOVE;
  }
  else
    counter++;

  GtkStyleContext* context1 = gtk_widget_get_style_context(plugin->mainButton);
  GtkStyleContext* context2 = gtk_widget_get_style_context(plugin->window);
  if (counter & 1)
  {
    gtk_style_context_add_class(context1, "redButtonClass");
    gtk_style_context_add_class(context2, "redButtonClass");
  }
  else
  {
    gtk_style_context_remove_class(context1, "redButtonClass");
    gtk_style_context_remove_class(context2, "redButtonClass");
  }
  return G_SOURCE_CONTINUE;
}
//============================================================================================
static void handle_method_call(GDBusConnection* connection, const gchar* sender, const gchar* object_path, const gchar* interface_name, const gchar* method_name, GVariant* parameters, GDBusMethodInvocation* invocation, gpointer data)
{
  pluginStruct* plugin = data;
  //dbus-send --session --type=method_call --print-reply --dest=andromeda.server /andromeda/methods andromeda.listener.print string:'hello world'
  if (g_strcmp0(method_name, "print") == 0)
  {
    const gchar* param;
    g_variant_get(parameters, "(&s)", &param);
    GtkTextIter    iter;
    GtkTextBuffer* buffer;
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(plugin->text));
    gtk_text_buffer_get_end_iter (buffer, &iter);
    gtk_text_buffer_insert (buffer, &iter, param, -1);
    gtk_text_buffer_insert (buffer, &iter, "\n", -1);
    gtk_scrolled_window_scroll_down(plugin->container);
//    GVariant* answer;
//    answer = g_variant_new("(s)", param);
//    g_dbus_method_invocation_return_value(invocation, answer);
    g_dbus_method_invocation_return_value(invocation, NULL);
  }
  //dbus-send --session --type=method_call --print-reply --dest=andromeda.server /andromeda/methods andromeda.listener.flash
  else if (g_strcmp0(method_name, "flash") == 0)
  {
    if (plugin->timer == NULL)
    {
      plugin->timer = g_timeout_source_new(500);
      g_source_set_priority (plugin->timer, G_PRIORITY_DEFAULT);
      g_source_set_callback(plugin->timer, (GSourceFunc)timerFunction, plugin, NULL);
      g_source_attach(plugin->timer, NULL);
      g_source_unref(plugin->timer);
    }
    g_dbus_method_invocation_return_value(invocation, NULL);
  }
}
//============================================================================================
static void on_bus_acquired(GDBusConnection* connection, const gchar* name, gpointer data)
{
  pluginStruct* plugin = data;
  GDBusNodeInfo* introspection_data;
  introspection_data = g_dbus_node_info_new_for_xml(
      "<node>"
      "  <interface name='andromeda.listener'>"
      "    <method name='print'>"
      "      <arg type='s' name='in_text' direction='in'/>"
//      "      <arg type='s' name='response' direction='out'/>"
      "    </method>"
      "    <method name='flash'/>"
      "  </interface>"
      "</node>",
      NULL);
  const GDBusInterfaceVTable interface_vtable = {handle_method_call, NULL, NULL};
  g_dbus_connection_register_object(connection, "/andromeda/methods", introspection_data->interfaces[0], &interface_vtable, plugin, NULL, NULL);
  g_dbus_node_info_unref(introspection_data);
}
//============================================================================================
static void mainButtonClicked(GtkWidget* button, pluginStruct* widgets)
{
  if (gtk_widget_get_visible(widgets->window))
  {
    gtk_widget_hide(widgets->window);
    gtk_button_set_label((GtkButton*)widgets->mainButton, "⬆");
  }
  else
  {
    GdkWindow* id;
    gint x, y;
    int w;

    id = gtk_widget_get_window(button);
    gdk_window_get_origin(id, &x, &y);
    w = gdk_window_get_width (id);
    x = x - (__WINDOW_WIDTH - w);
    y = y - __WINDOW_HEIGHT;

    gtk_window_move(GTK_WINDOW(widgets->window), x, y);
    gtk_widget_show_all(widgets->window);
    gtk_button_set_label((GtkButton*)widgets->mainButton, "⬇");
  }
}
//============================================================================================
static void pluginFree(XfcePanelPlugin* rootWidget, pluginStruct* plugin)
{
  g_bus_unown_name(plugin->busId);

  if (plugin->timer != NULL)
  {
    g_source_destroy(plugin->timer);
    plugin->timer = NULL;
  }

  gtk_widget_destroy(plugin->text);
  gtk_widget_destroy(plugin->container);
  gtk_widget_destroy(plugin->window);
  gtk_widget_destroy(plugin->mainButton);
  g_slice_free(pluginStruct, plugin);
  g_message("===============andromeda exit===============\n");
}
//============================================================================================
static void pluginConstruct(XfcePanelPlugin* rootWidget)
{
  g_message("===============andromeda enter===============\n");
  GtkCssProvider* provider;
  GdkScreen*      screen;

  //добавление класса CSS
  provider = gtk_css_provider_new();
  screen = gdk_display_get_default_screen(gdk_display_get_default());
  gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  gtk_css_provider_load_from_data(provider, ".redButtonClass{\ntransition: 200ms linear;\nbackground: #ff0000;\n}", -1, NULL);
  g_object_unref(provider);

  pluginStruct* plugin;
  plugin = g_slice_new(pluginStruct);

  plugin->timer = NULL;

  plugin->mainButton = xfce_create_panel_button();
  gtk_container_add(GTK_CONTAINER(rootWidget), plugin->mainButton);
  gtk_button_set_label(GTK_BUTTON(plugin->mainButton), "⬆");
  //add right mouse button menu
  xfce_panel_plugin_add_action_widget(rootWidget, plugin->mainButton);
  gtk_widget_show(plugin->mainButton);

  plugin->window = gtk_window_new(GTK_WINDOW_POPUP);
  gtk_window_set_default_size(GTK_WINDOW(plugin->window), __WINDOW_WIDTH, __WINDOW_HEIGHT);

  plugin->container = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_set_border_width(GTK_CONTAINER(plugin->container), 10);
  gtk_container_add(GTK_CONTAINER(plugin->window), plugin->container);

  plugin->text=gtk_text_view_new();
  gtk_container_add(GTK_CONTAINER(plugin->container), plugin->text);
  
  g_signal_connect(plugin->mainButton, "clicked", G_CALLBACK(mainButtonClicked), plugin);
  g_signal_connect(rootWidget, "free-data",  G_CALLBACK(pluginFree), plugin);
  
  plugin->busId = g_bus_own_name(G_BUS_TYPE_SESSION, "andromeda.server", G_BUS_NAME_OWNER_FLAGS_NONE, on_bus_acquired, NULL, NULL, plugin, NULL);
}
XFCE_PANEL_PLUGIN_REGISTER(pluginConstruct);