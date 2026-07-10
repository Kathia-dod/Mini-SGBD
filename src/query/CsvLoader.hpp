#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include "../buffer/BufferManager.hpp"

class CsvLoader {
public:
    static int load(const std::string& csvPath, BufferManager& bm) {
        std::ifstream file(csvPath);
        if (!file.is_open()) {
            std::cerr << "[CsvLoader] No se pudo abrir " << csvPath << "\n";
            return -1;
        }

        std::string line;
        bool firstLine = true;
        int inserted = 0;

        uint32_t pageId;
        Page* page = bm.newPage(pageId);

        while (std::getline(file, line)) {
            if (firstLine) { firstLine = false; continue; } // saltar encabezado
            if (line.empty()) continue;
            if (!line.empty() && line.back() == '\r') line.pop_back(); // CRLF

            for (auto& c : line) if (c == ',') c = '|'; // separador interno de Tuple

            int slot = page->insertRecord(line.c_str(), static_cast<uint16_t>(line.size()));
            if (slot == -1) {
                bm.unpinPage(pageId, true);
                page = bm.newPage(pageId);
                slot = page->insertRecord(line.c_str(), static_cast<uint16_t>(line.size()));
            }
            if (slot == -1) {
                std::cerr << "[CsvLoader] fila demasiado grande para una pagina, se omite: "
                          << line.substr(0, 40) << "...\n";
                continue;
            }
            inserted++;
        }
        bm.unpinPage(pageId, true);
        std::cout << "[CsvLoader] " << inserted << " registros cargados desde " << csvPath << "\n";
        return inserted;
    }
};