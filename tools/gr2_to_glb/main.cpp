#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include "GrannyExtractor.h"
#include "GlbWriter.h"

namespace fs = std::filesystem;

void PrintUsage() {
    std::cout << "Uzycie: gr2_to_glb --input <plik.gr2> --output <plik.glb>\n"
              << "Opcjonalnie: --batch <katalog_zrodlowy> [--outdir <katalog_docelowy>]\n";
}

bool ConvertSingle(const std::string& inputPath, const std::string& outputPath) {
    GrannyExtractor extractor;
    if (!extractor.Load(inputPath)) {
        std::cerr << "Nie udalo sie zaladowac: " << inputPath << "\n";
        return false;
    }

    GlbWriter writer;
    if (!writer.Write(outputPath, extractor)) {
        std::cerr << "Nie udalo sie zapisac: " << outputPath << "\n";
        return false;
    }

    return true;
}

int main(int argc, char** argv) {
    std::string inputPath;
    std::string outputPath;
    std::string batchDir;
    std::string outDir;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--input" && i + 1 < argc) {
            inputPath = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            outputPath = argv[++i];
        } else if (arg == "--batch" && i + 1 < argc) {
            batchDir = argv[++i];
        } else if (arg == "--outdir" && i + 1 < argc) {
            outDir = argv[++i];
        } else {
            std::cerr << "Nieznany argument: " << arg << "\n";
            PrintUsage();
            return 1;
        }
    }

    if (!batchDir.empty()) {
        if (!fs::exists(batchDir) || !fs::is_directory(batchDir)) {
            std::cerr << "Katalog wsadowy nie istnieje: " << batchDir << "\n";
            return 1;
        }

        std::vector<fs::path> gr2Files;
        for (const auto& entry : fs::recursive_directory_iterator(batchDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".gr2") {
                gr2Files.push_back(entry.path());
            }
        }

        std::cout << "Znaleziono " << gr2Files.size() << " plikow .gr2 w " << batchDir << "\n";
        size_t successCount = 0;
        size_t failCount = 0;

        for (size_t i = 0; i < gr2Files.size(); ++i) {
            const auto& p = gr2Files[i];
            fs::path targetPath;
            if (!outDir.empty()) {
                fs::path relativePath = fs::relative(p, batchDir);
                targetPath = fs::path(outDir) / relativePath;
                targetPath.replace_extension(".glb");
                fs::create_directories(targetPath.parent_path());
            } else {
                targetPath = p;
                targetPath.replace_extension(".glb");
            }

            if (ConvertSingle(p.string(), targetPath.string())) {
                successCount++;
            } else {
                failCount++;
            }

            if ((i + 1) % 10 == 0 || i + 1 == gr2Files.size()) {
                std::cout << "Postep: [" << (i + 1) << "/" << gr2Files.size() << "]\n";
            }
        }

        std::cout << "Tryb wsadowy zakonczony. Sukces: " << successCount << ", Bledy: " << failCount << "\n";
        return failCount == 0 ? 0 : 1;
    }

    if (inputPath.empty() || outputPath.empty()) {
        PrintUsage();
        return 1;
    }

    if (!ConvertSingle(inputPath, outputPath)) {
        return 1;
    }

    std::cout << "Konwersja zakonczona sukcesem: " << outputPath << "\n";
    return 0;
}
