#pragma once

#include "MoonrakerPrinterAgent.hpp"

#include <string>

namespace Slic3r {

// PrusaPrinterAgent - filament sync for Prusa Core One / Core One INDX.
//
// Pulls the per-tool filament type and color from the printer over the PrusaLink
// HTTP API (GET /api/v1/filament) and feeds it into OrcaSlicer's existing filament
// sync path (DevFilaSystem -> filament_ams_list -> PresetBundle::sync_ams_list).
//
// It derives from MoonrakerPrinterAgent purely to reuse build_ams_payload()/AmsTrayData
// and the IPrinterAgent boilerplate; it does NOT speak the Moonraker protocol. Therefore
// connect_printer() is overridden to a lightweight, non-streaming setup (no /server/info
// probe, no websocket status stream), and credentials/host are read from the printer
// preset's PrusaLink host config rather than the LAN access code.
class PrusaPrinterAgent final : public MoonrakerPrinterAgent
{
public:
    explicit PrusaPrinterAgent(std::string log_dir);
    ~PrusaPrinterAgent() override = default;

    static AgentInfo get_agent_info_static();
    AgentInfo        get_agent_info() override { return get_agent_info_static(); }

    // Pull-mode filament sync over PrusaLink (no MQTT / websocket).
    int  connect_printer(std::string dev_id, std::string dev_ip, std::string username, std::string password, bool use_ssl) override;
    int  disconnect_printer() override;
    bool fetch_filament_info(std::string dev_id) override;

private:
    // PrusaLink host connection settings read from the edited printer preset.
    struct HostConfig
    {
        std::string host;          // print_host (may already include http(s):// scheme)
        int         auth_type = 0; // AuthorizationType: atKeyPassword (0) or atUserPassword (1)
        std::string apikey;        // printhost_apikey
        std::string user;          // printhost_user
        std::string password;      // printhost_password
        std::string cafile;        // printhost_cafile
        bool        valid = false; // false when no print host is configured
    };

    static HostConfig  read_host_config();
    static std::string make_url(const std::string& host, const std::string& path);
};

} // namespace Slic3r
