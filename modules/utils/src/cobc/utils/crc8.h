/*
 * Copyright (c) 2013, German Aerospace Center (DLR)
 *
 * This file is part of libCOBC 0.4.
 *
 * It is distributed under the terms of the GNU General Public License with a
 * linking exception. See the file "LICENSE" for the full license governing
 * this code.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */
// ----------------------------------------------------------------------------

#ifndef COBC_CRC8_H
#define COBC_CRC8_H

#include <stdint.h>
#include <stddef.h>

#include "bounded_array.h"

namespace cobc
{
/**
 * CRC-8 calculation for RMAP.
 *
 * Polynomial    : x^8 + x^2 + x + 1
 * Initial value : 0x00
 *
 * \see     ECSS-E-50-11 SpaceWire RMAP Protocol
 * \author  Fabian Greif
 */
class Crc8
{
public:
    inline
    Crc8() :
        mCrc(initialValue)
    {
    }

    inline
    ~Crc8()
    {
    }

    /**
     * Calculate CRC from a block of data.
     *
     * \param data
     *     address
     * \param length
     *     length
     *
     * \retval crc
     *     calculated checksum
     */
    static uint8_t
    calculate(cobc::BoundedArray<const uint8_t> data);

    /**
     * Reset CRC calculation
     */
    inline void
    reset()
    {
        mCrc = initialValue;
    }

    /**
     * CRC update.
     *
     * \param data
     *     byte
     */
    void
    update(uint8_t data);

    /**
     * Get result of CRC calculation.
     */
    inline uint8_t
    getValue() const
    {
        return mCrc;
    }

private:
    // disable copy constructor
    Crc8(const Crc8&);

    // disable copy-assignment operator
    Crc8&
    operator=(const Crc8&);

    static const uint8_t initialValue = 0x00;
    static const int numberOfValuesPerByte = 256;

    /// Pre-calculated CRC table for one byte
    static const uint8_t crcTable[numberOfValuesPerByte];

    uint8_t mCrc;
};
}

#endif