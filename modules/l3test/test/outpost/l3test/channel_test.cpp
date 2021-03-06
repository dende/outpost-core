/*
 * Copyright (c) 2014-2017, German Aerospace Center (DLR)
 *
 * This file is part of the development version of OUTPOST.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Authors:
 * - 2014-2017, Fabian Greif (DLR RY-AVS)
 */

#include <l3test/script/channel.h>

#include <unittest/harness.h>

using namespace l3test::script;

/*
 * Check the default values of the object.
 */
TEST(ChannelTest, channelHasNoPacketAfterCreation)
{
    Channel channel;

    EXPECT_FALSE(channel.hasPacket());
}

TEST(ChannelTest, channelHasPacketAfterPushingData)
{
    Channel channel;

    uint8_t data[16];
    channel.append(data, 16);
    EXPECT_FALSE(channel.hasPacket());

    channel.finishPacket();
    EXPECT_TRUE(channel.hasPacket());
}

TEST(ChannelTest, pushedDataAreReceivedCorrectly)
{
    Channel channel;

    uint8_t data[16];
    for (uint_fast8_t i = 0; i < 16; ++i)
    {
        data[i] = 16 - i;
    }

    channel.append(data, 16);
    channel.finishPacket();

    ASSERT_TRUE(channel.hasPacket());
    EXPECT_EQ(16U, channel.getPacketLength());

    std::vector<uint8_t> recv = channel.getPacket();

    EXPECT_EQ(16U, recv.size());
    EXPECT_ARRAY_EQ(uint8_t, data, recv.data(), 16);

    // remove the packet form the list
    channel.nextPacket();
    EXPECT_FALSE(channel.hasPacket());
}

/*
 * Send and receive multiple packets.
 *
 * Check that the order of packets is correct.
 */
TEST(ChannelTest, receiveMultiplePacketsAndValidateData)
{
    Channel channel;
    uint8_t data[16];

    for (uint_fast8_t i = 0; i < 16; ++i)
    {
        data[i] = 16 - i;
    }

    channel.append(data, 16);
    channel.finishPacket();

    uint8_t data2[8];
    for (uint_fast8_t i = 0; i < 8; ++i)
    {
        data2[i] = i;
    }

    channel.append(data2, 8);
    channel.finishPacket();

    ASSERT_TRUE(channel.hasPacket());
    EXPECT_EQ(16U, channel.getPacketLength());

    std::vector<uint8_t> recv = channel.getPacket();

    EXPECT_EQ(16U, recv.size());
    EXPECT_ARRAY_EQ(uint8_t, data, recv.data(), 16);

    // remove the packet form the list
    channel.nextPacket();

    ASSERT_TRUE(channel.hasPacket());
    EXPECT_EQ(8U, channel.getPacketLength());

    std::vector<uint8_t> recv2 = channel.getPacket();

    EXPECT_EQ(8U, recv2.size());
    EXPECT_ARRAY_EQ(uint8_t, data2, recv2.data(), 8);

    // remove the packet form the list
    channel.nextPacket();

    EXPECT_FALSE(channel.hasPacket());
}
