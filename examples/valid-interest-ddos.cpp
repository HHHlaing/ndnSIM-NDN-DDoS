// b-1-1.cpp
// Please make sure each time to set a different RngRun
// ./waf --run "fake-interest-ddos --RngRun=2 --maxRange=400 --frequency=200 --capacity=200 --withStrategy=false --output=test"
// fake-interest-ddos topology contains: 12 good users, 60 attackers
// B-1: Interest Aggregation with valid Interests

#include "helper.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/ddos-strategy.hpp"

namespace ns3 {

int
main(int argc, char* argv[]) {

  Config::SetDefault("ns3::PointToPointNetDevice::Mtu", StringValue("9000"));

  // parameters
  std::string maxRange = "10000";
  std::string frequency = "200";
  std::string topo = "valid-interest-ddos";
  std::string outFile = "test";
  std::string useStrategy = "true";
  std::string capacity = "200";

  CommandLine cmd;
  cmd.AddValue("maxRange", "Max Data Range", maxRange);
  cmd.AddValue("frequency", "Sending Frequency", frequency);
  cmd.AddValue("capacity", "valid capacity", capacity);
  cmd.AddValue("withStrategy", "Whether use ddos strategy", useStrategy);
  cmd.AddValue("topo", "Topology File", topo);
  cmd.AddValue("output", "Output File Name", outFile);
  cmd.Parse(argc, argv);

  AnnotatedTopologyReader topologyReader("", 1);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/" + topo + ".txt");
  topologyReader.Read();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Nocache");
  ndnHelper.SetDefaultRoutes(false);
  ndnHelper.InstallAll();

  if (useStrategy == "true") {
    ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/ddos");
  }

  ndn::AppHelper consumerHelper("ConsApp");
  consumerHelper.SetAttribute("Name", StringValue("/u1/cs/server"));
  consumerHelper.SetAttribute("Frequency", StringValue("10"));
  consumerHelper.SetAttribute("MaxSeq", StringValue(maxRange));
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();


  NodeContainer consumers;
  fillConsumerContainer(consumers);
  for (int i = 0; i < consumers.size(); i++) {
    int init = static_cast<int>(x->GetValue()*(std::stoi(maxRange) - 1));
    consumerHelper.SetAttribute("InitSeq", IntegerValue(init));
    consumerHelper.Install(consumers[i]);
  }

  NodeContainer attackers;
  fillAttackerContainer(attackers);
  consumerHelper.SetAttribute("Frequency", StringValue(frequency));
  for (int i = 0; i < attackers.size(); i++) {
    ApplicationContainer evilApp;
    int init = static_cast<int>(x->GetValue()*(std::stoi(maxRange) - 1));
    consumerHelper.SetAttribute("InitSeq", IntegerValue(init));
    evilApp.Add(consumerHelper.Install(attackers[i]));

    evilApp.Start(Seconds (10.0));
  }

  // Getting producers
  Ptr<Node> as1_cs_server = Names::Find<Node>("as1-cs-server");
  ndn::AppHelper producerHelper("DDoSProdApp");
  producerHelper.SetPrefix("/u1/cs/server");
  producerHelper.SetAttribute("ValidThreshold", StringValue(capacity));
  producerHelper.Install(as1_cs_server);

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();
  ndnGlobalRoutingHelper.AddOrigins("/u1/cs/server", as1_cs_server);
  Ptr<Node> as1_cs = Names::Find<Node>("as1-cs");
  ndnGlobalRoutingHelper.AddOrigins("/u1/cs", as1_cs);
  ndnGlobalRoutingHelper.CalculateRoutes();

  Simulator::Stop(Seconds(30.0));
  ndn::L3RateTracer::InstallAll("src/ndnSIM/Results/vaild-interest-ddos/" + outFile + ".txt",
                                Seconds(0.5));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[]) {
  return ns3::main(argc, argv);
}
