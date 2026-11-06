// Generated from RFC 7541 Appendix B (HPACK Huffman Code). DO NOT EDIT.
#pragma once
#include <cstdint>

namespace Nodes::GrpcHpack {

struct HuffmanCodeEntry {
    uint32_t code;
    uint8_t bitLength;
};

// Index 0..255 = byte symbol, 256 = EOS.
inline constexpr HuffmanCodeEntry kHuffmanCodes[257] = {
    {0x1FF8u, 13u},
    {0x7FFFD8u, 23u},
    {0xFFFFFE2u, 28u},
    {0xFFFFFE3u, 28u},
    {0xFFFFFE4u, 28u},
    {0xFFFFFE5u, 28u},
    {0xFFFFFE6u, 28u},
    {0xFFFFFE7u, 28u},
    {0xFFFFFE8u, 28u},
    {0xFFFFEAu, 24u},
    {0x3FFFFFFCu, 30u},
    {0xFFFFFE9u, 28u},
    {0xFFFFFEAu, 28u},
    {0x3FFFFFFDu, 30u},
    {0xFFFFFEBu, 28u},
    {0xFFFFFECu, 28u},
    {0xFFFFFEDu, 28u},
    {0xFFFFFEEu, 28u},
    {0xFFFFFEFu, 28u},
    {0xFFFFFF0u, 28u},
    {0xFFFFFF1u, 28u},
    {0xFFFFFF2u, 28u},
    {0x3FFFFFFEu, 30u},
    {0xFFFFFF3u, 28u},
    {0xFFFFFF4u, 28u},
    {0xFFFFFF5u, 28u},
    {0xFFFFFF6u, 28u},
    {0xFFFFFF7u, 28u},
    {0xFFFFFF8u, 28u},
    {0xFFFFFF9u, 28u},
    {0xFFFFFFAu, 28u},
    {0xFFFFFFBu, 28u},
    {0x14u, 6u}, // ' '
    {0x3F8u, 10u}, // '!'
    {0x3F9u, 10u}, // '"'
    {0xFFAu, 12u}, // '#'
    {0x1FF9u, 13u}, // '$'
    {0x15u, 6u}, // '%'
    {0xF8u, 8u}, // '&'
    {0x7FAu, 11u}, // '''
    {0x3FAu, 10u}, // '('
    {0x3FBu, 10u}, // ')'
    {0xF9u, 8u}, // '*'
    {0x7FBu, 11u}, // '+'
    {0xFAu, 8u}, // ','
    {0x16u, 6u}, // '-'
    {0x17u, 6u}, // '.'
    {0x18u, 6u}, // '/'
    {0x0u, 5u}, // '0'
    {0x1u, 5u}, // '1'
    {0x2u, 5u}, // '2'
    {0x19u, 6u}, // '3'
    {0x1Au, 6u}, // '4'
    {0x1Bu, 6u}, // '5'
    {0x1Cu, 6u}, // '6'
    {0x1Du, 6u}, // '7'
    {0x1Eu, 6u}, // '8'
    {0x1Fu, 6u}, // '9'
    {0x5Cu, 7u}, // ':'
    {0xFBu, 8u}, // ';'
    {0x7FFCu, 15u}, // '<'
    {0x20u, 6u}, // '='
    {0xFFBu, 12u}, // '>'
    {0x3FCu, 10u}, // '?'
    {0x1FFAu, 13u}, // '@'
    {0x21u, 6u}, // 'A'
    {0x5Du, 7u}, // 'B'
    {0x5Eu, 7u}, // 'C'
    {0x5Fu, 7u}, // 'D'
    {0x60u, 7u}, // 'E'
    {0x61u, 7u}, // 'F'
    {0x62u, 7u}, // 'G'
    {0x63u, 7u}, // 'H'
    {0x64u, 7u}, // 'I'
    {0x65u, 7u}, // 'J'
    {0x66u, 7u}, // 'K'
    {0x67u, 7u}, // 'L'
    {0x68u, 7u}, // 'M'
    {0x69u, 7u}, // 'N'
    {0x6Au, 7u}, // 'O'
    {0x6Bu, 7u}, // 'P'
    {0x6Cu, 7u}, // 'Q'
    {0x6Du, 7u}, // 'R'
    {0x6Eu, 7u}, // 'S'
    {0x6Fu, 7u}, // 'T'
    {0x70u, 7u}, // 'U'
    {0x71u, 7u}, // 'V'
    {0x72u, 7u}, // 'W'
    {0xFCu, 8u}, // 'X'
    {0x73u, 7u}, // 'Y'
    {0xFDu, 8u}, // 'Z'
    {0x1FFBu, 13u}, // '['
    {0x7FFF0u, 19u}, // '\\'
    {0x1FFCu, 13u}, // ']'
    {0x3FFCu, 14u}, // '^'
    {0x22u, 6u}, // '_'
    {0x7FFDu, 15u}, // '`'
    {0x3u, 5u}, // 'a'
    {0x23u, 6u}, // 'b'
    {0x4u, 5u}, // 'c'
    {0x24u, 6u}, // 'd'
    {0x5u, 5u}, // 'e'
    {0x25u, 6u}, // 'f'
    {0x26u, 6u}, // 'g'
    {0x27u, 6u}, // 'h'
    {0x6u, 5u}, // 'i'
    {0x74u, 7u}, // 'j'
    {0x75u, 7u}, // 'k'
    {0x28u, 6u}, // 'l'
    {0x29u, 6u}, // 'm'
    {0x2Au, 6u}, // 'n'
    {0x7u, 5u}, // 'o'
    {0x2Bu, 6u}, // 'p'
    {0x76u, 7u}, // 'q'
    {0x2Cu, 6u}, // 'r'
    {0x8u, 5u}, // 's'
    {0x9u, 5u}, // 't'
    {0x2Du, 6u}, // 'u'
    {0x77u, 7u}, // 'v'
    {0x78u, 7u}, // 'w'
    {0x79u, 7u}, // 'x'
    {0x7Au, 7u}, // 'y'
    {0x7Bu, 7u}, // 'z'
    {0x7FFEu, 15u}, // '{'
    {0x7FCu, 11u}, // '|'
    {0x3FFDu, 14u}, // '}'
    {0x1FFDu, 13u}, // '~'
    {0xFFFFFFCu, 28u},
    {0xFFFE6u, 20u},
    {0x3FFFD2u, 22u},
    {0xFFFE7u, 20u},
    {0xFFFE8u, 20u},
    {0x3FFFD3u, 22u},
    {0x3FFFD4u, 22u},
    {0x3FFFD5u, 22u},
    {0x7FFFD9u, 23u},
    {0x3FFFD6u, 22u},
    {0x7FFFDAu, 23u},
    {0x7FFFDBu, 23u},
    {0x7FFFDCu, 23u},
    {0x7FFFDDu, 23u},
    {0x7FFFDEu, 23u},
    {0xFFFFEBu, 24u},
    {0x7FFFDFu, 23u},
    {0xFFFFECu, 24u},
    {0xFFFFEDu, 24u},
    {0x3FFFD7u, 22u},
    {0x7FFFE0u, 23u},
    {0xFFFFEEu, 24u},
    {0x7FFFE1u, 23u},
    {0x7FFFE2u, 23u},
    {0x7FFFE3u, 23u},
    {0x7FFFE4u, 23u},
    {0x1FFFDCu, 21u},
    {0x3FFFD8u, 22u},
    {0x7FFFE5u, 23u},
    {0x3FFFD9u, 22u},
    {0x7FFFE6u, 23u},
    {0x7FFFE7u, 23u},
    {0xFFFFEFu, 24u},
    {0x3FFFDAu, 22u},
    {0x1FFFDDu, 21u},
    {0xFFFE9u, 20u},
    {0x3FFFDBu, 22u},
    {0x3FFFDCu, 22u},
    {0x7FFFE8u, 23u},
    {0x7FFFE9u, 23u},
    {0x1FFFDEu, 21u},
    {0x7FFFEAu, 23u},
    {0x3FFFDDu, 22u},
    {0x3FFFDEu, 22u},
    {0xFFFFF0u, 24u},
    {0x1FFFDFu, 21u},
    {0x3FFFDFu, 22u},
    {0x7FFFEBu, 23u},
    {0x7FFFECu, 23u},
    {0x1FFFE0u, 21u},
    {0x1FFFE1u, 21u},
    {0x3FFFE0u, 22u},
    {0x1FFFE2u, 21u},
    {0x7FFFEDu, 23u},
    {0x3FFFE1u, 22u},
    {0x7FFFEEu, 23u},
    {0x7FFFEFu, 23u},
    {0xFFFEAu, 20u},
    {0x3FFFE2u, 22u},
    {0x3FFFE3u, 22u},
    {0x3FFFE4u, 22u},
    {0x7FFFF0u, 23u},
    {0x3FFFE5u, 22u},
    {0x3FFFE6u, 22u},
    {0x7FFFF1u, 23u},
    {0x3FFFFE0u, 26u},
    {0x3FFFFE1u, 26u},
    {0xFFFEBu, 20u},
    {0x7FFF1u, 19u},
    {0x3FFFE7u, 22u},
    {0x7FFFF2u, 23u},
    {0x3FFFE8u, 22u},
    {0x1FFFFECu, 25u},
    {0x3FFFFE2u, 26u},
    {0x3FFFFE3u, 26u},
    {0x3FFFFE4u, 26u},
    {0x7FFFFDEu, 27u},
    {0x7FFFFDFu, 27u},
    {0x3FFFFE5u, 26u},
    {0xFFFFF1u, 24u},
    {0x1FFFFEDu, 25u},
    {0x7FFF2u, 19u},
    {0x1FFFE3u, 21u},
    {0x3FFFFE6u, 26u},
    {0x7FFFFE0u, 27u},
    {0x7FFFFE1u, 27u},
    {0x3FFFFE7u, 26u},
    {0x7FFFFE2u, 27u},
    {0xFFFFF2u, 24u},
    {0x1FFFE4u, 21u},
    {0x1FFFE5u, 21u},
    {0x3FFFFE8u, 26u},
    {0x3FFFFE9u, 26u},
    {0xFFFFFFDu, 28u},
    {0x7FFFFE3u, 27u},
    {0x7FFFFE4u, 27u},
    {0x7FFFFE5u, 27u},
    {0xFFFECu, 20u},
    {0xFFFFF3u, 24u},
    {0xFFFEDu, 20u},
    {0x1FFFE6u, 21u},
    {0x3FFFE9u, 22u},
    {0x1FFFE7u, 21u},
    {0x1FFFE8u, 21u},
    {0x7FFFF3u, 23u},
    {0x3FFFEAu, 22u},
    {0x3FFFEBu, 22u},
    {0x1FFFFEEu, 25u},
    {0x1FFFFEFu, 25u},
    {0xFFFFF4u, 24u},
    {0xFFFFF5u, 24u},
    {0x3FFFFEAu, 26u},
    {0x7FFFF4u, 23u},
    {0x3FFFFEBu, 26u},
    {0x7FFFFE6u, 27u},
    {0x3FFFFECu, 26u},
    {0x3FFFFEDu, 26u},
    {0x7FFFFE7u, 27u},
    {0x7FFFFE8u, 27u},
    {0x7FFFFE9u, 27u},
    {0x7FFFFEAu, 27u},
    {0x7FFFFEBu, 27u},
    {0xFFFFFFEu, 28u},
    {0x7FFFFECu, 27u},
    {0x7FFFFEDu, 27u},
    {0x7FFFFEEu, 27u},
    {0x7FFFFEFu, 27u},
    {0x7FFFFF0u, 27u},
    {0x3FFFFEEu, 26u},
    {0x3FFFFFFFu, 30u},
};

} // namespace Nodes::GrpcHpack
