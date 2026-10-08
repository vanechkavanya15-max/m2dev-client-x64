#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>

struct RenderQueueEntry {
    uint64_t key;
    void* data;
};

template <typename T, typename KeyExtractor>
void RadixSort(T* __restrict data, size_t count, T* __restrict temp, KeyExtractor keyExt) noexcept {
    if (count <= 1) return;

    size_t histograms[8][256] = {0};

    // Calculate histograms for all passes in one go.
    for (size_t i = 0; i < count; ++i) {
        uint64_t k = keyExt(data[i]);
        ++histograms[0][(k      ) & 0xFF];
        ++histograms[1][(k >>  8) & 0xFF];
        ++histograms[2][(k >> 16) & 0xFF];
        ++histograms[3][(k >> 24) & 0xFF];
        ++histograms[4][(k >> 32) & 0xFF];
        ++histograms[5][(k >> 40) & 0xFF];
        ++histograms[6][(k >> 48) & 0xFF];
        ++histograms[7][(k >> 56) & 0xFF];
    }

    T* __restrict src = data;
    T* __restrict dst = temp;

    for (size_t pass = 0; pass < 8; ++pass) {
        const size_t* __restrict hist = histograms[pass];

        if (hist[0] == count) {
            continue; 
        }

        size_t offsets[256];
        size_t sum = 0;
        for (size_t i = 0; i < 256; ++i) {
            offsets[i] = sum;
            sum += hist[i];
        }

        const size_t shift = pass * 8;

        for (size_t i = 0; i < count; ++i) {
            T v = src[i];
            size_t b = (keyExt(v) >> shift) & 0xFF;
            dst[offsets[b]++] = v;
        }

        T* tmp = src;
        src = dst;
        dst = tmp;
    }

    if (src != data) {
        std::memcpy(data, temp, count * sizeof(T));
    }
}

void Sort64(RenderQueueEntry* entries, size_t count, RenderQueueEntry* tempBuffer) noexcept;

