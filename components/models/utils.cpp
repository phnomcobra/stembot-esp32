#include "utils.h"

void set_opt_str(JsonDocument& doc, const char* key, const std::optional<std::string>& v)
{
    if (v)
    {
        doc[key] = *v;
    }
    else
    {
        doc[key] = nullptr;
    }
}

void set_opt_i64(JsonDocument& doc, const char* key, const std::optional<int64_t>& v)
{
    if (v)
    {
        doc[key] = *v;
    }
    else
    {
        doc[key] = nullptr;
    }
}

void set_opt_f64(JsonDocument& doc, const char* key, const std::optional<double>& v)
{
    if (v)
    {
        doc[key] = *v;
    }
    else
    {
        doc[key] = nullptr;
    }
}

std::optional<std::string> get_opt_str(JsonVariantConst v)
{
    if (v.isNull())
    {
        return std::nullopt;
    }
    return v.as<std::string>();
}

std::optional<int64_t> get_opt_i64(JsonVariantConst v)
{
    if (v.isNull())
    {
        return std::nullopt;
    }
    return v.as<int64_t>();
}

std::optional<double> get_opt_f64(JsonVariantConst v)
{
    if (v.isNull())
    {
        return std::nullopt;
    }
    return v.as<double>();
}
