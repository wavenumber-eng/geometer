#include "geometer/step_to_glb_options_json.h"

#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <initializer_list>
#include <string>

namespace geometer
{
namespace
{

void set_status(Status* status, int code, const std::string& message)
{
    if (status == nullptr)
    {
        return;
    }
    status->code = code;
    status->message = message;
}

class Parser
{
  public:
    explicit Parser(const char* text) : text_(text == nullptr ? "" : text) {}

    bool parse(StepToGlbOptions* options, std::string* error)
    {
        skip_ws();
        if (eof())
        {
            return true;
        }
        if (consume_literal("null"))
        {
            skip_ws();
            if (!eof())
            {
                *error = "Unexpected trailing characters in STEP-to-GLB options JSON.";
                return false;
            }
            return true;
        }
        if (!parse_object(options, error))
        {
            return false;
        }
        skip_ws();
        if (!eof())
        {
            *error = "Unexpected trailing characters in STEP-to-GLB options JSON.";
            return false;
        }
        return true;
    }

  private:
    bool eof() const
    {
        return pos_ >= text_.size();
    }

    char peek() const
    {
        return eof() ? '\0' : text_[pos_];
    }

    char take()
    {
        return eof() ? '\0' : text_[pos_++];
    }

    void skip_ws()
    {
        while (!eof() && std::isspace(static_cast<unsigned char>(peek())) != 0)
        {
            ++pos_;
        }
    }

    bool consume(char expected)
    {
        skip_ws();
        if (peek() != expected)
        {
            return false;
        }
        ++pos_;
        return true;
    }

    bool consume_literal(const char* literal)
    {
        skip_ws();
        const std::string expected(literal);
        if (text_.compare(pos_, expected.size(), expected) != 0)
        {
            return false;
        }
        pos_ += expected.size();
        return true;
    }

    bool parse_object(StepToGlbOptions* options, std::string* error)
    {
        if (!consume('{'))
        {
            *error = "STEP-to-GLB options JSON must be an object.";
            return false;
        }
        skip_ws();
        if (consume('}'))
        {
            return true;
        }

        while (true)
        {
            std::string key;
            if (!parse_string(&key, error))
            {
                return false;
            }
            if (!consume(':'))
            {
                *error = "Expected colon in STEP-to-GLB options JSON object.";
                return false;
            }
            if (matches(key, {"linear_deflection", "linearDeflection", "deflection"}))
            {
                if (!parse_positive_number(&options->linear_deflection, error, "linear_deflection"))
                {
                    return false;
                }
            }
            else if (matches(key, {"angular_deflection", "angularDeflection", "angular"}))
            {
                if (!parse_positive_number(&options->angular_deflection, error,
                                           "angular_deflection"))
                {
                    return false;
                }
            }
            else if (!skip_value(error))
            {
                return false;
            }

            skip_ws();
            if (consume('}'))
            {
                return true;
            }
            if (!consume(','))
            {
                *error = "Expected comma in STEP-to-GLB options JSON object.";
                return false;
            }
        }
    }

    bool parse_string(std::string* value, std::string* error)
    {
        if (!consume('"'))
        {
            *error = "Expected string in STEP-to-GLB options JSON.";
            return false;
        }
        value->clear();
        while (!eof())
        {
            const char ch = take();
            if (ch == '"')
            {
                return true;
            }
            if (ch != '\\')
            {
                value->push_back(ch);
                continue;
            }
            if (eof())
            {
                *error = "Invalid string escape in STEP-to-GLB options JSON.";
                return false;
            }
            const char escaped = take();
            switch (escaped)
            {
            case '"':
            case '\\':
            case '/':
                value->push_back(escaped);
                break;
            case 'b':
                value->push_back('\b');
                break;
            case 'f':
                value->push_back('\f');
                break;
            case 'n':
                value->push_back('\n');
                break;
            case 'r':
                value->push_back('\r');
                break;
            case 't':
                value->push_back('\t');
                break;
            default:
                *error = "Unsupported string escape in STEP-to-GLB options JSON.";
                return false;
            }
        }
        *error = "Unterminated string in STEP-to-GLB options JSON.";
        return false;
    }

    bool parse_positive_number(double* value, std::string* error, const char* field_name)
    {
        skip_ws();
        const char* start = text_.c_str() + pos_;
        char* end = nullptr;
        const double parsed = std::strtod(start, &end);
        if (end == start || !std::isfinite(parsed))
        {
            *error = std::string(field_name) + " must be a finite number.";
            return false;
        }
        if (parsed <= 0.0)
        {
            *error = std::string(field_name) + " must be greater than zero.";
            return false;
        }
        pos_ += static_cast<std::size_t>(end - start);
        *value = parsed;
        return true;
    }

    bool skip_value(std::string* error)
    {
        skip_ws();
        switch (peek())
        {
        case '{':
            return skip_object(error);
        case '[':
            return skip_array(error);
        case '"':
        {
            std::string ignored;
            return parse_string(&ignored, error);
        }
        case 't':
            return consume_expected_literal("true", error);
        case 'f':
            return consume_expected_literal("false", error);
        case 'n':
            return consume_expected_literal("null", error);
        default:
            if (peek() == '-' || std::isdigit(static_cast<unsigned char>(peek())) != 0)
            {
                return skip_number(error);
            }
            *error = "Unexpected token in STEP-to-GLB options JSON.";
            return false;
        }
    }

    bool skip_object(std::string* error)
    {
        if (!consume('{'))
        {
            *error = "Expected object in STEP-to-GLB options JSON.";
            return false;
        }
        skip_ws();
        if (consume('}'))
        {
            return true;
        }
        while (true)
        {
            std::string key;
            if (!parse_string(&key, error))
            {
                return false;
            }
            if (!consume(':') || !skip_value(error))
            {
                *error = error->empty() ? "Invalid object in STEP-to-GLB options JSON." : *error;
                return false;
            }
            skip_ws();
            if (consume('}'))
            {
                return true;
            }
            if (!consume(','))
            {
                *error = "Expected comma in STEP-to-GLB options JSON object.";
                return false;
            }
        }
    }

    bool skip_array(std::string* error)
    {
        if (!consume('['))
        {
            *error = "Expected array in STEP-to-GLB options JSON.";
            return false;
        }
        skip_ws();
        if (consume(']'))
        {
            return true;
        }
        while (true)
        {
            if (!skip_value(error))
            {
                return false;
            }
            skip_ws();
            if (consume(']'))
            {
                return true;
            }
            if (!consume(','))
            {
                *error = "Expected comma in STEP-to-GLB options JSON array.";
                return false;
            }
        }
    }

    bool skip_number(std::string* error)
    {
        const char* start = text_.c_str() + pos_;
        char* end = nullptr;
        const double parsed = std::strtod(start, &end);
        if (end == start || !std::isfinite(parsed))
        {
            *error = "Invalid number in STEP-to-GLB options JSON.";
            return false;
        }
        pos_ += static_cast<std::size_t>(end - start);
        return true;
    }

    bool consume_expected_literal(const char* literal, std::string* error)
    {
        if (!consume_literal(literal))
        {
            *error = "Invalid literal in STEP-to-GLB options JSON.";
            return false;
        }
        return true;
    }

    bool matches(const std::string& value, std::initializer_list<const char*> keys) const
    {
        for (const char* key : keys)
        {
            if (value == key)
            {
                return true;
            }
        }
        return false;
    }

    std::string text_;
    std::size_t pos_ = 0;
};

} // namespace

int parse_step_to_glb_options_json(const char* json, StepToGlbOptions* options, Status* status)
{
    if (options == nullptr)
    {
        set_status(status, 92, "STEP-to-GLB options pointer is null.");
        return 92;
    }

    StepToGlbOptions parsed = *options;
    std::string error;
    Parser parser(json);
    if (!parser.parse(&parsed, &error))
    {
        set_status(status, 91, error);
        return 91;
    }

    *options = parsed;
    set_status(status, 0, "");
    return 0;
}

} // namespace geometer
