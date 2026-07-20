#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <cstdint>

struct Tuple {
    std::vector<std::string> values;
    int rid = -1;    // pageId*1000+slot; -1 si no aplica (ej. resultado de JOIN)

    std::string serialize() const {
        std::ostringstream oss;
        for (size_t i = 0; i < values.size(); i++) {
            if (i) oss << '|';
            oss << values[i];
        }
        return oss.str();
    }

    static Tuple deserialize(const char* data, uint16_t length) {
        Tuple t;
        std::string s(data, length);
        std::stringstream ss(s);
        std::string field;

        while (std::getline(ss, field, '|')) t.values.push_back(field);
        return t;
    }
};
