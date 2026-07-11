#pragma once

#include <ArduinoJson.h>
#include <cstdint>
#include <optional>
#include <string>

void set_opt_str(JsonDocument& doc, const char* key, const std::optional<std::string>& v);
void set_opt_i64(JsonDocument& doc, const char* key, const std::optional<int64_t>& v);
void set_opt_f64(JsonDocument& doc, const char* key, const std::optional<double>& v);

std::optional<std::string> get_opt_str(JsonVariantConst v);
std::optional<int64_t> get_opt_i64(JsonVariantConst v);
std::optional<double> get_opt_f64(JsonVariantConst v);
