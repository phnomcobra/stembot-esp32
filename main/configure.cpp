// configure.cpp
// Serial TUI for configuring the stembot agent after flashing.
//
// Uses the ESP-IDF console component (linenoise-based REPL) over UART0.
// The REPL runs in its own FreeRTOS task started by event_loop(), so it
// never blocks the main agent process.
//
// Note: no external Arduino Serial or WiFi managed components are used.
// The built-in ESP-IDF console (esp_console) and WiFi (esp_wifi) components
// cover all required functionality for an IDF-based project.

#include "argtable3/argtable3.h"
#include "config.hpp"
#include "control.hpp"
#include "esp_console.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <string>

// Global Config instance owned by the TUI.
static Config g_config;

// ── WiFi ─────────────────────────────────────────────────────────────────────

static bool s_wifi_initialized = false;
static bool s_event_loop_created = false;

static void on_wifi_event(void* /*arg*/, esp_event_base_t /*base*/, int32_t event_id,
                          void* /*data*/)
{
    if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        printf("\r\n[wifi] disconnected\r\n");
    }
}

static void on_ip_event(void* /*arg*/, esp_event_base_t /*base*/, int32_t event_id, void* data)
{
    if (event_id == IP_EVENT_STA_GOT_IP)
    {
        const auto* ev = static_cast<ip_event_got_ip_t*>(data);
        printf("\r\n[wifi] connected — IP: " IPSTR "\r\n", IP2STR(&ev->ip_info.ip));
    }
}

static esp_err_t wifi_sta_connect()
{
    if (g_config.wifiSSID.empty())
    {
        printf("Error: wifiSSID is not set. Use: set wifiSSID <name>\r\n");
        return ESP_ERR_INVALID_STATE;
    }

    if (!s_event_loop_created)
    {
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        s_event_loop_created = true;
    }

    if (!s_wifi_initialized)
    {
        ESP_ERROR_CHECK(esp_netif_init());
        esp_netif_create_default_wifi_sta();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                            on_wifi_event, nullptr, nullptr));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                            on_ip_event, nullptr, nullptr));

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        s_wifi_initialized = true;
    }

    wifi_config_t wifi_cfg = {};
    strncpy(reinterpret_cast<char*>(wifi_cfg.sta.ssid), g_config.wifiSSID.c_str(),
            sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy(reinterpret_cast<char*>(wifi_cfg.sta.password), g_config.wifiPassword.c_str(),
            sizeof(wifi_cfg.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    esp_err_t err = esp_wifi_start();
    if (err == ESP_OK)
    {
        esp_wifi_connect();
        printf("Connecting to '%s'…\r\n", g_config.wifiSSID.c_str());
    }
    return err;
}

// ── cmd: list ─────────────────────────────────────────────────────────────────

static int cmd_list(int /*argc*/, char** /*argv*/)
{
    char key_hex[65] = {};
    for (int i = 0; i < 32; i++)
    {
        snprintf(key_hex + i * 2, 3, "%02x", static_cast<unsigned>(g_config.key[i]));
    }
    printf("  %-16s %.*s\r\n", "agtuuid:", 36, g_config.agtuuid);
    printf("  %-16s %s\r\n", "key:", key_hex);
    printf("  %-16s %s\r\n", "peerUrl:", g_config.peerUrl.c_str());
    printf("  %-16s %s\r\n", "wifiSSID:", g_config.wifiSSID.c_str());
    printf("  %-16s %s\r\n",
           "wifiPassword:", g_config.wifiPassword.empty() ? "(not set)" : "(hidden)");
    return 0;
}

// ── cmd: set ──────────────────────────────────────────────────────────────────

static struct
{
    struct arg_str* field;
    struct arg_str* value;
    struct arg_end* end;
} s_set_args;

static int cmd_set(int argc, char** argv)
{
    if (arg_parse(argc, argv, reinterpret_cast<void**>(&s_set_args)) != 0)
    {
        arg_print_errors(stdout, s_set_args.end, argv[0]);
        return 1;
    }
    const char* field = s_set_args.field->sval[0];
    const char* value = s_set_args.value->sval[0];

    if (strcmp(field, "agtuuid") == 0)
    {
        strncpy(g_config.agtuuid, value, sizeof(g_config.agtuuid) - 1);
        g_config.agtuuid[sizeof(g_config.agtuuid) - 1] = '\0';
    }
    else if (strcmp(field, "peerUrl") == 0)
    {
        g_config.peerUrl = value;
    }
    else if (strcmp(field, "wifiSSID") == 0)
    {
        g_config.wifiSSID = value;
    }
    else if (strcmp(field, "wifiPassword") == 0)
    {
        g_config.wifiPassword = value;
    }
    else if (strcmp(field, "passphrase") == 0)
    {
        g_config.set_passphrase(value);
    }
    else
    {
        printf("Unknown field '%s'.\r\n"
               "Valid fields: agtuuid, peerUrl, wifiSSID, wifiPassword, passphrase\r\n",
               field);
        return 1;
    }
    g_config.save();
    printf("OK\r\n");
    return 0;
}

// ── cmd: wifi_connect ─────────────────────────────────────────────────────────

static int cmd_wifi_connect(int /*argc*/, char** /*argv*/)
{
    esp_err_t err = wifi_sta_connect();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        printf("WiFi start failed: %s\r\n", esp_err_to_name(err));
        return 1;
    }
    return err == ESP_OK ? 0 : 1;
}

// ── Control form stubs ────────────────────────────────────────────────────────
// Each stub corresponds to a control form variant in control.h / control.rs.
// Replace the printf body with real dispatch logic when implementing the agent.

static int cmd_create_peer(int /*argc*/, char** /*argv*/)
{
    CreatePeer form{};
    printf("[stub] create_peer — would dispatch: %s\r\n", form.to_json().c_str());
    return 0;
}

static int cmd_discover_peer(int /*argc*/, char** /*argv*/)
{
    DiscoverPeer form{};
    printf("[stub] discover_peer — would dispatch: %s\r\n", form.to_json().c_str());
    return 0;
}

static int cmd_delete_peers(int /*argc*/, char** /*argv*/)
{
    DeletePeers form{};
    printf("[stub] delete_peers — would dispatch: %s\r\n", form.to_json().c_str());
    return 0;
}

static int cmd_get_peers(int /*argc*/, char** /*argv*/)
{
    GetPeers form{};
    printf("[stub] get_peers — would dispatch: %s\r\n", form.to_json().c_str());
    return 0;
}

static int cmd_get_routes(int /*argc*/, char** /*argv*/)
{
    GetRoutes form{};
    printf("[stub] get_routes — would dispatch: %s\r\n", form.to_json().c_str());
    return 0;
}

static int cmd_sync_process(int /*argc*/, char** /*argv*/)
{
    SyncProcess form{};
    printf("[stub] sync_process — would dispatch: %s\r\n", form.to_json().c_str());
    return 0;
}

static int cmd_write_file(int /*argc*/, char** /*argv*/)
{
    WriteFile form{};
    printf("[stub] write_file — would dispatch: %s\r\n", form.to_json().c_str());
    return 0;
}

static int cmd_load_file(int /*argc*/, char** /*argv*/)
{
    LoadFile form{};
    printf("[stub] load_file — would dispatch: %s\r\n", form.to_json().c_str());
    return 0;
}

static int cmd_get_config(int /*argc*/, char** /*argv*/)
{
    GetConfig form{};
    printf("[stub] get_config — would dispatch: %s\r\n", form.to_json().c_str());
    return 0;
}

// ── Command registration ──────────────────────────────────────────────────────

// Build a fully zero-initialised esp_console_cmd_t to satisfy -Wmissing-field-initializers.
static esp_console_cmd_t make_cmd(const char* name, const char* help, esp_console_cmd_func_t fn,
                                  void* argtable = nullptr)
{
    esp_console_cmd_t cmd = {};
    cmd.command = name;
    cmd.help = help;
    cmd.hint = nullptr;
    cmd.func = fn;
    cmd.argtable = argtable;
    cmd.func_w_context = nullptr;
    cmd.context = nullptr;
    return cmd;
}

static void register_commands()
{
    // list
    esp_console_cmd_t list_cmd = make_cmd("list", "Show all configuration settings", cmd_list);
    ESP_ERROR_CHECK(esp_console_cmd_register(&list_cmd));

    // set
    s_set_args.field = arg_str1(nullptr, nullptr, "<field>",
                                "agtuuid | peerUrl | wifiSSID | wifiPassword | passphrase");
    s_set_args.value = arg_str1(nullptr, nullptr, "<value>", "New value");
    s_set_args.end = arg_end(2);
    esp_console_cmd_t set_cmd =
        make_cmd("set", "Set a configuration field and persist it to NVS", cmd_set, &s_set_args);
    ESP_ERROR_CHECK(esp_console_cmd_register(&set_cmd));

    // wifi_connect
    esp_console_cmd_t wifi_cmd =
        make_cmd("wifi_connect", "Connect to Wi-Fi using stored credentials", cmd_wifi_connect);
    ESP_ERROR_CHECK(esp_console_cmd_register(&wifi_cmd));

    // Control form stubs
    static const struct
    {
        const char* name;
        const char* help;
        esp_console_cmd_func_t fn;
    } stubs[] = {
        {"create_peer", "Create a peer connection (stub)", cmd_create_peer},
        {"discover_peer", "Discover a peer via URL (stub)", cmd_discover_peer},
        {"delete_peers", "Delete one or more peers (stub)", cmd_delete_peers},
        {"get_peers", "List known peers (stub)", cmd_get_peers},
        {"get_routes", "Show routing table (stub)", cmd_get_routes},
        {"sync_process", "Execute a process on a remote agent (stub)", cmd_sync_process},
        {"write_file", "Write a file to a remote agent (stub)", cmd_write_file},
        {"load_file", "Load a file from a remote agent (stub)", cmd_load_file},
        {"get_config", "Fetch agent configuration (stub)", cmd_get_config},
    };
    for (const auto& s : stubs)
    {
        esp_console_cmd_t cmd = make_cmd(s.name, s.help, s.fn);
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    }
}

// ── Public entry point ────────────────────────────────────────────────────────
// Initialises the console REPL and returns immediately; the REPL runs in
// its own FreeRTOS task so it never blocks app_main or the agent loop.

void event_loop()
{
    esp_console_repl_t* repl = nullptr;

    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "stembot> ";
    repl_cfg.max_cmdline_length = 256;
    repl_cfg.task_stack_size = 8192;
    repl_cfg.task_priority = 2;

    esp_console_dev_uart_config_t uart_cfg = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_cfg, &repl_cfg, &repl));

    esp_console_register_help_command();
    register_commands();

    printf("\r\n\r\n"
           "  stembot configuration console\r\n"
           "  Type 'help' for available commands.\r\n\r\n");

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
    // esp_console_start_repl() spawns the REPL task and returns — execution
    // continues in app_main while the TUI runs independently in the background.
}
