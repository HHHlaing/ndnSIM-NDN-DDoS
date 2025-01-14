// cons-app.cpp

#include "cons-app.hpp"

#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"

#include <ndn-cxx/lp/tags.hpp>
#include "ns3/random-variable-stream.h"

NS_LOG_COMPONENT_DEFINE("ConsApp");

namespace ns3 {
namespace ndn{

NS_OBJECT_ENSURE_REGISTERED(ConsApp);

// register NS-3 type
TypeId
ConsApp::GetTypeId()
{
  static TypeId tid = TypeId("ConsApp")
    .SetParent<ndn::App>()
    .AddConstructor<ConsApp>()
    .AddAttribute("Frequency", "Frequency of interest packets", StringValue("1"),
                  MakeIntegerAccessor(&ConsApp::m_frequency), MakeIntegerChecker<int32_t>())

    .AddAttribute("Randomize",
                  "Type of send time randomization: none (default), uniform, exponential",
                  StringValue("none"),
                  MakeStringAccessor(&ConsApp::SetRandomize, &ConsApp::GetRandomize),
                  MakeStringChecker())

    .AddAttribute("Names",
                  "Name of interests separated by comma",
                  StringValue("/"),
                  MakeStringAccessor(&ConsApp::m_interestNames), MakeStringChecker())

    .AddAttribute("ValidInterest",
                  "Auto append sequence number: true (default), false",
                  BooleanValue(true),
                  MakeBooleanAccessor(&ConsApp::m_validInterest), MakeBooleanChecker())

    .AddAttribute("GoodConsumer",
                  "Consumer is good: true (default), false (attacker)",
                  BooleanValue(true),
                  MakeBooleanAccessor(&ConsApp::m_isGood), MakeBooleanChecker())

    .AddAttribute("InitSeq",
                  "The first seq to send",
                  IntegerValue(-1),
                  MakeIntegerAccessor(&ConsApp::m_initSeq), MakeIntegerChecker<int32_t>())

    .AddAttribute("MaxSeq",
                  "The max seq to send",
                  StringValue("200"),
                  MakeIntegerAccessor(&ConsApp::m_maxSeq), MakeIntegerChecker<int32_t>())

    .AddAttribute("GoodTrafficAlso",
                  "GoodTrafficAlso",
                  IntegerValue(-1),
                  MakeIntegerAccessor(&ConsApp::m_goodTrafficAlso), MakeIntegerChecker<int32_t>())

    .AddAttribute("PlayByRule",
                  "PlayByRule",
                  BooleanValue(false),
                  MakeBooleanAccessor(&ConsApp::m_play_by_rule), MakeBooleanChecker());

  return tid;
}

ConsApp::ConsApp()
  : m_frequency(1.0)
  , m_originFreq(1.0)
  , m_firstTime(true)
  , m_isGood(true)
  , m_interestNames("/")
  , m_goodTrafficAlso(-1)
  , m_goodTrafficSeq(1)
{
}

ConsApp::~ConsApp()
{
}


void
ConsApp::ScheduleNextPacket()
{
  // double mean = 8.0 * m_payloadSize / m_desiredRate.GetBitRate ();
  // std::cout << "next: " << Simulator::Now().ToDouble(Time::S) + mean << "s\n";

  // NS_LOG_INFO("Current Frequency: " << m_frequency);

  if (m_firstTime) {
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &ConsApp::SendInterest, this);
    m_firstTime = false;
  }
  else if (!m_sendEvent.IsRunning()) {
    if (m_frequency == 0) {
      m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / 1)
                                        : Seconds(m_random->GetValue()),
                                        &ConsApp::SendInterest, this);
    }
    else {
      m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
                                        : Seconds(m_random->GetValue()),
                                        &ConsApp::SendInterest, this);
    }
  }
}

void
ConsApp::ScheduleNextGoodPacket()
{
  m_sendGoodEvent = Simulator::Schedule(Seconds(1.0 / m_goodTrafficAlso),
                                        &ConsApp::SendGoodInterest, this);
}

void
ConsApp::SetRandomize(const std::string& value)
{
  if (value == "uniform") {
    m_random = CreateObject<UniformRandomVariable>();
    m_random->SetAttribute("Min", DoubleValue(0.0));
    m_random->SetAttribute("Max", DoubleValue(2 * 1.0 / m_frequency));
  }
  else if (value == "exponential") {
    m_random = CreateObject<ExponentialRandomVariable>();
    m_random->SetAttribute("Mean", DoubleValue(1.0 / m_frequency));
    m_random->SetAttribute("Bound", DoubleValue(50 * 1.0 / m_frequency));
  }
  else
    m_random = 0;

  m_randomType = value;
}

std::string
ConsApp::GetRandomize() const
{
  return m_randomType;
}

// Processing upon start of the application
void
ConsApp::StartApplication()
{
  // initialize ndn::App
  ndn::App::StartApplication();
  // std::cout << "Interest Name is " << m_interestName << std::endl;

  size_t pos = 0;
  std::string delimeter = ",";
  std::string token;
  while ((pos = m_interestNames.find(delimeter)) != std::string::npos) {
    token = m_interestNames.substr(0, pos);
    m_interestNameList.push_back(token);
    m_lastSeq[token] = m_initSeq;
    m_interestNames.erase(0, pos + delimeter.length());
  }

  m_interestNameList.push_back(m_interestNames);
  m_lastSeq[m_interestNames] = m_initSeq;
  m_goodTrafficSeq = m_initSeq;
  m_originFreq = m_frequency;

  // NS_LOG_INFO("Current Frequency: " << m_frequency);
  // NS_LOG_INFO("Current Max Range: " << m_maxSeq);

  ScheduleNextPacket();
  if (m_goodTrafficAlso > 0) {
    ScheduleNextGoodPacket();
  }
}

// Processing when application is stopped
void
ConsApp::StopApplication()
{
  // cleanup ndn::App
  ndn::App::StopApplication();
}

void
ConsApp::SendGoodInterest()
{
  std::string interestName = "/good/interest"
    + std::to_string(m_goodTrafficSeq) + std::to_string(GetNode()->GetId());
  m_goodTrafficSeq++;
  Name interest_copy(interestName);

  auto interest = std::make_shared<ndn::Interest>(interest_copy);
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setMustBeFresh(true);
  interest->setTag(make_shared<lp::HopCountTag>(0));

  // Call trace (for logging purposes)
  m_transmittedInterests(interest, this, m_face);

  m_appLink->onReceiveInterest(*interest);
  NS_LOG_INFO("Sending Good Interest " << interest->getName());

  ScheduleNextGoodPacket();
}

void
ConsApp::SendInterest()
{
  if (m_frequency == 0) {
    ScheduleNextPacket();
    return;
  }

  /////////////////////////////////////////////
  // Sending one Interest packet out randomly//
  ////////////////////////////////////////////
  int random = rand() % m_interestNameList.size();
  std::string interestName = m_interestNameList.at(random);

  Name interest_copy(interestName);

  int lastSeq = m_lastSeq.find(interestName)->second;

  if (m_validInterest){
    if (m_isGood) {
      interest_copy.append("g" + std::to_string(lastSeq + 1));
    }
    else {
      interest_copy.append("b" + std::to_string(lastSeq + 1));
    }
    lastSeq += 1;

    if (lastSeq == m_maxSeq){
      lastSeq = -1;
    }
  }
  else{
    // fake Interest packet
    interest_copy.append("a" + std::to_string(rand()%400) + std::to_string(lastSeq + 1));
    lastSeq += 1;

    if (lastSeq == m_maxSeq){
      lastSeq = -1;
    }
  }

  m_lastSeq[interestName] = lastSeq;

  // Create and configure ndn::Interest
  auto interest = std::make_shared<ndn::Interest>(interest_copy);
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setMustBeFresh(true);

  interest->setTag(make_shared<lp::HopCountTag>(0));

  // Call trace (for logging purposes)
  m_transmittedInterests(interest, this, m_face);

  if (m_goodTrafficAlso > 0) {
    NS_LOG_INFO("Send Attack Interest " << interest->getName());
  }

  m_appLink->onReceiveInterest(*interest);
  ScheduleNextPacket();
}

// Callback that will be called when Data arrives
void
ConsApp::OnData(std::shared_ptr<const ndn::Data> data)
{
  // NS_LOG_INFO("DATA received for name: " << data->getName());
}

// Callback that will be called when NACK arrives
void
ConsApp::OnNack(std::shared_ptr<const ndn::lp::Nack> nack)
{
  // NS_LOG_INFO("NACK received");
  if (m_isGood || m_play_by_rule) {
    if (nack->getReason() == lp::NackReason::DDOS_RESET_RATE) {
      m_frequency = m_originFreq;
      std::cout << "Consumer: I reset my frequency!!!!!! the new value " << m_frequency << std::endl;
    }
    else if (nack->getReason() == lp::NackReason::DDOS_FAKE_INTEREST
             || nack->getReason() == lp::NackReason::DDOS_VALID_INTEREST_OVERLOAD) {
      m_frequency = nack->getHeader().m_tolerance - 1;
      if (m_frequency >= m_originFreq) {
        m_frequency = m_originFreq;
      }
      if (m_frequency <= 0) {
        m_frequency = 0;
      }
      std::cout << "Consumer: I change my frequency!!!!!! the new value " << m_frequency << std::endl;
    }
  }
}

} // namespace ndn
} // namespace ns3
