// configure.cpp
// Serial TUI for configuring the stembot agent after flashing.
//
// Uses the ESP-IDF console component (linenoise-based REPL) over UART0.
// The REPL runs in its own FreeRTOS task started by event_loop(), so it
// never blocks the main agent process.

#include "apps/ping/ping_sock.h"
#include "argtable3/argtable3.h"
#include "config.hpp"
#include "control.hpp"
#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lwip/inet.h"
#include "lwip/ip_addr.h"
#include "lwip/netdb.h"
#include "network.hpp"

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <string>

// Global Config instance owned by the TUI.
static Config g_config;

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
    if (g_config.wifiSSID.empty())
    {
        printf("Error: wifiSSID is not set. Use: set wifiSSID <name>\r\n");
        return 1;
    }
    network_wifi_connect(g_config.wifiSSID, g_config.wifiPassword);
    return 0;
}

// ── cmd: ping ───────────────────────────────────────────────────────────────

static struct
{
    struct arg_str* host;
    struct arg_int* count;
    struct arg_end* end;
} s_ping_args;

static void on_ping_success(esp_ping_handle_t hdl, void* /*args*/)
{
    uint8_t ttl;
    uint16_t seqno;
    uint32_t elapsed_ms, recv_len;
    ip_addr_t target = {};
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_ms, sizeof(elapsed_ms));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target, sizeof(target));
    printf("  %" PRIu32 " bytes from %s  seq=%" PRIu16 "  ttl=%" PRIu8 "  time=%" PRIu32 " ms\r\n",
           recv_len, ipaddr_ntoa(&target), seqno, ttl, elapsed_ms);
}

static void on_ping_timeout(esp_ping_handle_t hdl, void* /*args*/)
{
    uint16_t seqno;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    printf("  Request timeout for seq %" PRIu16 "\r\n", seqno);
}

static void on_ping_end(esp_ping_handle_t hdl, void* args)
{
    uint32_t transmitted, received, duration_ms;
    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &duration_ms, sizeof(duration_ms));
    uint32_t loss = transmitted > 0 ? (transmitted - received) * 100 / transmitted : 0;
    printf("  --- %" PRIu32 " transmitted, %" PRIu32 " received, %" PRIu32 "%% loss, time %" PRIu32
           " ms\r\n",
           transmitted, received, loss, duration_ms);
    xSemaphoreGive(static_cast<SemaphoreHandle_t>(args));
}

static int cmd_ping(int argc, char** argv)
{
    if (arg_parse(argc, argv, reinterpret_cast<void**>(&s_ping_args)) != 0)
    {
        arg_print_errors(stdout, s_ping_args.end, argv[0]);
        return 1;
    }

    const char* host = s_ping_args.host->sval[0];
    const int count = (s_ping_args.count->count > 0) ? s_ping_args.count->ival[0] : 4;

    // Resolve hostname or IP string to an IPv4 address.
    struct addrinfo hints = {};
    struct addrinfo* res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    if (getaddrinfo(host, nullptr, &hints, &res) != 0 || res == nullptr)
    {
        printf("ping: cannot resolve '%s'\r\n", host);
        return 1;
    }
    ip_addr_t target = {};
    const auto* sa4 = reinterpret_cast<const sockaddr_in*>(res->ai_addr);
    inet_addr_to_ip4addr(ip_2_ip4(&target), &sa4->sin_addr);
    IP_SET_TYPE_VAL(target, IPADDR_TYPE_V4);
    freeaddrinfo(res);

    printf("PING %s (%s): 64 bytes of data\r\n", host, ipaddr_ntoa(&target));

    SemaphoreHandle_t done = xSemaphoreCreateBinary();

    esp_ping_config_t cfg = ESP_PING_DEFAULT_CONFIG();
    cfg.count = static_cast<uint32_t>(count);
    cfg.target_addr = target;

    esp_ping_callbacks_t cbs = {};
    cbs.on_ping_success = on_ping_success;
    cbs.on_ping_timeout = on_ping_timeout;
    cbs.on_ping_end = on_ping_end;
    cbs.cb_args = done;

    esp_ping_handle_t hdl = nullptr;
    if (esp_ping_new_session(&cfg, &cbs, &hdl) != ESP_OK)
    {
        printf("ping: failed to create session\r\n");
        vSemaphoreDelete(done);
        return 1;
    }

    esp_ping_start(hdl);
    // Block until on_ping_end signals completion.
    const TickType_t timeout_ticks =
        pdMS_TO_TICKS((uint32_t)count * (cfg.interval_ms + cfg.timeout_ms) + 2000);
    xSemaphoreTake(done, timeout_ticks);

    esp_ping_stop(hdl);
    esp_ping_delete_session(hdl);
    vSemaphoreDelete(done);
    return 0;
}

// ── cmd: net_info ─────────────────────────────────────────────────────────────

static int cmd_net_info(int /*argc*/, char** /*argv*/)
{
    esp_netif_t* netif = esp_netif_get_default_netif();
    if (netif == nullptr)
    {
        printf("  No active network interface.\r\n");
        return 0;
    }

    esp_netif_ip_info_t ip_info = {};
    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK)
    {
        printf("  %-12s " IPSTR "\r\n", "IP:", IP2STR(&ip_info.ip));
        printf("  %-12s " IPSTR "\r\n", "Netmask:", IP2STR(&ip_info.netmask));
        printf("  %-12s " IPSTR "\r\n", "Gateway:", IP2STR(&ip_info.gw));
    }

    esp_netif_dns_info_t dns_info = {};
    if (esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info) == ESP_OK)
    {
        printf("  %-12s " IPSTR "\r\n", "DNS:", IP2STR(&dns_info.ip.u_addr.ip4));
    }

    return 0;
}

// ── Control form stubs ────────────────────────────────────────────────────────
// Each stub corresponds to a control form variant in control.h / control.rs.
// Replace the printf body with real dispatch logic when implementing the agent.

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

    // ping
    s_ping_args.host = arg_str1(nullptr, nullptr, "<host>", "Hostname or IP address to ping");
    s_ping_args.count = arg_int0("c", "count", "<n>", "Number of packets to send (default: 4)");
    s_ping_args.end = arg_end(2);
    esp_console_cmd_t ping_cmd =
        make_cmd("ping", "Send ICMP echo requests to a host", cmd_ping, &s_ping_args);
    ESP_ERROR_CHECK(esp_console_cmd_register(&ping_cmd));

    // net_info
    esp_console_cmd_t net_info_cmd =
        make_cmd("net_info", "Show IP address, gateway, netmask, and DNS", cmd_net_info);
    ESP_ERROR_CHECK(esp_console_cmd_register(&net_info_cmd));

    // Control form stubs
    static const struct
    {
        const char* name;
        const char* help;
        esp_console_cmd_func_t fn;
    } stubs[] = {
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
