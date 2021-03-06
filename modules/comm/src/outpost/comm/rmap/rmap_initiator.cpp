/*
 * Copyright (c) 2017, German Aerospace Center (DLR)
 *
 * This file is part of the development version of OUTPOST.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Authors:
 * - 2017, Muhammad Bassam (DLR RY-AVS)
 * - 2017, Fabian Greif (DLR RY-AVS)
 */

#include "rmap_initiator.h"

#include "rmap_common.h"

using namespace outpost::comm;

outpost::smpc::Topic<outpost::comm::NonRmapDataType> outpost::comm::nonRmapPacketReceived;

constexpr outpost::time::Duration RmapInitiator::receiveTimeout;

//-----------------------------------------------------------------------------
RmapInitiator::RmapInitiator(hal::SpaceWire& spw,
                             RmapTargetsList* list,
                             uint8_t priority,
                             size_t stackSize,
                             outpost::support::parameter::HeartbeatSource heartbeatSource) :
    outpost::rtos::Thread(priority, stackSize, "RMEN"),
    mSpW(spw),
    mTargetNodes(list),
    mOperationLock(),
    mInitiatorLogicalAddress(rmap::defaultLogicalAddress),
    mIncrementMode(false),
    mVerifyMode(false),
    mReplyMode(false),
    mStopped(true),
    mTransactionId(0),
    mTransactionsList(),
    mDiscardedPacket(nullptr),
    mCounters(),
    mRxData(),
    mHeartbeatSource(heartbeatSource)
{
}

RmapInitiator::~RmapInitiator()
{
}

bool
RmapInitiator::write(const char* targetNodeName,
                     uint32_t memoryAddress,
                     outpost::Slice<const uint8_t> const& data,
                     outpost::time::Duration timeout)
{
    bool result = false;
    if (mTargetNodes)
    {
        RmapTargetNode* targetNode = mTargetNodes->getTargetNode(targetNodeName);
        if (targetNode)
        {
            // Exit if trying to send zero length
            if (data.getNumberOfElements() != 0)
            {
                result = write(*targetNode, memoryAddress, data, timeout);
            }
        }
    }
    return result;
}

bool
RmapInitiator::write(RmapTargetNode& rmapTargetNode,
                     uint32_t memoryAddress,
                     outpost::Slice<const uint8_t> const& data,
                     outpost::time::Duration timeout)
{
    if (data.getNumberOfElements() == 0)
    {
        // the second write function also return without a message in this case
        return false;
    }

    // Guard operation against concurrent accesses
    outpost::rtos::MutexGuard lock(mOperationLock);
    bool result = false;

    // Using existing free element from the transaction list
    RmapTransaction* transaction = mTransactionsList.getFreeTransaction();
    RmapPacket* cmd = transaction->getCommandPacket();

    // Packet configuration
    cmd->setInitiatorLogicalAddress(mInitiatorLogicalAddress);
    cmd->setWrite();
    cmd->setCommand();

    if (mIncrementMode)
    {
        cmd->setIncrementFlag();
    }
    else
    {
        cmd->unsetIncrementFlag();
    }

    if (mVerifyMode)
    {
        cmd->setVerifyFlag();
    }
    else
    {
        cmd->unsetVerifyFlag();
    }

    if (mReplyMode)
    {
        // Sets the transaction mode
        transaction->setBlockingMode(true);

        // Extra block call with zero timeout for acquiring already released lock
        transaction->blockTransaction(outpost::time::Duration::zero());

        cmd->setReplyFlag();
    }
    else
    {
        // UnSets the transaction mode
        transaction->setBlockingMode(false);

        cmd->unsetReplyFlag();
    }
    cmd->setExtendedAddress(0x00);
    cmd->setAddress(memoryAddress);
    cmd->setDataLength(data.getNumberOfElements());
    cmd->setTargetInformation(rmapTargetNode);
    transaction->setTimeoutDuration(timeout);

    // Transaction will be initiated and sent through the SpW interface
    if (sendPacket(transaction, data))
    {
        // If reply is expected
        if (mReplyMode)
        {
            transaction->setState(RmapTransaction::commandSent);

            // Wait for the RMAP reply
            transaction->blockTransaction(timeout);

            // Command sent but no reply
            if (transaction->getState() == RmapTransaction::commandSent)
            {
                console_out("RMAP-Initiator: command sent but no reply received for the "
                            "transaction %u\n",
                            transaction->getTransactionID());

                result = false;
            }
            // Command sent and reply received
            else if (transaction->getState() == RmapTransaction::replyReceived)
            {
                RmapPacket* rply = transaction->getReplyPacket();

                if (rply->getStatus() == RmapReplyStatus::commandExecutedSuccessfully)
                {
                    console_out("RMAP-Initiator: reply received with success\n");

                    result = true;
                }
                else
                {
                    console_out("RMAP-Initiator: reply received with failure\n");

                    RmapReplyStatus::replyStatus(
                            static_cast<RmapReplyStatus::ErrorStatusCodes>(rply->getStatus()));

                    result = false;
                }
            }
        }
        else
        {
            if (transaction->getState() == RmapTransaction::initiated)
            {
                // Command was sent successfully
                result = true;
            }
            else
            {
                // Command was not sent successfully
                console_out("RMAP-Initiator: transaction could not be initiated\n");
                result = false;
            }
        }
    }

    // Delete the transaction from the list
    mTransactionsList.removeTransaction(transaction->getTransactionID());
    transaction = nullptr;
    cmd = nullptr;

    return result;
}

bool
RmapInitiator::read(const char* targetNodeName,
                    uint32_t memoryAddress,
                    outpost::Slice<uint8_t> const& buffer,
                    outpost::time::Duration timeout)
{
    bool result = false;
    if (mTargetNodes)
    {
        RmapTargetNode* targetNode = mTargetNodes->getTargetNode(targetNodeName);
        if (targetNode)
        {
            // Exit if trying to read zero length
            if (buffer.getNumberOfElements() != 0)
            {
                result = read(*targetNode, memoryAddress, buffer, timeout);
            }
        }
    }
    return result;
}

bool
RmapInitiator::read(RmapTargetNode& rmapTargetNode,
                    uint32_t memoryAddress,
                    outpost::Slice<uint8_t> const& buffer,
                    outpost::time::Duration timeout)
{
    if (buffer.getNumberOfElements() > Buffer::bufferSize)
    {
        console_out("RMAP-Initiator: Requested size for read %u, maximal allowed size %u\n",
                    length,
                    Buffer::bufferSize);
        return false;
    }

    if (buffer.getNumberOfElements() == 0)
    {
        // the second read function also return without a message in this case
        return false;
    }

    // Guard operation against concurrent accesses
    outpost::rtos::MutexGuard lock(mOperationLock);
    bool result = false;

    RmapTransaction* transaction = mTransactionsList.getFreeTransaction();

    if (!transaction)
    {
        console_out("RMAP-Initiator: All transactions are in use\n");
        return false;
    }

    RmapPacket* cmd = transaction->getCommandPacket();

    // Read transaction will always be blocking
    transaction->setBlockingMode(true);

    // Extra block call with zero timeout for acquiring already released lock
    transaction->blockTransaction(outpost::time::Duration::zero());

    // Sets the command packet
    cmd->setInitiatorLogicalAddress(mInitiatorLogicalAddress);
    cmd->setRead();
    cmd->setCommand();

    if (mIncrementMode)
    {
        cmd->setIncrementFlag();
    }
    else
    {
        cmd->unsetIncrementFlag();
    }
    if (mVerifyMode)
    {
        cmd->setVerifyFlag();
    }
    else
    {
        cmd->unsetVerifyFlag();
    }

    cmd->setReplyFlag();
    cmd->setExtendedAddress(0x00);
    cmd->setAddress(memoryAddress);
    cmd->setDataLength(buffer.getNumberOfElements());

    // InitiatorLogicalAddress might be updated in below
    cmd->setTargetInformation(rmapTargetNode);
    transaction->setInitiatorLogicalAddress(cmd->getInitiatorLogicalAddress());
    transaction->setTimeoutDuration(timeout);
    outpost::Slice<const uint8_t> empty{outpost::Slice<const uint8_t>::empty()};

    // Command is read, thus no data bytes available
    if (sendPacket(transaction, empty))
    {
        transaction->setState(RmapTransaction::commandSent);

        console_out("RMAP-Initiator: Command sent %u, waiting for reply\n",
                    transaction->getState());

        // Wait for the RMAP reply
        transaction->blockTransaction(timeout);

        console_out("RMAP-Initiator: Notified with state: %u\n", transaction->getState());

        if (transaction->getState() == RmapTransaction::replyReceived)
        {
            RmapPacket* rply = transaction->getReplyPacket();
            uint8_t replyStatus = rply->getStatus();

            if (replyStatus != RmapReplyStatus::commandExecutedSuccessfully)
            {
                console_out("RMAP-Initiator: Command not executed successfully: %u\n", replyStatus);
                result = false;
            }
            else
            {
                if (buffer.getNumberOfElements() < rply->getDataLength())
                {
                    console_out("RMAP-Initiator: Read reply with more data then requested\n");
                    result = false;
                }
                else if (buffer.getNumberOfElements() > rply->getDataLength())
                {
                    console_out("RMAP-Initiator: Read reply with insufficient data\n");
                    result = false;
                }
                else
                {
                    // Copy received data to the external buffer
                    mRxData.getData(buffer.begin());

                    // Release the SpW buffer

                    result = true;
                }
            }
        }
        else
        {
            console_out("RMAP-Initiator: Timeout\n");
            result = false;
        }
    }
    else
    {
        console_out("RMAP-Initiator: Transaction could not be initiated\n");
        result = false;
    }

    // Delete the transaction from the list
    mTransactionsList.removeTransaction(transaction->getTransactionID());
    transaction = nullptr;
    cmd = nullptr;

    return result;
}

//=============================================================================

void
RmapInitiator::run()
{
    static RmapPacket packet;

    mStopped = false;
    while (!mStopped)
    {
        outpost::support::Heartbeat::send(mHeartbeatSource, receiveTimeout * 2);
        if (receivePacket(&packet))
        {
            // Only handling reply packet, no command packets
            if (packet.isReplyPacket())
            {
                replyPacketReceived(&packet);
            }
            else
            {
                mCounters.mErrorneousReplyPackets++;
            }
        }
    }
    outpost::support::Heartbeat::suspend(mHeartbeatSource);
    mStopped = true;
}

bool
RmapInitiator::sendPacket(RmapTransaction* transaction, outpost::Slice<const uint8_t> data)
{
    RmapPacket* cmd = transaction->getCommandPacket();
    hal::SpaceWire::TransmitBuffer* txBuffer = 0;
    uint16_t transactionID;
    bool result = false;

    transactionID = getNextAvailableTransactionID();

    transaction->setTransactionID(transactionID);
    cmd->setTransactionID(transactionID);

    // It is assumed that the transaction is already in the list and in case of
    // required reply corresponding transaction will found and freed accordingly
    // therefore transmit can directly begin

    // Request TX buffer
    if (mSpW.requestBuffer(txBuffer, transaction->getTimeoutDuration())
        == hal::SpaceWire::Result::success)
    {
        // Serialize the packet content to the SpW buffer
        if (cmd->constructPacket(txBuffer->getData(), data))
        {
            // Total packet length varies with command type
            if (cmd->isWrite())
            {
                // Header length + Data length + Header CRC + Data CRC
                txBuffer->setLength(cmd->getHeaderLength() + cmd->getDataLength() + 2);
            }
            else if (cmd->isRead())
            {
                // Header length Header CRC
                txBuffer->setLength(cmd->getHeaderLength() + 1);
            }

            txBuffer->setEndMarker(outpost::hal::SpaceWire::eop);

#ifdef DEBUG_EN
            outpost::Slice<uint8_t> txData = txBuffer->getData();
            console_out("TX-Data length: %zu\n", txData.getNumberOfElements());
            for (uint16_t i = 0; i < txData.getNumberOfElements(); i++)
            {
                console_out("%02X ", txData[i]);
                if (i % 30 == 29)
                {
                    console_out("\n");
                }
            }
            console_out("\n");
#endif

            if (mSpW.send(txBuffer, transaction->getTimeoutDuration())
                == hal::SpaceWire::Result::success)
            {
                transaction->setState(RmapTransaction::initiated);

                result = true;
            }
        }
    }
    return result;
}

bool
RmapInitiator::receivePacket(RmapPacket* rxedPacket)
{
    hal::SpaceWire::ReceiveBuffer rxBuffer;
    bool result = false;

    // Receive response
    if (mSpW.receive(rxBuffer, receiveTimeout) == hal::SpaceWire::Result::success)
    {
        if (rxBuffer.getEndMarker() == hal::SpaceWire::eop)
        {
            outpost::Slice<const uint8_t> rxData = rxBuffer.getData();

#ifdef DEBUG_EN
            console_out("RX-Data length: %zu\n", rxData.getNumberOfElements());
            for (uint16_t i = 0; i < rxData.getNumberOfElements(); i++)
            {
                console_out("%02X ", rxData[i]);
                if (i % 30 == 29)
                {
                    console_out("\n");
                }
            }
            console_out("\n");
#endif

            rxedPacket->reset();

            if (rxedPacket->extractPacket(rxData, mInitiatorLogicalAddress))
            {
                if (mRxData.addData(rxedPacket->getData(), rxedPacket->getDataLength()))
                {
                    result = true;
                }
                else
                {
                    mCounters.mErrorInStoringReplyPacket++;
                }
                mSpW.releaseBuffer(rxBuffer);
            }
            else
            {
                console_out("RMAP-Initiator: packet interpretation failed, could be "
                            "non-rmap"
                            "packet\n");
                nonRmapPacketReceived.publish(rxData);
                mCounters.mNonRmapPacketReceived++;
                mSpW.releaseBuffer(rxBuffer);
                result = false;
            }
        }
        else
        {
            console_out("RMAP-Initiator: Wrong end of packet\n");
            mSpW.releaseBuffer(rxBuffer);
            result = false;
        }
    }
    return result;
}

void
RmapInitiator::replyPacketReceived(RmapPacket* packet)
{
    // Find a corresponding command packet
    RmapTransaction* transaction = resolveTransaction(packet);

    if (!transaction)
    {
        // If not found, increment error counter
        mCounters.mDiscardedReceivedPackets++;
        mDiscardedPacket = packet;
        console_out("RMAP Reply packet (dataLength %lu bytes) was received but "
                    "no corresponding transaction was found.\n",
                    packet->getDataLength());
    }
    else
    {
        // Register reply packet to the resolved transaction
        transaction->setReplyPacket(packet);

        // Update transaction state
        transaction->setState(RmapTransaction::replyReceived);

        console_out("RMAP-Initiator: Reply received, thus notifying blocking thread\n");

        if (transaction->isBlockingMode())
        {
            console_out("RMAP-Initiator: Transaction with TID %u is blocking, thus "
                        "releasing lock...\n",
                        transaction->getTransactionID());
            transaction->releaseTransaction();
        }
    }
}

RmapTransaction*
RmapInitiator::resolveTransaction(RmapPacket* packet)
{
    uint16_t transactionID = packet->getTransactionID();
    RmapTransaction* transaction = mTransactionsList.getTransaction(transactionID);

    if (!transaction)
    {
        // TID is not in use
        console_out("RMAP-Initiator: Unexpected RMAP Reply Packet Was Received\n");
        return NULL;
    }
    return transaction;
}

uint16_t
RmapInitiator::getNextAvailableTransactionID()
{
    // Check in the current transaction list that the TID is not in use
    for (uint32_t i = 0; i < rmap::maxTransactionIds; i++)
    {
        if (!mTransactionsList.isTransactionIdUsed(i))
        {
            mTransactionId = i;
            break;
        }
    }
    return mTransactionId;
}
