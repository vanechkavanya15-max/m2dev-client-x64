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

bool ConvertSingle(const fs::path& inputPath, const fs::path& outputPath) {
    try {
        GrannyExtractor extractor;
        if (!extractor.Load(inputPath)) {
            std::cerr << "Nie udalo sie zaladowac: " << (const char*)inputPath.u8string().c_str() << "\n";
            return false;
        }

        GlbWriter writer;
        if (!writer.Write(outputPath, extractor)) {
            std::cerr << "Nie udalo sie zapisac: " << (const char*)outputPath.u8string().c_str() << "\n";
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Wyjatek podczas konwersji " << (const char*)inputPath.u8string().c_str() << ": " << e.what() << "\n";
        return false;
    } catch (...) {
        std::cerr << "Nieznany wyjatek podczas konwersji " << (const char*)inputPath.u8string().c_str() << "\n";
        return false;
    }
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
        fs::path bPath = fs::u8path(batchDir);
        if (!fs::exists(bPath) || !fs::is_directory(bPath)) {
            std::cerr << "Katalog wsadowy nie istnieje: " << batchDir << "\n";
            return 1;
        }

        std::vector<fs::path> gr2Files;
        std::error_code ec;
        for (const auto& entry : fs::recursive_directory_iterator(bPath, fs::directory_options::skip_permission_denied, ec)) {
            if (entry.is_regular_file(ec) && entry.path().extension() == ".gr2") {
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
                fs::path relativePath = fs::relative(p, bPath);
                targetPath = fs::u8path(outDir) / relativePath;
                targetPath.replace_extension(".glb");
                fs::create_directories(targetPath.parent_path(), ec);
            } else {
                targetPath = p;
                targetPath.replace_extension(".glb");
            }

            if (ConvertSingle(p, targetPath)) {
                successCount++;
            } else {
                failCount++;
            }

            if ((i + 1) % 100 == 0 || i + 1 == gr2Files.size()) {
                std::cout << "Postep: [" << (i + 1) << "/" << gr2Files.size() << "] Sukces: " << successCount << ", Bledy: " << failCount << "\n";
            }
        }

        std::cout << "Tryb wsadowy zakonczony. Sukces: " << successCount << ", Bledy: " << failCount << "\n";
        return failCount == 0 ? 0 : 1;
    }

    if (inputPath.empty() || outputPath.empty()) {
        PrintUsage();
        return 1;
    }

    fs::path inPath = fs::u8path(inputPath);
    fs::path outPath = fs::u8path(outputPath);

    if (!ConvertSingle(inPath, outPath)) {
        return 1;
    }

    std::cout << "Konwersja zakonczona sukcesem: " << (const char*)outPath.u8string().c_str() << "\n";
    return 0;
}
