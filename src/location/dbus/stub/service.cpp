/*
 * Copyright © 2012-2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#include <location/dbus/stub/service.h>
#include <location/dbus/stub/session.h>

#include <location/dbus/codec.h>
#include <location/dbus/service.h>
#include <location/dbus/util.h>
#include <location/glib/holder.h>
#include <location/glib/util.h>

#include <location/providers/remote/provider.h>

#include <location/logging.h>

#include <boost/core/ignore_unused.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

namespace
{

using Holder = location::glib::Holder<std::weak_ptr<location::dbus::stub::Service>>;

}  // namespace

void location::dbus::stub::Service::create(std::function<void(const location::Result<location::dbus::stub::Service::Ptr>&)> callback)
{
    g_bus_get(G_BUS_TYPE_SESSION, nullptr, on_bus_acquired, new BusAcquisitionContext{std::move(callback)});
}

location::dbus::stub::Service::Service(const glib::SharedObject<GDBusConnection>& connection,
                                       const glib::SharedObject<ComUbuntuLocationService>& proxy)
    : connection{connection},
      proxy{proxy},
      state_{boost::lexical_cast<Service::State>(com_ubuntu_location_service_get_state(proxy.get()))},
      does_satellite_based_positioning_{com_ubuntu_location_service_get_does_satellite_based_positioning(proxy.get())},
      does_report_cell_and_wifi_ids_{com_ubuntu_location_service_get_does_report_cell_and_wifi_ids(proxy.get())},
      is_online_{com_ubuntu_location_service_get_is_online(proxy.get())}
{
}

std::shared_ptr<location::dbus::stub::Service> location::dbus::stub::Service::finalize_construction()
{
    auto sp = shared_from_this();
    std::weak_ptr<Service> wp{sp};

    does_satellite_based_positioning_.changed().connect([this, wp](bool value)
    {
        if (auto sp = wp.lock())
            com_ubuntu_location_service_set_does_satellite_based_positioning(Service::proxy.get(), value);
    });

    does_report_cell_and_wifi_ids_.changed().connect([this, wp](bool value)
    {
        if (auto sp = wp.lock())
            com_ubuntu_location_service_set_does_report_cell_and_wifi_ids(Service::proxy.get(), value);
    });

    is_online_.changed().connect([this, wp](bool value)
    {
        if (auto sp = wp.lock())
            com_ubuntu_location_service_set_is_online(Service::proxy.get(), value);
    });

    signal_handler_ids.insert(
                g_signal_connect_data(Service::proxy.get(), "notify::does-satellite-based-positioning",
                                      G_CALLBACK(on_does_satellite_based_positioning_changed),
                                      new Holder{wp}, Holder::closure_notify, GConnectFlags(0)));

    signal_handler_ids.insert(
                g_signal_connect_data(Service::proxy.get(), "notify::does-report-cell-and-wifi-ids",
                                      G_CALLBACK(on_does_report_cell_and_wifi_ids_changed),
                                      new Holder{wp}, Holder::closure_notify, GConnectFlags(0)));

    signal_handler_ids.insert(
                g_signal_connect_data(Service::proxy.get(), "notify::is-online",
                                      G_CALLBACK(on_is_online_changed),
                                      new Holder{wp}, Holder::closure_notify, GConnectFlags(0)));

    return sp;
}

location::dbus::stub::Service::~Service()
{
    for (auto id : signal_handler_ids)
        g_signal_handler_disconnect(proxy.get(), id);
}

void location::dbus::stub::Service::create_session_for_criteria(const Criteria& criteria, const std::function<void(const Session::Ptr&)>& cb)
{
    auto sp = shared_from_this();
    std::weak_ptr<Service> wp{sp};

    com_ubuntu_location_service_call_create_session_for_criteria(
                proxy.get(), dbus::encode(criteria), nullptr, on_session_ready, new Service::SessionCreationContext{std::move(cb), wp});
}

void location::dbus::stub::Service::add_provider(const Provider::Ptr& provider)
{
    auto sp = shared_from_this();
    std::weak_ptr<Service> wp{sp};

    auto path = "/providers/" + std::to_string(provider_counter++);
    auto skeleton = location::providers::remote::Provider::Skeleton::create(connection, path, provider);

    com_ubuntu_location_service_call_add_provider(
                proxy.get(), path.c_str(), nullptr, Service::on_provider_added, new Service::ProviderAdditionContext{skeleton, wp});
}

const core::Property<location::Service::State>& location::dbus::stub::Service::state() const
{
    return state_;
}

core::Property<bool>& location::dbus::stub::Service::does_satellite_based_positioning()
{
    return does_satellite_based_positioning_;
}

core::Property<bool>& location::dbus::stub::Service::does_report_cell_and_wifi_ids()
{
    return does_report_cell_and_wifi_ids_;
}

core::Property<bool>& location::dbus::stub::Service::is_online()
{
    return is_online_;
}

core::Property<std::map<location::SpaceVehicle::Key, location::SpaceVehicle>>& location::dbus::stub::Service::visible_space_vehicles()
{
    return visible_space_vehicles_;
}

void location::dbus::stub::Service::on_proxy_ready(GObject* source, GAsyncResult* res, gpointer user_data)
{
    LOCATION_DBUS_TRACE_STATIC_TRAMPOLIN;

    boost::ignore_unused(source);

    if (auto context = static_cast<Service::ProxyCreationContext*>(user_data))
    {
        GError* error{nullptr};
        auto stub = com_ubuntu_location_service_proxy_new_finish(res, &error);

        if (error)
        {
            context->cb(make_error_result<Service::Ptr>(glib::wrap_error_as_exception(error)));
        }
        else
        {
            location::dbus::stub::Service::Ptr sp{
                new location::dbus::stub::Service{
                    context->connection, location::glib::make_shared_object(stub)}};
            context->cb(make_result(sp->finalize_construction()));
        }

        delete context;
    }
}

void location::dbus::stub::Service::on_bus_acquired(GObject* source, GAsyncResult* res, gpointer user_data)
{
    LOCATION_DBUS_TRACE_STATIC_TRAMPOLIN;

    boost::ignore_unused(source);

    if (auto context = static_cast<Service::BusAcquisitionContext*>(user_data))
    {
        GError* error{nullptr};
        auto bus = g_bus_get_finish(res, &error);

        if (error)
        {
            context->cb(Result<Service::Ptr>(glib::wrap_error_as_exception(error)));
        }
        else
        {
            com_ubuntu_location_service_proxy_new(
                        bus, G_DBUS_PROXY_FLAGS_NONE, location::dbus::Service::name(), location::dbus::Service::path(),
                        nullptr, on_proxy_ready, new Service::ProxyCreationContext{location::glib::make_shared_object(bus), context->cb});
        }

        delete context;
    }
}

void location::dbus::stub::Service::on_provider_added(GObject *source, GAsyncResult *res, gpointer user_data)
{
    LOCATION_DBUS_TRACE_STATIC_TRAMPOLIN;

    boost::ignore_unused(source);

    if (auto context = static_cast<Service::ProviderAdditionContext*>(user_data))
    {
        if (auto sp = context->wp.lock())
        {
            GError* error{nullptr};
            if (com_ubuntu_location_service_call_add_provider_finish(
                        sp->proxy.get(), res, &error))
            {
                sp->providers.insert(context->provider);
            }
            else
            {
                // TODO(tvoss): Clarify on error reporting infrastructure.
                glib::wrap_error_as_exception(error);
            }
        }
        delete context;
    }
}

void location::dbus::stub::Service::on_session_ready(GObject *source, GAsyncResult *res, gpointer user_data)
{
    LOCATION_DBUS_TRACE_STATIC_TRAMPOLIN;
    boost::ignore_unused(source);

    if (auto context = static_cast<Service::SessionCreationContext*>(user_data))
    {
        if (auto sp = context->wp.lock())
        {
            GError* error{nullptr}; char* path{nullptr};
            com_ubuntu_location_service_call_create_session_for_criteria_finish(
                        sp->proxy.get(), &path, res, &error);

            if (!error)
            {
                auto cb = context->cb;
                stub::Session::create(sp->connection, path, [cb](const Result<stub::Session::Ptr>& result)
                {
                    if (result)
                        cb(result.value());
                });
            }
            else
            {
                // TODO(tvoss): Clarifz on error reporting infrastructure.
                glib::wrap_error_as_exception(error);
            }
        }
        delete context;
    }
}

void location::dbus::stub::Service::on_does_satellite_based_positioning_changed(GObject* object, GParamSpec* spec, gpointer user_data)
{
    LOCATION_DBUS_TRACE_STATIC_TRAMPOLIN;
    boost::ignore_unused(object, spec);

    if (auto holder = static_cast<Holder*>(user_data))
    {
        if (auto sp = holder->value.lock())
        {
            sp->does_satellite_based_positioning() =
                    com_ubuntu_location_service_get_does_satellite_based_positioning(
                        sp->proxy.get());
        }
    }
}

void location::dbus::stub::Service::on_does_report_cell_and_wifi_ids_changed(GObject* object, GParamSpec* spec, gpointer user_data)
{
    LOCATION_DBUS_TRACE_STATIC_TRAMPOLIN;
    boost::ignore_unused(object, spec);

    if (auto holder = static_cast<Holder*>(user_data))
    {
        if (auto sp = holder->value.lock())
        {
            sp->does_report_cell_and_wifi_ids() =
                    com_ubuntu_location_service_get_does_report_cell_and_wifi_ids(
                        sp->proxy.get());
        }
    }
}

void location::dbus::stub::Service::on_is_online_changed(GObject* object, GParamSpec* spec, gpointer user_data)
{
    LOCATION_DBUS_TRACE_STATIC_TRAMPOLIN;
    boost::ignore_unused(object, spec);

    if (auto holder = static_cast<Holder*>(user_data))
    {
        if (auto sp = holder->value.lock())
        {
            sp->is_online() =
                    com_ubuntu_location_service_get_is_online(
                        sp->proxy.get());
        }
    }
}
