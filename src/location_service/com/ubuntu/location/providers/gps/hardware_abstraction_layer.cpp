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

#include "hardware_abstraction_layer.h"

#include <com/ubuntu/location/clock.h>
#include <com/ubuntu/location/logging.h>

#include <com/ubuntu/location/connectivity/manager.h>

#include <ubuntu/hardware/gps.h>

namespace gps = com::ubuntu::location::providers::gps;
namespace location = com::ubuntu::location;

namespace impl
{
struct HardwareAbstractionLayer : public gps::HardwareAbstractionLayer
{
    struct SuplAssistant : public gps::HardwareAbstractionLayer::SuplAssistant
    {
        struct
        {
            core::Property<gps::HardwareAbstractionLayer::SuplAssistant::Status> status;
            core::Property<gps::HardwareAbstractionLayer::SuplAssistant::IpV4Address> server_ip;
        } impl;

        const core::Property<Status>& status() const
        {
            return impl.status;
        }

        core::Property<Status>& status()
        {
            return impl.status;
        }

        const core::Property<IpV4Address>& server_ip() const
        {
            return impl.server_ip;
        }

        core::Property<IpV4Address>& server_ip()
        {
            return impl.server_ip;
        }

        virtual void set_server(const std::string& host_name, std::uint16_t port)
        {
            (void) host_name;
            (void) port;
        }

        void notify_data_connection_open_via_apn(const std::string& name)
        {
            (void) name;
        }

        void notify_data_connection_closed()
        {
        }

        void notify_data_connection_not_available()
        {
        }
    };

    static void on_nmea_update(int64_t timestamp, const char *nmea, int length, void *context)
    {
        VLOG(20) << __PRETTY_FUNCTION__ << ": "
                << "timestamp=" << timestamp << " "
                << "nmea=" << nmea << " "
                << "length=" << length << " "
                << "context=" << context;
    }

    static void on_xtra_download_request(void* context)
    {
        VLOG(1) << __PRETTY_FUNCTION__ << ": "
                << "context=" << context;
    }

    static void on_gps_ni_notify(UHardwareGpsNiNotification* notification, void* context)
    {
        VLOG(1) << __PRETTY_FUNCTION__ << ": "
                << "notification=" << notification << " "
                << "context=" << context;
    }

    static void on_ril_request_set_id(uint32_t flags, void* context)
    {
        VLOG(1) << __PRETTY_FUNCTION__ << ": "
                << "flags=" << flags << " "
                << "context=" << context;
    }

    static void on_ril_request_reference_location(uint32_t flags, void* context)
    {
        VLOG(1) << __PRETTY_FUNCTION__ << ": "
                << "flags=" << flags << " "
                << "context=" << context;

        /*auto thiz = static_cast<impl::HardwareAbstractionLayer*>(context);

        auto connectivity_manager = location::connectivity::platform_default_manager();

        if (connectivity_manager)
        {
            auto visible_cells = connectivity_manager->visible_radio_cells().get();

            if (!visible_cells.empty())
            {
                const auto& cell = visible_cells.front();
                UHardwareGpsAGpsRefLocation ref_loc;
                switch (cell.type)
                {
                case cul::connectivity::RadioCell::Type::gsm:
                    ref_loc.type = AGPS_REF_LOCATION_TYPE_GSM_CELLID;
                    ref_loc.u.cellID.mcc = cell.gsm().mobile_country_code.get();
                    ref_loc.u.cellID.mnc = cell.gsm().mobile_network_code.get();
                    ref_loc.u.cellID.lac = cell.gsm().location_area_code.get();
                    ref_loc.u.cellID.cid = cell.gsm().id.get();
                    u_hardware_gps_agps_inject_reference_location(&ref_loc, sizeof(ref_loc));
                    break;
                case cul::connectivity::RadioCell::Type::umts:
                    ref_loc.type = ref_loc.type = AGPS_REF_LOCATION_TYPE_UMTS_CELLID;
                    ref_loc.type = AGPS_REF_LOCATION_TYPE_GSM_CELLID;
                    ref_loc.u.cellID.mcc = cell.umts().mobile_country_code.get();
                    ref_loc.u.cellID.mnc = cell.umts().mobile_network_code.get();
                    ref_loc.u.cellID.lac = cell.umts().location_area_code.get();
                    ref_loc.u.cellID.cid = cell.umts().id.get();
                    u_hardware_gps_agps_inject_reference_location(
                                thiz->d->gps_handle,
                                &ref_loc,
                                sizeof(ref_loc));
                    break;
                default:
                    LOG(WARNING) << "The Android GPS HAL only supports gsm and umts cell ids.";
                    break;
                }
            }
        }*/
    }

    static void on_location_update(UHardwareGpsLocation* location, void* context)
    {
        VLOG(1) << __PRETTY_FUNCTION__;

        auto thiz = static_cast<impl::HardwareAbstractionLayer*>(context);

        if (location->flags & U_HARDWARE_GPS_LOCATION_HAS_LAT_LONG)
        {
            location::Position pos
            {
                location::wgs84::Latitude{location->latitude * location::units::Degrees},
                location::wgs84::Longitude{location->longitude * location::units::Degrees}
            };

            if (location->flags & U_HARDWARE_GPS_LOCATION_HAS_ACCURACY)
                pos.accuracy.horizontal = location->accuracy * location::units::Meters;

            if(location->flags & U_HARDWARE_GPS_LOCATION_HAS_ALTITUDE)
                pos.altitude = location::wgs84::Altitude{location->altitude * location::units::Meters};

            // The Android HAL does not provide us with accuracy information for
            // altitude measurements. We just leave out that field.

            thiz->position_updates()(pos);

            VLOG(1) << pos;
        }

        if (location->flags & U_HARDWARE_GPS_LOCATION_HAS_SPEED)
        {
            location::Velocity v{location->speed * location::units::MetersPerSecond};
            thiz->velocity_updates()(v);
            VLOG(1) << v;
        }

        if (location->flags & U_HARDWARE_GPS_LOCATION_HAS_BEARING)
        {
            location::Heading h{location->bearing * location::units::Degrees};
            thiz->heading_updates()(h);
            VLOG(1) << h;
        }
    }

    static void on_status_update(uint16_t status, void* context)
    {
        VLOG(1) << __PRETTY_FUNCTION__ << ": status=" << status << ", context=" << context;
        auto thiz = static_cast<impl::HardwareAbstractionLayer*>(context);
        thiz->chipset_status() = static_cast<gps::ChipsetStatus>(status);
    }

    static void on_sv_status_update(UHardwareGpsSvStatus* sv_info, void* context)
    {
        VLOG(20) << __PRETTY_FUNCTION__ << ": "
                << "count: " << sv_info->num_svs << " "
                << "a: " << sv_info->almanac_mask << " "
                << "e: " << sv_info->ephemeris_mask << " "
                << "u: " << sv_info->used_in_fix_mask << " ";

        auto thiz = static_cast<impl::HardwareAbstractionLayer*>(context);

        std::set<location::SpaceVehicle> svs;

        for (int i = 0; i < sv_info->num_svs; i++)
        {
            location::SpaceVehicle sv;

            // PRN is in the range of [1, 32], adjusting it to make sure we
            // can use it for bitfield flag testing operations.
            int prn = sv_info->sv_list[i].prn - 1;
            if (prn < 0)
                continue;

            auto shift = (1 << prn);

            sv.key.type = location::SpaceVehicle::Type::gps;
            sv.key.id = sv_info->sv_list[i].prn;
            sv.snr = sv_info->sv_list[i].snr;
            sv.has_almanac_data = sv_info->almanac_mask & shift;
            sv.has_ephimeris_data = sv_info->ephemeris_mask & shift;
            sv.used_in_fix = sv_info->used_in_fix_mask & shift;
            sv.azimuth = sv_info->sv_list[i].elevation * location::units::Degrees;
            sv.elevation = sv_info->sv_list[i].azimuth * location::units::Degrees;

            svs.insert(sv);
        }

        thiz->space_vehicle_updates()(svs);
    }

    static void on_set_capabilities(uint32_t capabilities, void* context)
    {
        VLOG(1) << __PRETTY_FUNCTION__;
        auto thiz = static_cast<impl::HardwareAbstractionLayer*>(context);
        thiz->capabilities() = capabilities;
    }

    static void on_agps_status_update(UHardwareGpsAGpsStatus* status, void* context)
    {
        VLOG(1) << __PRETTY_FUNCTION__;

        if (status->type == U_HARDWARE_GPS_AGPS_TYPE_C2K)
        {
            VLOG(1) << "\t We only support SUPL at this point in time.";
            return;
        }
        auto thiz = static_cast<impl::HardwareAbstractionLayer*>(context);
        thiz->impl.supl_assistant.status() = static_cast<gps::HardwareAbstractionLayer::SuplAssistant::Status>(status->status);
        thiz->impl.supl_assistant.server_ip() = gps::HardwareAbstractionLayer::SuplAssistant::IpV4Address{status->ipaddr};
    }

    static void on_request_utc_time(void* context)
    {
        VLOG(1) << __PRETTY_FUNCTION__;

        auto thiz = static_cast<impl::HardwareAbstractionLayer*>(context);

        if (thiz->impl.utc_time_request_handler)
        {
            thiz->impl.utc_time_request_handler();
        } else
        {
            auto now = location::Clock::now().time_since_epoch();
            auto thiz = static_cast<impl::HardwareAbstractionLayer*>(context);

            static const int zero_uncertainty = 0;

            u_hardware_gps_inject_time(
                        thiz->impl.gps_handle,
                        now.count(),
                        now.count(),
                        zero_uncertainty);
        }
    }

    HardwareAbstractionLayer() : impl(this)
    {
    }

    std::uint32_t capabilities() const
    {
        return impl.capabilities;
    }

    std::uint32_t& capabilities()
    {
        return impl.capabilities;
    }

    // From gps::HardwareAbstractionLayer
    gps::HardwareAbstractionLayer::SuplAssistant& supl_assistant()
    {
        return impl.supl_assistant;
    }

    const core::Signal<location::Position>& position_updates() const
    {
        return impl.position_updates;
    }

    core::Signal<location::Position>& position_updates()
    {
        return impl.position_updates;
    }

    const core::Signal<location::Heading>& heading_updates() const
    {
        return impl.heading_updates;
    }

    core::Signal<location::Heading>& heading_updates()
    {
        return impl.heading_updates;
    }

    const core::Signal<location::Velocity>& velocity_updates() const
    {
        return impl.velocity_updates;
    }

    core::Signal<location::Velocity>& velocity_updates()
    {
        return impl.velocity_updates;
    }

    const core::Signal<std::set<location::SpaceVehicle>>& space_vehicle_updates() const
    {
        return impl.space_vehicle_updates;
    }

    core::Signal<std::set<location::SpaceVehicle>>& space_vehicle_updates()
    {
        return impl.space_vehicle_updates;
    }

    void delete_all_aiding_data()
    {
        // TODO(tvoss): We should expose this flag in gps.h.
        static const std::uint16_t delete_all = 0xFFFF;
        u_hardware_gps_delete_aiding_data(impl.gps_handle, delete_all);
    }

    const core::Property<gps::ChipsetStatus>& chipset_status() const
    {
        return impl.chipset_status;
    }

    core::Property<gps::ChipsetStatus>& chipset_status()
    {
        return impl.chipset_status;
    }

    bool is_capable_of(gps::AssistanceMode mode) const
    {
        bool result = false;

        switch(mode)
        {
        case gps::AssistanceMode::standalone:
            result = true;
            break;
        case gps::AssistanceMode::mobile_station_assisted:
            result = capabilities() & U_HARDWARE_GPS_CAPABILITY_MSA;
            break;
        case gps::AssistanceMode::mobile_station_based:
            result = capabilities() & U_HARDWARE_GPS_CAPABILITY_MSB;
            break;
        }

        return result;
    }

    bool is_capable_of(gps::PositionMode mode) const
    {
        bool result = false;
        switch(mode)
        {
        case gps::PositionMode::single_shot:
            result = capabilities() & U_HARDWARE_GPS_CAPABILITY_SINGLE_SHOT;
            break;
        case gps::PositionMode::periodic:
            result = capabilities() & U_HARDWARE_GPS_CAPABILITY_SCHEDULING;
            break;
        }

        return result;
    }

    bool is_capable_of(gps::Capability capability) const
    {
        bool result = false;

        switch(capability)
        {
        case gps::Capability::on_demand_time_injection:
            result = capabilities() & U_HARDWARE_GPS_CAPABILITY_ON_DEMAND_TIME;
            break;
        default:
            break;
        }

        return result;
    }

    bool start_positioning()
    {
        VLOG(1) << __PRETTY_FUNCTION__ << ": " << this << ", " << impl.gps_handle;
        return u_hardware_gps_start(impl.gps_handle);
    }

    bool stop_positioning()
    {
        VLOG(1) << __PRETTY_FUNCTION__ << ": " << this << ", " << impl.gps_handle;
        return u_hardware_gps_stop(impl.gps_handle);
    }

    bool set_assistance_mode(gps::AssistanceMode mode)
    {
        if (!is_capable_of(mode))
            return false;

        impl.assistance_mode = mode;
        return impl.dispatch_updated_modes_to_driver();
    }

    bool set_position_mode(gps::PositionMode mode)
    {
        switch(mode)
        {
        case gps::PositionMode::single_shot:
            if (!(capabilities() & U_HARDWARE_GPS_CAPABILITY_SINGLE_SHOT))
                return false;
            break;
        case gps::PositionMode::periodic:
            if (!(capabilities() & U_HARDWARE_GPS_CAPABILITY_SCHEDULING))
                return false;
            break;
        }

        impl.position_mode = mode;
        return impl.dispatch_updated_modes_to_driver();
    }

    virtual bool inject_reference_position(const location::Position& position)
    {
        // TODO(tvoss): We should expose the int return type of the underyling
        //  Android HAL to capture errors here.
        u_hardware_gps_inject_location(impl.gps_handle,
                                       position.latitude.value.value(),
                                       position.longitude.value.value(),
                                       position.accuracy.horizontal ?
                                           position.accuracy.horizontal->value() : 0);
        return true;
    }

    virtual bool inject_reference_time(const std::chrono::microseconds& reference_time,
                                       const std::chrono::microseconds& sample_time)
    {
        if (!is_capable_of(gps::Capability::on_demand_time_injection))
            return false;

        // TODO(tvoss): We should expose the int return type of the underyling
        //  Android HAL to capture errors here.
        u_hardware_gps_inject_time(impl.gps_handle,
                                   reference_time.count(),
                                   sample_time.count(),
                                   10);
        return true;
    }

    struct Impl
    {
        Impl(HardwareAbstractionLayer* parent)
            : capabilities(0),
              assistance_mode(gps::AssistanceMode::standalone),
              position_mode(gps::PositionMode::periodic)
        {
            ::memset(&gps_params, 0, sizeof(gps_params));

            gps_params.location_cb = HardwareAbstractionLayer::on_location_update;
            gps_params.status_cb = HardwareAbstractionLayer::on_status_update;
            gps_params.sv_status_cb = HardwareAbstractionLayer::on_sv_status_update;
            gps_params.set_capabilities_cb = HardwareAbstractionLayer::on_set_capabilities;
            gps_params.request_utc_time_cb = HardwareAbstractionLayer::on_request_utc_time;
            gps_params.agps_status_cb = HardwareAbstractionLayer::on_agps_status_update;

            gps_params.nmea_cb = HardwareAbstractionLayer::on_nmea_update;
            gps_params.xtra_download_request_cb = HardwareAbstractionLayer::on_xtra_download_request;
            gps_params.gps_ni_notify_cb = HardwareAbstractionLayer::on_gps_ni_notify;
            gps_params.request_setid_cb = HardwareAbstractionLayer::on_ril_request_set_id;
            gps_params.request_refloc_cb = HardwareAbstractionLayer::on_ril_request_reference_location;

            gps_params.context = parent;

            gps_handle = u_hardware_gps_new(std::addressof(gps_params));
        }

        bool dispatch_updated_modes_to_driver()
        {
            std::map<gps::AssistanceMode, std::uint32_t> assistance_mode_lut =
            {
                {gps::AssistanceMode::mobile_station_assisted, U_HARDWARE_GPS_POSITION_MODE_MS_ASSISTED},
                {gps::AssistanceMode::mobile_station_based, U_HARDWARE_GPS_POSITION_MODE_MS_BASED},
                {gps::AssistanceMode::standalone, U_HARDWARE_GPS_POSITION_MODE_STANDALONE}
            };

            std::map<gps::PositionMode, std::uint32_t> position_mode_lut =
            {
                {gps::PositionMode::periodic, U_HARDWARE_GPS_POSITION_RECURRENCE_PERIODIC},
                {gps::PositionMode::single_shot, U_HARDWARE_GPS_POSITION_RECURRENCE_SINGLE}
            };

            auto am = assistance_mode_lut.at(assistance_mode);
            auto pm = position_mode_lut.at(position_mode);

            static const uint32_t preferred_accuracy_in_meters = 1;
            static const uint32_t preferred_ttff_in_ms = 0;
            static const std::chrono::milliseconds min_interval{500};

            return u_hardware_gps_set_position_mode(
                        gps_handle,
                        am,
                        pm,
                        min_interval.count(),
                        preferred_accuracy_in_meters,
                        preferred_ttff_in_ms);
        }

        UHardwareGpsParams gps_params;
        UHardwareGps gps_handle;

        std::uint32_t capabilities;
        gps::AssistanceMode assistance_mode;
        gps::PositionMode position_mode;

        SuplAssistant supl_assistant;

        gps::HardwareAbstractionLayer::UtcTimeRequestHandler utc_time_request_handler;

        core::Signal<std::set<location::SpaceVehicle>> space_vehicle_updates;
        core::Signal<location::Position> position_updates;
        core::Signal<location::Heading> heading_updates;
        core::Signal<location::Velocity> velocity_updates;
        core::Property<gps::ChipsetStatus> chipset_status;
    } impl;
};
}

std::shared_ptr<gps::HardwareAbstractionLayer> gps::HardwareAbstractionLayer::create_default_instance()
{
    return std::shared_ptr<gps::HardwareAbstractionLayer>(new impl::HardwareAbstractionLayer());
}
