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


#include <wayfire/plugin.hpp>
#include <wayfire/singleton-plugin.hpp>
#include <wayfire/signal-definitions.hpp>
#include <wayfire/util/log.hpp>

#include "wayfire-desktop/wayfire-desktop.hpp"

class extra_protos
{
    wf::option_wrapper_t<bool> enable_wayfire_desktop{
        "extra-protos/enable_wayfire_desktop"};

  public:
    std::unique_ptr<wayfire_desktop> wayfire_desktop_ptr = nullptr;

    extra_protos()
    {
        enable_wayfire_desktop.set_callback([=] ()
        {
            enable_wayfire_desktop_changed();
        });
        enable_wayfire_desktop_changed();
    }

    void enable_wayfire_desktop_changed()
    {
        if (enable_wayfire_desktop)
        {
            wayfire_desktop_ptr = std::make_unique<wayfire_desktop>();
        } else if (wayfire_desktop_ptr)
        {
            wayfire_desktop_ptr.reset();
            wayfire_desktop_ptr = nullptr;
        }
    }

    ~extra_protos()
    {
        wayfire_desktop_ptr.reset();
    }
};

class extra_protos_singleton : public wf::singleton_plugin_t<extra_protos> {};

DECLARE_WAYFIRE_PLUGIN(extra_protos_singleton);
