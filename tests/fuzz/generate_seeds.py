#!/usr/bin/env python3
"""
Corpus seed generator for Bitmap fuzz testing suite.
Generates diverse valid, corrupted-header, and truncated BMP files
to maximize libFuzzer and ASan/UBSan code coverage.
"""

import os
import sys
import struct
import argparse

def create_bmp_bytes(width, height, bpp, compression=0, bf_off_bits=54, bi_size=40,
                     planes=1, custom_magic=b'BM', custom_bf_size=None, payload=None):
    """Constructs raw BMP bytes with exact header and payload specification."""
    bytes_per_pixel = bpp // 8 if bpp >= 8 else 1
    row_stride = ((width * bytes_per_pixel + 3) // 4) * 4 if (0 < width <= 1024) else 4
    abs_height = min(abs(height), 1024)
    computed_image_size = (row_stride * abs_height) & 0xFFFFFFFF
    
    if payload is None:
        if width > 1024 or abs(height) > 1024:
            payload = b'\x00' * 16
        elif bpp == 24:
            row_data = b'\xAA\xBB\xCC' * width + b'\x00' * (row_stride - width * 3)
            payload = row_data * abs_height
        elif bpp == 32:
            row_data = b'\x10\x20\x30\xFF' * width + b'\x00' * (row_stride - width * 4)
            payload = row_data * abs_height
        else:
            payload = b'\x00' * min(computed_image_size, 4096)

    total_size = (bf_off_bits + len(payload)) & 0xFFFFFFFF
    if custom_bf_size is not None:
        total_size = custom_bf_size & 0xFFFFFFFF

    # BITMAPFILEHEADER (14 bytes)
    # Magic (2), Size (4), Res1 (2), Res2 (2), OffBits (4)
    file_header = struct.pack('<2sIHHI', custom_magic, total_size, 0, 0, bf_off_bits & 0xFFFFFFFF)

    # BITMAPINFOHEADER (40 bytes)
    # biSize (4), biWidth (4), biHeight (4), biPlanes (2), biBitCount (2),
    # biCompression (4), biSizeImage (4), biXPels (4), biYPels (4), biClrUsed (4), biClrImportant (4)
    info_header = struct.pack('<IiiHHIIiiII',
                              bi_size & 0xFFFFFFFF,
                              width,
                              height,
                              planes & 0xFFFF,
                              bpp & 0xFFFF,
                              compression & 0xFFFFFFFF,
                              computed_image_size,
                              2835, 2835, # ~72 DPI
                              0, 0)

    return file_header + info_header + payload


def generate_corpus(output_dir):
    """Populate directory with varied BMP seeds."""
    os.makedirs(output_dir, exist_ok=True)
    seeds = {}

    # --- 1. Valid Seeds ---
    seeds['valid_1x1_24bpp.bmp'] = create_bmp_bytes(1, 1, 24)
    seeds['valid_2x2_32bpp.bmp'] = create_bmp_bytes(2, 2, 32)
    seeds['valid_3x3_24bpp_padded.bmp'] = create_bmp_bytes(3, 3, 24)
    seeds['valid_4x4_32bpp.bmp'] = create_bmp_bytes(4, 4, 32)
    seeds['valid_topdown_2x2_32bpp.bmp'] = create_bmp_bytes(2, -2, 32)
    seeds['valid_topdown_3x3_24bpp.bmp'] = create_bmp_bytes(3, -3, 24)
    seeds['valid_8x8_24bpp.bmp'] = create_bmp_bytes(8, 8, 24)

    # --- 2. Corrupt Magic & Header Seeds ---
    seeds['corrupt_bad_magic_xx.bmp'] = create_bmp_bytes(2, 2, 32, custom_magic=b'XX')
    seeds['corrupt_bad_magic_null.bmp'] = create_bmp_bytes(2, 2, 32, custom_magic=b'\x00\x00')
    seeds['corrupt_bi_size_0.bmp'] = create_bmp_bytes(2, 2, 32, bi_size=0)
    seeds['corrupt_bi_size_12.bmp'] = create_bmp_bytes(2, 2, 32, bi_size=12)
    seeds['corrupt_bi_size_108.bmp'] = create_bmp_bytes(2, 2, 32, bi_size=108)
    seeds['corrupt_planes_0.bmp'] = create_bmp_bytes(2, 2, 32, planes=0)
    seeds['corrupt_planes_2.bmp'] = create_bmp_bytes(2, 2, 32, planes=2)
    seeds['corrupt_bpp_1.bmp'] = create_bmp_bytes(2, 2, 1)
    seeds['corrupt_bpp_4.bmp'] = create_bmp_bytes(2, 2, 4)
    seeds['corrupt_bpp_8.bmp'] = create_bmp_bytes(2, 2, 8)
    seeds['corrupt_bpp_16.bmp'] = create_bmp_bytes(2, 2, 16)
    seeds['corrupt_bpp_48.bmp'] = create_bmp_bytes(2, 2, 48)
    seeds['corrupt_bpp_0.bmp'] = create_bmp_bytes(2, 2, 0)
    seeds['corrupt_compression_rle8.bmp'] = create_bmp_bytes(2, 2, 24, compression=1)
    seeds['corrupt_compression_rle4.bmp'] = create_bmp_bytes(2, 2, 24, compression=2)
    seeds['corrupt_compression_bitfields.bmp'] = create_bmp_bytes(2, 2, 32, compression=3)

    # --- 3. Boundary & Dimension Extreme Seeds ---
    seeds['corrupt_zero_width.bmp'] = create_bmp_bytes(0, 2, 32, payload=b'')
    seeds['corrupt_zero_height.bmp'] = create_bmp_bytes(2, 0, 32, payload=b'')
    seeds['corrupt_negative_width.bmp'] = create_bmp_bytes(-2, 2, 32, payload=b'\x00'*16)
    seeds['corrupt_max_dimension.bmp'] = create_bmp_bytes(70000, 1, 24, payload=b'\x00'*16)
    seeds['corrupt_int32_max_width.bmp'] = create_bmp_bytes(2147483647, 1, 24, payload=b'\x00'*16)
    seeds['corrupt_int32_min_height.bmp'] = create_bmp_bytes(1, -2147483648, 24, payload=b'\x00'*16)

    # --- 4. Offset & Size Inconsistency Seeds ---
    seeds['corrupt_offbits_small.bmp'] = create_bmp_bytes(2, 2, 32, bf_off_bits=30)
    seeds['corrupt_offbits_huge.bmp'] = create_bmp_bytes(2, 2, 32, bf_off_bits=1000000)
    seeds['corrupt_bfsize_too_small.bmp'] = create_bmp_bytes(2, 2, 32, custom_bf_size=50)

    # --- 5. Truncated Seeds ---
    valid_base = seeds['valid_2x2_32bpp.bmp']
    seeds['trunc_0_bytes.bmp'] = b''
    seeds['trunc_13_bytes.bmp'] = valid_base[:13]
    seeds['trunc_14_bytes.bmp'] = valid_base[:14]
    seeds['trunc_30_bytes.bmp'] = valid_base[:30]
    seeds['trunc_53_bytes.bmp'] = valid_base[:53]
    seeds['trunc_54_bytes_header_only.bmp'] = valid_base[:54]
    seeds['trunc_55_bytes.bmp'] = valid_base[:55]
    seeds['trunc_half_payload.bmp'] = valid_base[:62]

    count = 0
    for name, content in seeds.items():
        filepath = os.path.join(output_dir, name)
        with open(filepath, 'wb') as f:
            f.write(content)
        count += 1

    print(f"Generated {count} BMP seeds in '{output_dir}'")


def main():
    parser = argparse.ArgumentParser(description="Generate BMP seed corpus for fuzz testing.")
    parser.add_argument("--output-dir", "-o", default="corpus", help="Target directory for seed files")
    args = parser.parse_args()
    generate_corpus(args.output_dir)


if __name__ == "__main__":
    main()
