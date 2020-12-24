/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Scott Moreau
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include <iostream>
#include <gdk/wayland/gdkwayland.h>

#include "menu.hpp"

void WfMenu::on_menu_item_maximize()
{
    wf_desktop_base_maximize(wf_desktop_manager);
    wl_display_flush(display);
    exit(0);
}

void WfMenu::on_menu_item_minimize()
{
    wf_desktop_base_minimize(wf_desktop_manager);
    wl_display_flush(display);
    exit(0);
}

void WfMenu::on_menu_item_close()
{
    wf_desktop_base_close(wf_desktop_manager);
    wl_display_flush(display);
    exit(0);
}

bool WfMenu::on_key_press(guint keyval, guint keycode, Gdk::ModifierType state)
{
    popover_menu.popup();

    return true;
}

void WfMenu::on_popover_hide()
{
    //exit(0);
}

void WfMenu::on_startup()
{
    app->add_window(*this);

    ref_event = Gtk::EventControllerKey::create();
    ref_event->signal_key_pressed().connect(
        sigc::mem_fun(*this, &WfMenu::on_key_press), false);
    this->add_controller(ref_event);

    auto ref_action_group = Gio::SimpleActionGroup::create();

    ref_action_group->add_action("maximize",
      sigc::mem_fun(*this, &WfMenu::on_menu_item_maximize));
    
    ref_action_group->add_action("minimize",
      sigc::mem_fun(*this, &WfMenu::on_menu_item_minimize));
    
    ref_action_group->add_action("close",
      sigc::mem_fun(*this, &WfMenu::on_menu_item_close));
    
    insert_action_group("example", ref_action_group);

    ref_builder = Gtk::Builder::create();

    Glib::ustring ui_info =
      "<interface>"
    "  <menu id='menubar'>"
    "    <section>"
    "      <item>";
    ui_info += maximized ?
    "        <attribute name='label' translatable='yes'>_Unmaximize</attribute>" :
    "        <attribute name='label' translatable='yes'>_Maximize</attribute>";
    ui_info +=
    "        <attribute name='action'>example.maximize</attribute>"
    "      </item>"
    "      <item>";
    ui_info += minimized ?
    "        <attribute name='label' translatable='yes'>_Unminimize</attribute>" :
    "        <attribute name='label' translatable='yes'>_Minimize</attribute>";
    ui_info +=
    "        <attribute name='action'>example.minimize</attribute>"
    "      </item>"
    "      <item>"
    "        <attribute name='label' translatable='yes'>_Close</attribute>"
    "        <attribute name='action'>example.close</attribute>"
    "      </item>"
    "    </section>"
    "  </menu>"
    "</interface>";

    try
    {
      ref_builder->add_from_string(ui_info);
    }
    catch(const Glib::Error& ex)
    {
      std::cerr << "building menus failed: " <<  ex.what();
    }

    auto object =
      ref_builder->get_object("menubar");
    auto gmenu =
      std::dynamic_pointer_cast<Gio::Menu>(object);
    if(!gmenu)
      g_warning("GMenu not found");

    const std::string style_info = "* {border-radius: 0;}";
    auto style_context = this->get_style_context();
    css_provider = Gtk::CssProvider::create();
    css_provider->load_from_data(style_info);
    style_context->add_provider_for_display(style_context->get_display(), css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    popover_menu = Gtk::PopoverMenu(gmenu, Gtk::PopoverMenu::Flags::NESTED);
    popover_menu.set_has_arrow(false);
    popover_menu.set_halign(Gtk::Align::START);
    popover_menu.set_valign(Gtk::Align::START);
    box = Gtk::Box(Gtk::Orientation::HORIZONTAL, 0);
    box.set_parent(*this);
    popover_menu.set_parent(box);
    const Gdk::Rectangle rect(-1, -1, 1, 1);
    popover_menu.set_pointing_to(rect);
    popover_menu.signal_hide().connect(
      sigc::mem_fun(*this, &WfMenu::on_popover_hide));
    show();
}

static void registry_add(void *data, struct wl_registry *registry,
    uint32_t id, const char *interface,
    uint32_t version)
{
    WfMenu *wfm = (WfMenu *) data;

    if (strcmp(interface, wf_desktop_base_interface.name) == 0)
    {
        wfm->wf_desktop_manager = (wf_desktop_base *)
            wl_registry_bind(registry, id,
            &wf_desktop_base_interface, 1);
    }
}

static void registry_remove(void *data, struct wl_registry *registry,
    uint32_t id)
{
    exit(0);
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_add,
    .global_remove = registry_remove,
};

static void view_actions(void *data,
    struct wf_desktop_base *wf_desktop_base,
    const char *actions)
{
    WfMenu *wfm = (WfMenu *) data;

    std::string str(actions);
    auto pos = str.find("Maximized:") + strlen("Maximized:");
    if (str[pos] == '1')
    {
        wfm->maximized = true;
    } else
    {
        wfm->maximized = false;
    }
    pos = str.find("Minimized:") + strlen("Minimized:");
    if (str[pos] == '1')
    {
        wfm->minimized = true;
    } else
    {
        wfm->minimized = false;
    }
}

static struct wf_desktop_base_listener desktop_base_listener {
	.view_actions = view_actions,
};

WfMenu::WfMenu(const Glib::RefPtr<Gtk::Application>& app)
{
    display = gdk_wayland_display_get_wl_display(gdk_display_get_default());
    if (!display)
    {
        return;
    }

    wl_registry *registry = wl_display_get_registry(display);
    if (!registry)
    {
        return;
    }

    wl_registry_add_listener(registry, &registry_listener, this);

    wl_display_roundtrip(display);
    wl_registry_destroy(registry);
    if (!wf_desktop_manager)
    {
        std::cout << "Wayfire desktop protocol not advertised by compositor." << std::endl;
        return;
    }

    maximized = minimized = false;
    wf_desktop_base_add_listener(wf_desktop_manager,
        &desktop_base_listener, this);
    wl_display_roundtrip(display);

    this->app = app;
    app->signal_startup().connect(
      sigc::mem_fun(*this, &WfMenu::on_startup));
    set_default_size(1, 1);
    set_decorated(false);
    Glib::set_application_name("wf-menu");
}

WfMenu::~WfMenu()
{
}

int main(int argc, char *argv[])
{
    auto app = Gtk::Application::create("wf-menu");

    WfMenu window(app);

    return app->run(argc, argv);
}
