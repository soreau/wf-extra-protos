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

class wayfire_desktop
{
    /* The command should be set to a client that shows a menu window. */
    wf::option_wrapper_t<std::string> command{"menu/command"};
    wf::option_wrapper_t<std::string> app_id{"menu/app_id"};
    wf::signal_connection_t show_window_menu;
    wf::signal_connection_t view_unmapped;
    wf::signal_connection_t view_mapped;
    wf::signal_connection_t on_button;
    wayfire_view menu_view   = nullptr;
    std::string get_actions_for_view();
    wf::point_t position_offset;
    wl_global *manager;

  public:
    wayfire_view origin_view = nullptr;
    std::vector<wl_resource*> client_resources;
    void send_view_data(wl_resource *resource);
    wayfire_desktop();
    ~wayfire_desktop();
};
