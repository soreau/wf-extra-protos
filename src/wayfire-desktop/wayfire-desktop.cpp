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


#include <sys/time.h>
#include <wayfire/core.hpp>
#include <wayfire/view.hpp>
#include <wayfire/plugin.hpp>
#include <wayfire/output.hpp>
#include <wayfire/output-layout.hpp>
#include <wayfire/workspace-manager.hpp>
#include <wayfire/signal-definitions.hpp>
#include <linux/input-event-codes.h>
#include <wayfire/util/log.hpp>

#include "wayfire-desktop.hpp"
#include "wayfire-desktop-server-protocol.h"

extern "C"
{
#include <wlr/types/wlr_seat.h>
}

static void bind_manager(wl_client *client, void *data,
    uint32_t version, uint32_t id);

void wayfire_desktop::send_view_data(wl_resource *resource)
{
    auto str = get_actions_for_view();
    auto actions = str.c_str();
printf("sending view data: %s\n", actions);fflush(stdout);
    if (resource)
    {
        wf_desktop_base_send_view_actions(resource, actions);
        return;
    }

    for (auto r : client_resources)
    {
        wf_desktop_base_send_view_actions(r, actions);
    }
}

std::string wayfire_desktop::get_actions_for_view()
{
    auto view = origin_view;
    std::string actions;

    if (!view)
    {
        return "";
    }
    actions += "Title:" + view->get_title();
    actions += "\nMaximized:";
    if (view->tiled_edges == wf::TILED_EDGES_ALL)
    {
        actions += "1";
    } else
    {
        actions += "0";
    }
    actions += "\nMinimized:";
    if (view->minimized)
    {
        actions += "1";
    } else
    {
        actions += "0";
    }

    return actions;
}

wayfire_desktop::wayfire_desktop()
{
    manager = wl_global_create(wf::get_core().display,
        &wf_desktop_base_interface, 1, this, bind_manager);

    if (!manager)
    {
        LOGE("Failed to create wayfire_desktop interface");
        return;
    }

    view_mapped.set_callback([this] (wf::signal_data_t *data)
    {
        auto view = get_signaled_view(data);
        if (!view || !origin_view || (view->get_app_id() != std::string(app_id)))
        {
            return;
        }

        view->set_decoration(nullptr);

        auto vg = origin_view->get_output_geometry();
        auto position = wf::point_t{vg.x, vg.y} + position_offset;

        vg = view->get_wm_geometry();
        auto output = view->get_output();
        if (!output)
        {
            return;
        }
        auto og     = output->get_relative_geometry();
        int padding = 20;
        og.x     += padding;
        og.y     += padding;
        og.width -= padding * 2 + vg.width;
        og.height -= padding * 2 + vg.height;
        if ((og.width <= 0) || (og.height <= 0))
        {
            return;
        }

        wlr_box box{og.x, og.y, og.width, og.height};
        wf::pointf_t p{(float)position.x, (float)position.y};
        p = origin_view->transform_point(p);
        wlr_box_closest_point(&box, p.x, p.y, &p.x, &p.y);
        position.x = (int)p.x;
        position.y = (int)p.y;

        ((wf::view_mapped_signal*)data)->is_positioned = true;
        view->move(position.x, position.y);

        /* Place above other views */
        output->workspace->add_view(view, wf::LAYER_UNMANAGED);

        menu_view = view;

        output->disconnect_signal(&view_mapped);
        output->connect_signal("view-unmapped", &view_unmapped);
        wf::get_core().connect_signal("pointer_button", &on_button);

        /* Send wf-menu a key event to popup the menu */
        if (std::string(app_id) == "wf-menu")
        {
            struct timeval now;
            gettimeofday(&now, 0);
            auto msec = now.tv_sec * 1000 + now.tv_usec / 1000;
            wlr_seat_keyboard_notify_key(wf::get_core().get_current_seat(),
                msec, KEY_S, WL_KEYBOARD_KEY_STATE_PRESSED);
            gettimeofday(&now, 0);
            msec = now.tv_sec * 1000 + now.tv_usec / 1000;
            wlr_seat_keyboard_notify_key(wf::get_core().get_current_seat(),
                msec, KEY_S, WL_KEYBOARD_KEY_STATE_RELEASED);
        }
    });

    on_button.set_callback([=] (wf::signal_data_t *data)
    {
        auto ev = static_cast<
            wf::input_event_signal<wlr_event_pointer_button>*>(data);

        if (ev->event->state != WLR_BUTTON_PRESSED)
        {
            return;
        }

        auto view = wf::get_core().get_cursor_focus_view();
        if (!menu_view || !view)
        {
            return;
        }

        /* Check if the client of the views match, in case it's a
         * subsurface/menu, meaning the views won't match but
         * the underlying client object will */
        if (menu_view->get_client() != view->get_client())
        {
            menu_view->close();
        }
    });

    view_unmapped.set_callback([this] (wf::signal_data_t *data)
    {
        auto view = get_signaled_view(data);
        if (view && (menu_view == view))
        {
            menu_view = origin_view = nullptr;
            wf::get_core().disconnect_signal(&on_button);
            auto output = view->get_output();
            if (!output)
            {
                return;
            }
            output->disconnect_signal(&view_unmapped);
        }
    });

    show_window_menu.set_callback([this] (wf::signal_data_t *data)
    {
        if (origin_view || menu_view)
        {
            return;
        }

        /* Showing menu for this view */
        origin_view = get_signaled_view(data);
        auto output = origin_view->get_output();
        if (!output)
        {
            return;
        }
        output->connect_signal("view-mapped", &view_mapped);
        position_offset =
            ((wf::view_show_window_menu_signal*)data)->relative_position;
        wf::get_core().run(std::string(command));
    });

    wf::get_core().connect_signal("view-show-window-menu", &show_window_menu);
}

wayfire_desktop::~wayfire_desktop()
{
    if (menu_view)
    {
        menu_view->close();
    }
    wl_global_remove(manager);
}

static void handle_maximize(struct wl_client *client, struct wl_resource *resource)
{
    LOGI(__func__);
    wayfire_desktop *wd = (wayfire_desktop*)wl_resource_get_user_data(resource);
    auto view = wd->origin_view;

    if (view)
    {
        view->tile_request(view->tiled_edges ==
            wf::TILED_EDGES_ALL ? 0 : wf::TILED_EDGES_ALL);
    }
}

static void handle_minimize(struct wl_client *client, struct wl_resource *resource)
{
    LOGI(__func__);
    wayfire_desktop *wd = (wayfire_desktop*)wl_resource_get_user_data(resource);
    auto view = wd->origin_view;

    if (view)
    {
        view->minimize_request(!view->minimized);
    }
}

static void handle_close(struct wl_client *client, struct wl_resource *resource)
{
    LOGI(__func__);
    wayfire_desktop *wd = (wayfire_desktop*)wl_resource_get_user_data(resource);
    auto view = wd->origin_view;

    if (view)
    {
        view->close();
    }
}

static const struct wf_desktop_base_interface wayfire_desktop_impl =
{
    .maximize = handle_maximize,
    .minimize = handle_minimize,
    .close    = handle_close,
};

static void destroy_client(wl_resource *resource)
{
    wayfire_desktop *wd = (wayfire_desktop*)wl_resource_get_user_data(resource);

    auto vector = wd->client_resources;
    std::remove(vector.begin(), vector.end(), resource);
}

static void bind_manager(wl_client *client, void *data,
    uint32_t version, uint32_t id)
{
    wayfire_desktop *wd = (wayfire_desktop*)data;

    if (!wd->origin_view)
    {
        wd->origin_view = wf::get_core().get_cursor_focus_view();
    }
    if (!wd->origin_view)
    {
        return;
    }

    auto resource =
        wl_resource_create(client, &wf_desktop_base_interface, 1, id);
    wl_resource_set_implementation(resource,
        &wayfire_desktop_impl, data, destroy_client);
    wd->client_resources.push_back(resource);
    wd->send_view_data(resource);
    
}
