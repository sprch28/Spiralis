#ifndef ____SP_STEG_IMAGE____
#define ____SP_STEG_IMAGE____
#pragma once
#include "../setup/init.hpp"
#include "../containers/array.hpp"
#include "../io/IO.hpp"

namespace sp { namespace steg{

// (For read_bytes())
// We wrap the returned data and size in this so the data is responsibly freed 
// (it's not tracked by anything else when read_bytes returns)
struct read_info{
    void* bytes;
    size_type size;
    ~read_info(){
        if(bytes) sp::free(bytes);
    }
};

//#pragma pack(push, 1)
struct bmp_header{
    bmp_header() : header(54) {}

    sp::vector<unsigned char> header;

    void fill_data(){
        if(header.size() < 54) return;

        // --- Bitmap File Header (14 bytes) ---
        identity[0] = header[0];
        identity[1] = header[1];
        file_size = *reinterpret_cast<int*>(header.data() + 2);
        reserved[0] = *reinterpret_cast<short*>(header.data() + 6);
        reserved[1] = *reinterpret_cast<short*>(header.data() + 8);
        data_start = *reinterpret_cast<int*>(header.data() + 10);

        // --- Bitmap Info Header (40 bytes) ---
        _bc_size = *reinterpret_cast<int*>(header.data() + 14);
        _bc_width = *reinterpret_cast<int*>(header.data() + 18);
        _bc_height = *reinterpret_cast<int*>(header.data() + 22);
        _bc_planes = *reinterpret_cast<short*>(header.data() + 26);
        _bc_bit_count = *reinterpret_cast<short*>(header.data() + 28);
        _bc_compression = *reinterpret_cast<int*>(header.data() + 30);
        _bc_image_size = *reinterpret_cast<int*>(header.data() + 34);
        _bc_x_pixels_per_meter = *reinterpret_cast<int*>(header.data() + 38);
        _bc_y_pixels_per_meter = *reinterpret_cast<int*>(header.data() + 42);
        _bc_colors_used = *reinterpret_cast<int*>(header.data() + 46);
        _bc_colors_important = *reinterpret_cast<int*>(header.data() + 50);
    }

    // --- Bitmap File Header (14 bytes) ---
    char identity[2]{};       // "BM" signature
    int file_size;            // Total file size in bytes
    short reserved[2]{};      // Reserved (must be 0)
    int data_start;           // Offset to actual pixel data

    // --- Bitmap Info Header (40 bytes) ---
    int _bc_size;             // Header size (typically 40)
    int _bc_width;            // Image width in pixels
    int _bc_height;           // Image height in pixels
    short _bc_planes;         // Number of color planes (must be 1)
    short _bc_bit_count;      // Bits per pixel (e.g., 24, 32)
    int _bc_compression;      // Compression type (0 for uncompressed)
    int _bc_image_size;       // Image size in bytes (can be 0 for uncompressed)
    int _bc_x_pixels_per_meter;// Horizontal resolution
    int _bc_y_pixels_per_meter;// Vertical resolution
    int _bc_colors_used;      // Number of colors in color palette
    int _bc_colors_important; // Number of important colors
};
//#pragma pack(pop)

bool write_bytes(void* bytes, size_type size, sp::IO& scanner){
    bmp_header h;
    unsigned char* byte_iter = (unsigned char*)(bytes);
    
    // Load the first 54 bytes into the bmp_header struct.
    SP_IF_NOT_EXPECT(!scanner.read(h.header)) return false;
    h.fill_data(); 

    // Total bits needed: size of the size_type variable + the actual payload bytes
    ull total_bits_needed = (sizeof(size_type) + size) * 8;
    if(h.data_start + total_bits_needed > (ull)h.file_size) return false;

    scanner.write(h.header);

    ull idx = h.data_start;

    // Size
    for(ull i = 0; i < sizeof(size_type) * 8; ++i, ++idx){
        unsigned char cur_byte = scanner.read<unsigned char>().first;
        unsigned char new_byte = ((size >> i) & 1) ? (cur_byte | 1) : (cur_byte & (~1));
        scanner.write<unsigned char>(new_byte);
    }

    // Data
    ull bit_idx = 0;
    for(ull i = 0; i < size * 8; ++i, ++idx){
        unsigned char cur_byte = scanner.read<unsigned char>().first;
        unsigned char new_byte = ((*byte_iter >> (bit_idx++)) & 1) ? (cur_byte | 1) : (cur_byte & (~1));
        scanner.write<unsigned char>(new_byte);
        if(bit_idx == 8){
            bit_idx = 0;
            ++byte_iter;
        }
    }

    // Remaining
    for(; idx < (ull)h.file_size; ++idx){
        scanner.write<unsigned char>(scanner.read<unsigned char>().first);
    }

    return true;
}

read_info read_bytes(sp::IO& scanner){
    bmp_header h;
    
    // Load the first 54 bytes into the bmp_header struct.
    SP_IF_NOT_EXPECT(!scanner.read(h.header)) return {nullptr, 0};
    h.fill_data(); 

    size_type size_bytes = 0;

    for(ull i = 0; i < sizeof(size_type) * 8; ++i){
        unsigned char cur = scanner.read<unsigned char>().first;
        size_bytes |= ((static_cast<size_type>(cur & 1)) << i);
    }

    ull total_bits_needed = size_bytes * 8;
    SP_IF_NOT_EXPECT(h.data_start + total_bits_needed > (ull)h.file_size){
        return {nullptr, 0};
    }

    unsigned char* data = static_cast<unsigned char*>(sp::malloc(size_bytes));

    for(size_type i = 0; i < size_bytes; ++i){
        data[i] = 0;
        for(ull j = 0; j < 8; ++j){
            unsigned char cur_byte = scanner.read<unsigned char>().first;
            data[i] |= ((cur_byte & 1) << j);
        }
    }

    return {data, size_bytes};
}

}}
#endif // ____SP_STEG_IMAGE____