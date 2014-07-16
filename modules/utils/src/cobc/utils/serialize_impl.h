/*
 * Copyright (c) 2013, German Aerospace Center (DLR)
 * All Rights Reserved.
 *
 * See the file "LICENSE" for the full license governing this code.
 */
// ----------------------------------------------------------------------------

#ifndef COBC_UTILS_SERIALIZE_IMPL_H
#define COBC_UTILS_SERIALIZE_IMPL_H

#include "serialize.h"

namespace cobc
{

// ----------------------------------------------------------------------------
// Store
template <>
inline void
Serialize::store<uint8_t>(uint8_t data)
{
    store8(data);
}

template <>
inline void
Serialize::store<uint16_t>(uint16_t data)
{
    store16(data);
}

template <>
inline void
Serialize::store<uint32_t>(uint32_t data)
{
    store32(data);
}

template <>
inline void
Serialize::store<uint64_t>(uint64_t data)
{
    store64(data);
}

template <>
inline void
Serialize::store<float>(float data)
{
    storeFloat(data);
}

template <>
inline void
Serialize::store<double>(double data)
{
    storeDouble(data);
}

// ----------------------------------------------------------------------------
// Peek
template <>
inline uint8_t
Deserialize::peek<uint8_t>(size_t n) const
{
    uint8_t value = peek8(n);
    return value;
}

template <>
inline uint16_t
Deserialize::peek<uint16_t>(size_t n) const
{
    uint16_t value = peek16(n);
    return value;
}

template <>
inline uint32_t
Deserialize::peek<uint32_t>(size_t n) const
{
    uint32_t value = peek32(n);
    return value;
}

template <>
inline uint64_t
Deserialize::peek<uint64_t>(size_t n) const
{
    uint64_t value = peek64(n);
    return value;
}

template <>
inline float
Deserialize::peek<float>(size_t n) const
{
    float value = peekFloat(n);
    return value;
}

template <>
inline double
Deserialize::peek<double>(size_t n) const
{
    double value = peekDouble(n);
    return value;
}

// ----------------------------------------------------------------------------
// Read
template <>
inline uint8_t
Deserialize::read<uint8_t>()
{
    uint8_t value = read8();
    return value;
}

template <>
inline uint16_t
Deserialize::read<uint16_t>()
{
    uint16_t value = read16();
    return value;
}

template <>
inline uint32_t
Deserialize::read<uint32_t>()
{
    uint32_t value = read32();
    return value;
}

template <>
inline uint64_t
Deserialize::read<uint64_t>()
{
    uint64_t value = read64();
    return value;
}

template <>
inline float
Deserialize::read<float>()
{
    float value = readFloat();
    return value;
}


template <>
inline double
Deserialize::read<double>()
{
    double value = readDouble();
    return value;
}

}

#endif // COBC_UTILS_SERIALIZE_IMPL_H