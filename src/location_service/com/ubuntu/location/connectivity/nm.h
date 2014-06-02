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
#ifndef LOCATION_SERVICE_COM_UBUNTU_LOCATION_CONNECTIVITY_NM_H
#define LOCATION_SERVICE_COM_UBUNTU_LOCATION_CONNECTIVITY_NM_H

#include <core/dbus/bus.h>
#include <core/dbus/object.h>
#include <core/dbus/property.h>
#include <core/dbus/service.h>
#include <core/dbus/types/object_path.h>
#include <core/dbus/types/struct.h>
#include <core/dbus/types/stl/map.h>
#include <core/dbus/types/stl/string.h>
#include <core/dbus/types/stl/tuple.h>
#include <core/dbus/types/stl/vector.h>

namespace org
{
namespace freedesktop
{
struct NetworkManager
{
    typedef std::shared_ptr<NetworkManager> Ptr;

    struct AccessPoint
    {
        static const std::string& name()
        {
            static const std::string s{"org.freedesktop.NetworkManager.AccessPoint"};
            return s;
        }

        struct Frequency
        {
            static const std::string& name()
            {
                static const std::string s{"Frequency"};
                return s;
            }

            typedef AccessPoint Interface;
            typedef std::uint32_t ValueType;
            static const bool readable = true;
            static const bool writable = false;
        };

        struct Mode
        {
            enum Value
            {
                unknown = 0,
                adhoc = 1,
                infra = 2,
                ap = 3
            };

            static const std::string& name()
            {
                static const std::string s{"Mode"};
                return s;
            }

            typedef AccessPoint Interface;
            typedef std::uint32_t ValueType;
            static const bool readable = true;
            static const bool writable = false;
        };

        struct HwAddress
        {
            static const std::string& name()
            {
                static const std::string s{"HwAddress"};
                return s;
            }

            typedef AccessPoint Interface;
            typedef std::string ValueType;
            static const bool readable = true;
            static const bool writable = false;
        };

        struct Ssid
        {
            static const std::string& name()
            {
                static const std::string s{"Ssid"};
                return s;
            }

            typedef AccessPoint Interface;
            typedef std::vector<std::int8_t> ValueType;
            static const bool readable = true;
            static const bool writable = false;
        };

        struct Strength
        {
            static const std::string& name()
            {
                static const std::string s{"Strength"};
                return s;
            }

            typedef AccessPoint Interface;
            typedef std::int8_t ValueType;
            static const bool readable = true;
            static const bool writable = false;
        };

        struct PropertiesChanged
        {
            inline static std::string name()
            {
                return "PropertiesChanged";
            }

            typedef AccessPoint Interface;

            typedef std::map<std::string, core::dbus::types::Variant> ArgumentType;
        };

        AccessPoint(const std::shared_ptr<core::dbus::Object>& object)
            : object(object),
              frequency(object->get_property<Frequency>()),
              mode(object->get_property<Mode>()),
              hw_address(object->get_property<HwAddress>()),
              ssid(object->get_property<Ssid>()),
              strength(object->get_property<Strength>()),
              properties_changed(object->get_signal<PropertiesChanged>())
        {
        }

        std::shared_ptr<core::dbus::Object> object;
        std::shared_ptr<core::dbus::Property<Frequency>> frequency;
        std::shared_ptr<core::dbus::Property<Mode>> mode;
        std::shared_ptr<core::dbus::Property<HwAddress>> hw_address;
        std::shared_ptr<core::dbus::Property<Ssid>> ssid;
        std::shared_ptr<core::dbus::Property<Strength>> strength;
        std::shared_ptr<core::dbus::Signal<PropertiesChanged, PropertiesChanged::ArgumentType>> properties_changed;
    };

    struct Device
    {
        static const std::string& name()
        {
            static const std::string s{"org.freedesktop.NetworkManager.Device"};
            return s;
        }

        enum class Type
        {
            unknown = 0,
            ethernet = 1,
            wifi = 2,
            unused_1 = 3,
            unused_2 = 4,
            bluetooth = 5,
            olpc_mesh = 6,
            wimax = 7,
            modem = 8,
            infiniband = 9,
            bond = 10,
            vlan = 11,
            adsl = 12,
            bridge = 13
        };

        struct Wireless
        {
            static const std::string& name()
            {
                static const std::string s{"org.freedesktop.NetworkManager.Device.Wireless"};
                return s;
            }

            struct GetAccessPoints
            {
                static const std::string& name()
                {
                    static const std::string s{"GetAccessPoints"};
                    return s;
                }

                typedef Wireless Interface;

                static std::chrono::milliseconds default_timeout()
                {
                    return std::chrono::seconds{1};
                }
            };

            struct RequestScan
            {
                static const std::string& name()
                {
                    static const std::string s{"RequestScan"};
                    return s;
                }

                typedef Wireless Interface;

                static std::chrono::milliseconds default_timeout()
                {
                    return std::chrono::seconds{1};
                }
            };

            struct Signals
            {
                struct AccessPointAdded
                {
                    inline static std::string name()
                    {
                        return "AccessPointAdded";
                    }

                    typedef Wireless Interface;

                    typedef core::dbus::types::ObjectPath ArgumentType;
                };

                struct AccessPointRemoved
                {
                    inline static std::string name()
                    {
                        return "AccessPointRemoved";
                    }

                    typedef Wireless Interface;

                    typedef core::dbus::types::ObjectPath ArgumentType;
                };
            };
        };

        struct DeviceType
        {
            static const std::string& name()
            {
                static const std::string s{"DeviceType"};
                return s;
            }

            typedef Device Interface;
            typedef std::uint32_t ValueType;
            static const bool readable = true;
            static const bool writable = false;
        };

        Device(const std::shared_ptr<core::dbus::Service>& service,
               const std::shared_ptr<core::dbus::Object>& object)
            : service(service),
              object(object),
              device_type(object->get_property<DeviceType>()),
              signals
              {
                  object->get_signal<Wireless::Signals::AccessPointAdded>(),
                  object->get_signal<Wireless::Signals::AccessPointRemoved>()
              }
        {
        }

        Type type() const
        {
            return static_cast<Type>(device_type->get());
        }

        std::vector<AccessPoint> get_access_points() const
        {
            typedef std::vector<core::dbus::types::ObjectPath> ResultType;
            auto result = object->invoke_method_synchronously<Wireless::GetAccessPoints, ResultType>();

            if (result.is_error())
                throw std::runtime_error(result.error().print());

            std::vector<AccessPoint> aps;

            for (const auto& path : result.value())
                aps.push_back(AccessPoint(service->object_for_path(path)));

            return aps;
        }

        void request_scan() const
        {
            static const std::map<std::string, core::dbus::types::Variant> dictionary;
            auto result = object->invoke_method_asynchronously<Wireless::RequestScan, void>(dictionary);
        }

        std::shared_ptr<core::dbus::Service> service;
        std::shared_ptr<core::dbus::Object> object;
        std::shared_ptr<core::dbus::Property<DeviceType>> device_type;
        struct
        {
            core::dbus::Signal<Wireless::Signals::AccessPointAdded, Wireless::Signals::AccessPointAdded::ArgumentType>::Ptr ap_added;
            core::dbus::Signal<Wireless::Signals::AccessPointRemoved, Wireless::Signals::AccessPointRemoved::ArgumentType>::Ptr ap_removed;
        } signals;
    };

    static const std::string& name()
    {
        static const std::string s{"org.freedesktop.NetworkManager"};
        return s;
    }

    struct GetDevices
    {
        static const std::string& name()
        {
            static const std::string s{"GetDevices"};
            return s;
        }

        typedef NetworkManager Interface;

        static std::chrono::milliseconds default_timeout()
        {
            return std::chrono::seconds{1};
        }
    };

    struct Properties
    {
        struct Connectivity
        {
            enum Values
            {
                unknown = 0,
                none = 1,
                portal = 2,
                limited = 3,
                full = 4
            };

            static const std::string& name()
            {
                static const std::string s{"Connectivity"};
                return s;
            }

            typedef NetworkManager Interface;
            typedef std::uint32_t ValueType;
            static const bool readable = true;
            static const bool writable = false;
        };
    };

    struct Signals
    {
        struct DeviceAdded
        {
            inline static std::string name()
            {
                return "DeviceAdded";
            }

            typedef NetworkManager Interface;

            typedef core::dbus::types::ObjectPath ArgumentType;
        };

        struct DeviceRemoved
        {
            inline static std::string name()
            {
                return "DeviceRemoved";
            }

            typedef NetworkManager Interface;

            typedef core::dbus::types::ObjectPath ArgumentType;
        };

        struct PropertiesChanged
        {
            inline static std::string name()
            {
                return "PropertiesChanged";
            }

            typedef NetworkManager Interface;

            typedef std::map<std::string, core::dbus::types::Variant> ArgumentType;
        };
    };

    NetworkManager(const core::dbus::Bus::Ptr& bus)
        : service(core::dbus::Service::use_service<NetworkManager>(bus)),
          object(service->object_for_path(core::dbus::types::ObjectPath("/org/freedesktop/NetworkManager"))),
          properties
          {
              object->get_property<Properties::Connectivity>()
          },
          signals
          {
              object->get_signal<Signals::DeviceAdded>(),
              object->get_signal<Signals::DeviceRemoved>(),
              object->get_signal<Signals::PropertiesChanged>()
          }
    {
    }

    std::vector<Device> get_devices()
    {
        auto result =
                object->invoke_method_synchronously<
                    NetworkManager::GetDevices,
                    std::vector<core::dbus::types::ObjectPath>>();

        if (result.is_error())
            throw std::runtime_error(result.error().print());

        std::vector<Device> devices;
        for (const auto& path : result.value())
        {
            devices.emplace_back(
                        Device(
                            service,
                            service->object_for_path(path)));
        }

        return devices;
    }

    std::shared_ptr<core::dbus::Service> service;
    std::shared_ptr<core::dbus::Object> object;

    struct
    {
        std::shared_ptr<core::dbus::Property<Properties::Connectivity> > connectivity;
    } properties;
    struct
    {
        core::dbus::Signal<Signals::DeviceAdded, Signals::DeviceAdded::ArgumentType>::Ptr device_added;
        core::dbus::Signal<Signals::DeviceRemoved, Signals::DeviceRemoved::ArgumentType>::Ptr device_removed;
        core::dbus::Signal<Signals::PropertiesChanged, Signals::PropertiesChanged::ArgumentType>::Ptr properties_changed;
    } signals;
};
}
}

#endif // LOCATION_SERVICE_COM_UBUNTU_LOCATION_CONNECTIVITY_NM_H
