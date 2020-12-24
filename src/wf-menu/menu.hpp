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


#pragma once

#include <gtkmm.h>

#include "wayfire-desktop-client-protocol.h"

class WfMenu : public Gtk::Window
{
  public:
    WfMenu(const Glib::RefPtr<Gtk::Application>& app);
    virtual ~WfMenu();

    wl_display *display;
    wf_desktop_base *wf_desktop_manager;
    Glib::RefPtr<Gtk::Application> app;
    Gtk::PopoverMenu popover_menu;
    bool maximized, minimized;
    void on_startup();
  protected:
    Gtk::Box box;
    Glib::RefPtr<Gtk::Builder> ref_builder;
    Glib::RefPtr<Gtk::EventControllerKey> ref_event;
    Glib::RefPtr<Gtk::CssProvider> css_provider;
    bool on_key_press(guint keyval, guint keycode, Gdk::ModifierType state);
    void on_menu_item_maximize();
    void on_menu_item_minimize();
    void on_menu_item_close();
    void on_right_click(int n_press, double x, double y);
    void on_popover_hide();
};
