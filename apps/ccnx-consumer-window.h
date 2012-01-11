/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Ilya Moiseenko <iliamo@cs.ucla.edu>
 *         Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#ifndef CCNX_CONSUMER_WINDOW_H
#define CCNX_CONSUMER_WINDOW_H

#include "ccnx-consumer.h"
#include "ns3/traced-value.h"

namespace ns3 
{

/**
 * @ingroup ccnx
 * \brief CCNx application for sending out Interest packets (window-based)
 */
class CcnxConsumerWindow: public CcnxConsumer
{
public: 
  static TypeId GetTypeId ();
        
  /**
   * \brief Default constructor 
   */
  CcnxConsumerWindow ();

  // From CcnxApp
  // virtual void
  // OnInterest (const Ptr<const CcnxInterestHeader> &interest);

  virtual void
  OnNack (const Ptr<const CcnxInterestHeader> &interest);

  virtual void
  OnContentObject (const Ptr<const CcnxContentObjectHeader> &contentObject,
                   const Ptr<const Packet> &payload);

  virtual void
  OnTimeout (uint32_t sequenceNumber);
 
protected:
  /**
   * \brief Constructs the Interest packet and sends it using a callback to the underlying CCNx protocol
   */
  virtual void
  ScheduleNextPacket ();
  
private:
  virtual void
  SetWindow (uint32_t window);

  uint32_t
  GetWindow () const;
  
protected:
  TracedValue<uint32_t> m_window;
  uint32_t m_inFlight;
};

} // namespace ns3

#endif