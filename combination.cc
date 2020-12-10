#include <iostream>
#include <cstdlib>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/config.h"

#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/netanim-module.h"

#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor.h"
#include "ns3/packet-socket-address.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"

#include "ns3/bulk-send-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("EPC_LTE_WI-FI_Example");

int main(int argc, char *argv[]) {

    Time::SetResolution (Time::NS);

    uint16_t eNodes = 5;
    uint16_t userStatic = 10;
    uint16_t userMobile = 10;
    double simTime = 60;
    Time interPacketInterval = MilliSeconds (1000);

    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // The below value configures the default behavior of global routing.
    // By default, it is disabled.  To respond to interface events, set to true
    Config::SetDefault ("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue (true));

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
    lteHelper->SetEpcHelper (epcHelper);

    Ptr<Node> pgw = epcHelper->GetPgwNode ();

    // 5 Wi-Fi APs, Wi-Fi Gateway, ISP Gateway and Remote Host
    NodeContainer p2pNodes;
    p2pNodes.Create (8);

    // Remote Host Node
    Ptr<Node> remoteHost = p2pNodes.Get (0);

    // ISP Gateway Node
    Ptr<Node> ispMainGateway = p2pNodes.Get(1);

    // Wi-Fi Gateway Node
    Ptr<Node> wifiGateway = p2pNodes.Get(2);

    InternetStackHelper internet;

    // Remote Host and ISP
    NodeContainer remoteISP = NodeContainer(p2pNodes.Get(0), p2pNodes.Get(1));
    // ISP and Wi-Fi Gateway
    NodeContainer ispGateway = NodeContainer(p2pNodes.Get(2), p2pNodes.Get(1));
    // ISP and PGW
    NodeContainer ispPGW = NodeContainer(pgw, p2pNodes.Get(1));
    // W-Fi Gateway and AP Node 1
    NodeContainer gatewayAP1 = NodeContainer(p2pNodes.Get(2), p2pNodes.Get(3));
    // W-Fi Gateway and AP Node 2
    NodeContainer gatewayAP2 = NodeContainer(p2pNodes.Get(2), p2pNodes.Get(4));
    // W-Fi Gateway and AP Node 3
    NodeContainer gatewayAP3 = NodeContainer(p2pNodes.Get(2), p2pNodes.Get(5));
    // W-Fi Gateway and AP Node 4
    NodeContainer gatewayAP4 = NodeContainer(p2pNodes.Get(2), p2pNodes.Get(6));
    // W-Fi Gateway and AP Node 5
    NodeContainer gatewayAP5 = NodeContainer(p2pNodes.Get(2), p2pNodes.Get(7));

    internet.Install(p2pNodes);

    // Create the Internet
    NS_LOG_INFO ("Create channels.");
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
    p2p.SetDeviceAttribute ("Mtu", UintegerValue (1500));
    p2p.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));
    NetDeviceContainer pgwAndispGateway = p2p.Install (ispPGW);
    NetDeviceContainer wifiGatewayAndispGateway = p2p.Install (ispGateway);
    NetDeviceContainer remoteAndISP = p2p.Install (remoteISP);
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer remoteAndispIPs = ipv4.Assign (remoteAndISP);
    Ipv4InterfaceContainer isppgwIPs = ipv4.Assign(pgwAndispGateway);
    Ipv4InterfaceContainer ispwifiIPs = ipv4.Assign(wifiGatewayAndispGateway);
    Ipv4Address remoteHostAddr = remoteAndispIPs.GetAddress (0);

    p2p.SetDeviceAttribute ("DataRate", StringValue ("50Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));
    NetDeviceContainer ga1 = p2p.Install (gatewayAP1);
    NetDeviceContainer ga2 = p2p.Install (gatewayAP2);
    NetDeviceContainer ga3 = p2p.Install (gatewayAP3);
    NetDeviceContainer ga4 = p2p.Install (gatewayAP4);
    NetDeviceContainer ga5 = p2p.Install (gatewayAP5);

    // ------ WIFI SETUP FOR USERS -------------------------------------------

    // wifi host nodes
    // Static
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create (userStatic);
    // Mobile
    NodeContainer wifiMobNodes;
    wifiMobNodes.Create (userMobile);

    internet.Install(wifiStaNodes);
    internet.Install(wifiMobNodes);

    NodeContainer allNodes = NodeContainer(wifiStaNodes, wifiMobNodes);

    NodeContainer waps;
    waps.Add(p2pNodes.Get(3));
    waps.Add(p2pNodes.Get(4));
    waps.Add(p2pNodes.Get(5));
    waps.Add(p2pNodes.Get(6));
    waps.Add(p2pNodes.Get(7));

    // constructs the wifi devices and the interconnection channel between these wifi nodes
    YansWifiChannelHelper channel1 = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
    phy.SetChannel (channel1.Create ());
    phy.Set("ChannelNumber", UintegerValue(1));

    YansWifiChannelHelper channel2 = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy2 = YansWifiPhyHelper::Default ();
    phy2.SetChannel (channel2.Create ());
    phy2.Set("ChannelNumber", UintegerValue(2));

    YansWifiChannelHelper channel3 = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy3 = YansWifiPhyHelper::Default ();
    phy3.SetChannel (channel3.Create ());
    phy3.Set("ChannelNumber", UintegerValue(3));

    YansWifiChannelHelper channel4 = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy4 = YansWifiPhyHelper::Default ();
    phy4.SetChannel (channel4.Create ());
    phy4.Set("ChannelNumber", UintegerValue(4));

    YansWifiChannelHelper channel5 = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy5 = YansWifiPhyHelper::Default ();
    phy5.SetChannel (channel5.Create ());
    phy5.Set("ChannelNumber", UintegerValue(5));

    WifiHelper wifi;
    wifi.SetRemoteStationManager ("ns3::ArfWifiManager");

    // Ssid ssid = Ssid ("wifi-default");
    Ssid ssid1 = Ssid ("ns-3-ssid-1");
    Ssid ssid2 = Ssid ("ns-3-ssid-2");
    Ssid ssid3 = Ssid ("ns-3-ssid-3");
    Ssid ssid4 = Ssid ("ns-3-ssid-4");
    Ssid ssid5 = Ssid ("ns-3-ssid-5");

    WifiMacHelper mac1;
    mac1.SetType ("ns3::StaWifiMac",
        "Ssid", SsidValue (ssid1),
        "ActiveProbing", BooleanValue (false));

    WifiMacHelper mac2;
    mac2.SetType ("ns3::StaWifiMac",
        "Ssid", SsidValue (ssid2),
        "ActiveProbing", BooleanValue (false));

    WifiMacHelper mac3;
    mac3.SetType ("ns3::StaWifiMac",
        "Ssid", SsidValue (ssid3),
        "ActiveProbing", BooleanValue (false));

    WifiMacHelper mac4;
    mac4.SetType ("ns3::StaWifiMac",
        "Ssid", SsidValue (ssid4),
        "ActiveProbing", BooleanValue (false));

    WifiMacHelper mac5;
    mac5.SetType ("ns3::StaWifiMac",
        "Ssid", SsidValue (ssid5),
        "ActiveProbing", BooleanValue (false));

    // Once all the station-specific parameters are fully configured,
    // both at the MAC and PHY layers, we can invoke our now-familiar
    // Install method to create the wifi devices of these stations

    NodeContainer team1, team2, team3, team4, team5;
    for(int i=0; i<userMobile+userStatic; i++) {
        if(i<4) {
            team1.Add(allNodes.Get(i));
        } else if(i<8) {
            team2.Add(allNodes.Get(i));
        } else if(i<12) {
            team3.Add(allNodes.Get(i));
        } else if(i<16) {
            team4.Add(allNodes.Get(i));
        } else {
            team5.Add(allNodes.Get(i));
        }
    }

    NetDeviceContainer deviceTeam1 = wifi.Install (phy, mac1, team1);
    NetDeviceContainer deviceTeam2 = wifi.Install (phy2, mac2, team2);
    NetDeviceContainer deviceTeam3 = wifi.Install (phy3, mac3, team3);
    NetDeviceContainer deviceTeam4 = wifi.Install (phy4, mac4, team4);
    NetDeviceContainer deviceTeam5 = wifi.Install (phy5, mac5, team5);

    mac1.SetType ("ns3::ApWifiMac", 
        "Ssid", SsidValue (ssid1),
        "BeaconInterval", TimeValue (Seconds (1)));

    mac2.SetType ("ns3::ApWifiMac", 
        "Ssid", SsidValue (ssid2),
        "BeaconInterval", TimeValue (Seconds (1)));

    mac3.SetType ("ns3::ApWifiMac", 
        "Ssid", SsidValue (ssid3),
        "BeaconInterval", TimeValue (Seconds (1)));

    mac4.SetType ("ns3::ApWifiMac", 
        "Ssid", SsidValue (ssid4),
        "BeaconInterval", TimeValue (Seconds (1)));

    mac5.SetType ("ns3::ApWifiMac", 
        "Ssid", SsidValue (ssid5),
        "BeaconInterval", TimeValue (Seconds (1)));

    // The next lines create the 5 APs which shares
    // the same set of PHY-level Attributes (and channel) as the stations
    NetDeviceContainer apDevices1 = wifi.Install (phy, mac1, p2pNodes.Get(3));
    NetDeviceContainer apDevices2 = wifi.Install (phy2, mac2, p2pNodes.Get(4));
    NetDeviceContainer apDevices3 = wifi.Install (phy3, mac3, p2pNodes.Get(5));
    NetDeviceContainer apDevices4 = wifi.Install (phy4, mac4, p2pNodes.Get(6));
    NetDeviceContainer apDevices5 = wifi.Install (phy5, mac5, p2pNodes.Get(7));


    // ------ END OF WIFI SETUP FOR USERS -------------------------------------

    std::cout << "Initializing nodes" << std::endl;

    NodeContainer ueNodesMobile;
    NodeContainer ueNodesStatic;
    NodeContainer femptoEnbNodes;
    NodeContainer macroEnbNodes;
    ueNodesMobile.Create (userMobile);
    ueNodesStatic.Create (userStatic);
    femptoEnbNodes.Create (eNodes - 1);
    macroEnbNodes.Create (1);

    NodeContainer enbs;
    for(int i=0; i<eNodes-1; i++)
        enbs.Add(femptoEnbNodes.Get(i));
    enbs.Add(macroEnbNodes.Get((0)));

    std::cout << "Setting Mobility to Nodes" << std::endl;

    // ----- Start -------------- Install Mobility Models ---------------------------------------------------------------------------------------

    ObjectFactory pos;
    pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
    pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=220.0]"));
    pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=120.0]"));

    Ptr<PositionAllocator> positionAlloc = pos.Create ()->GetObject<PositionAllocator> ();

    MobilityHelper mobility;

    mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                           "Speed", StringValue ("ns3::UniformRandomVariable[Min=0.1|Max=3]"),
                           "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.1]"),
                           "PositionAllocator", PointerValue (positionAlloc));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.Install(ueNodesMobile);
    mobility.Install(wifiMobNodes);
    std::cout << "Mobility for Mobile UEs and Wi-Fi Hosts ----> Done" << std::endl;

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(positionAlloc);

    mobility.Install(ueNodesStatic);
    mobility.Install(wifiStaNodes);
    std::cout << "Mobility for Static UEs and Wi-Fi Hosts ----> Done" << std::endl;

    mobility.Install(pgw);
    mobility.Install(wifiGateway);
    std::cout << "Mobility for P-GW/S-GW and Wi-Fi Gateway ----> Done" << std::endl;

    mobility.Install(remoteHost);
    std::cout << "Mobility for Remote Host ----> Done" << std::endl;

    mobility.Install(ispMainGateway);
    std::cout << "Mobility for Main ISP Gateway ----> Done" << std::endl;

    Ptr<ListPositionAllocator> posAlloc = CreateObject<ListPositionAllocator> ();
    
    posAlloc->Add (Vector(10, 10, 0));
    posAlloc->Add (Vector(210, 10, 0));
    posAlloc->Add (Vector(10, 110, 0));
    posAlloc->Add (Vector(210, 110, 0));
    posAlloc->Add (Vector(105, 55, 0));
    
    mobility.SetPositionAllocator(posAlloc);

    mobility.Install(enbs);
    mobility.Install(waps);
    std::cout << "Mobility for fempto, macro eNBs and Wi-Fi APs ----> Done" << std::endl;

    // ----- End -------------- Install Mobility Models ---------------------------------------------------------------------------------------

    // --------- LTE ---------------------------------------------------------------------------------------

    Ipv4StaticRoutingHelper ipv4RoutingHelper;

    // Install LTE Devices to the nodes
    NetDeviceContainer enbsLteDevs = lteHelper->InstallEnbDevice (enbs);
    NetDeviceContainer mueLteDevs = lteHelper->InstallUeDevice (ueNodesMobile);
    NetDeviceContainer sueLteDevs = lteHelper->InstallUeDevice (ueNodesStatic);

    Ipv4InterfaceContainer ueIpIface;
    // Install the IP stack on the Mobile UEs
    internet.Install (ueNodesMobile);
    ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (mueLteDevs));
    // Assign IP address to UEs, and install applications
    for (uint32_t u = 0; u < ueNodesMobile.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodesMobile.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

    lteHelper->AttachToClosestEnb(mueLteDevs, enbsLteDevs);

    // Install the IP stack on the Static UEs
    internet.Install (ueNodesStatic);
    ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (sueLteDevs));
    // Assign IP address to UEs, and install applications
    for (uint32_t u = 0; u < ueNodesStatic.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodesStatic.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

    lteHelper->AttachToClosestEnb(sueLteDevs, enbsLteDevs);

    // --------- Wi-Fi ---------------------------------------------------------------------------------------

    NS_LOG_INFO ("Assign IP Addresses.");

    // Wi-Fi Gateway & First AP IPs
    ipv4.SetBase ("9.1.0.0", "255.255.255.0");
    Ipv4InterfaceContainer ga1Addresses = ipv4.Assign (ga1);

    // Wi-Fi Gateway & Second AP IPs
    ipv4.SetBase ("9.2.0.0", "255.255.255.0");
    Ipv4InterfaceContainer ga2Addresses = ipv4.Assign (ga2);

    // Wi-Fi Gateway & Third AP IPs
    ipv4.SetBase ("9.3.0.0", "255.255.255.0");
    Ipv4InterfaceContainer ga3Addresses = ipv4.Assign (ga3);

    // Wi-Fi Gateway & Forth AP IPs
    ipv4.SetBase ("9.4.0.0", "255.255.255.0");
    Ipv4InterfaceContainer ga4Addresses = ipv4.Assign (ga4);

    // Wi-Fi Gateway & Fifth AP IPs
    ipv4.SetBase ("9.5.0.0", "255.255.255.0");
    Ipv4InterfaceContainer ga5Addresses = ipv4.Assign (ga5);

    // Different subnets for each Wi-Fi Ap and each Wi-Fi Hosts
    // First AP Group IPs
    ipv4.SetBase ("10.1.0.0", "255.255.255.0");
    Ipv4InterfaceContainer wfaAddress1 = ipv4.Assign (apDevices1);
    ipv4.Assign (deviceTeam1);

    // Second AP Group IPs
    ipv4.SetBase ("10.2.0.0", "255.255.255.0");
    Ipv4InterfaceContainer wfaAddress2 = ipv4.Assign (apDevices2);
    ipv4.Assign (deviceTeam2);

    // Third AP Group IPs
    ipv4.SetBase ("10.3.0.0", "255.255.255.0");
    Ipv4InterfaceContainer wfaAddress3 = ipv4.Assign (apDevices3);
    ipv4.Assign (deviceTeam3);

    // Forth AP Group IPs
    ipv4.SetBase ("10.4.0.0", "255.255.255.0");
    Ipv4InterfaceContainer wfaAddress4 = ipv4.Assign (apDevices4);
    ipv4.Assign (deviceTeam4);

    // Fifth AP Group IPs
    ipv4.SetBase ("10.5.0.0", "255.255.255.0");
    Ipv4InterfaceContainer wfaAddress5 = ipv4.Assign (apDevices5);
    ipv4.Assign (deviceTeam5);

    // -------- Start Routing -----------------------------------------------------------------------------------------------------------------

    std::cout << "Before Remote Host" << std::endl;

    // remote Host routing
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
    remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), remoteAndispIPs.GetAddress(1), 1);     // UEs subnet
    remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("10.0.0.0"), Ipv4Mask ("255.0.0.0"), remoteAndispIPs.GetAddress(1), 1);    // Wi-Fi hosts subnet

    std::cout << "Before ISP Gateway" << std::endl;

    // isp Gateway routing
    Ptr<Ipv4StaticRouting> ISPStaticRouting = ipv4RoutingHelper.GetStaticRouting (ispMainGateway->GetObject<Ipv4> ());
    ISPStaticRouting->AddNetworkRouteTo (Ipv4Address ("1.0.0.0"), Ipv4Mask ("255.0.0.0"), remoteAndispIPs.GetAddress(0), 1);    // Remote Host subnet
    ISPStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), isppgwIPs.GetAddress(0), 2);          // UEs subnet
    ISPStaticRouting->AddNetworkRouteTo (Ipv4Address ("10.0.0.0"), Ipv4Mask ("255.0.0.0"), ispwifiIPs.GetAddress(0), 3);        // Wi-Fi hosts subnet

    std::cout << "Before PGW" << std::endl;

    // pgw routing
    Ptr<Ipv4StaticRouting> pgwStaticRouting = ipv4RoutingHelper.GetStaticRouting (pgw->GetObject<Ipv4> ());
    pgwStaticRouting->AddNetworkRouteTo (Ipv4Address ("1.0.0.0"), Ipv4Mask ("255.0.0.0"), isppgwIPs.GetAddress(1), 2);  // Remote Host subnet
    pgwStaticRouting->AddHostRouteTo (Ipv4Address ("7.0.0.0"), 1);                                                      // UEs subnet

    std::cout << "Before Wi-Fi Gateway" << std::endl;

    // // wifi gateway routing
    Ptr<Ipv4StaticRouting> wifiGatewayStaticRouting = ipv4RoutingHelper.GetStaticRouting (wifiGateway->GetObject<Ipv4> ());
    wifiGatewayStaticRouting->AddNetworkRouteTo (Ipv4Address ("1.0.0.0"), Ipv4Mask ("255.255.255.0"), ispwifiIPs.GetAddress(1), 1);     // Remote Host subnet
    wifiGatewayStaticRouting->AddNetworkRouteTo (Ipv4Address ("10.1.0.0"), Ipv4Mask ("255.255.255.0"), ga1Addresses.GetAddress(1), 2);  // Wi-Fi AP1 subnet
    wifiGatewayStaticRouting->AddNetworkRouteTo (Ipv4Address ("10.2.0.0"), Ipv4Mask ("255.255.255.0"), ga2Addresses.GetAddress(1), 3);  // Wi-Fi AP2 subnet
    wifiGatewayStaticRouting->AddNetworkRouteTo (Ipv4Address ("10.3.0.0"), Ipv4Mask ("255.255.255.0"), ga3Addresses.GetAddress(1), 4);  // Wi-Fi AP3 subnet
    wifiGatewayStaticRouting->AddNetworkRouteTo (Ipv4Address ("10.4.0.0"), Ipv4Mask ("255.255.255.0"), ga4Addresses.GetAddress(1), 5);  // Wi-Fi AP4 subnet
    wifiGatewayStaticRouting->AddNetworkRouteTo (Ipv4Address ("10.5.0.0"), Ipv4Mask ("255.255.255.0"), ga5Addresses.GetAddress(1), 6);  // Wi-Fi AP5 subnet

    std::cout << "Before Wi-Fi APs" << std::endl;

    // wifi aps routing
    Ptr<Ipv4StaticRouting> firstWifiApStaticRouting = ipv4RoutingHelper.GetStaticRouting (p2pNodes.Get(3)->GetObject<Ipv4> ());
    firstWifiApStaticRouting->AddNetworkRouteTo (Ipv4Address ("1.0.0.0"), Ipv4Mask ("255.255.255.0"), ga1Addresses.GetAddress(0), 1);   // Remote Host subnet

    Ptr<Ipv4StaticRouting> secondWifiApStaticRouting = ipv4RoutingHelper.GetStaticRouting (p2pNodes.Get(4)->GetObject<Ipv4> ());
    secondWifiApStaticRouting->AddNetworkRouteTo (Ipv4Address ("1.0.0.0"), Ipv4Mask ("255.255.255.0"), ga2Addresses.GetAddress(0), 1);  // Remote Host subnet

    Ptr<Ipv4StaticRouting> thirdWifiApStaticRouting = ipv4RoutingHelper.GetStaticRouting (p2pNodes.Get(5)->GetObject<Ipv4> ());
    thirdWifiApStaticRouting->AddNetworkRouteTo (Ipv4Address ("1.0.0.0"), Ipv4Mask ("255.255.255.0"), ga3Addresses.GetAddress(0), 1);   // Remote Host subnet

    Ptr<Ipv4StaticRouting> forthWifiApStaticRouting = ipv4RoutingHelper.GetStaticRouting (p2pNodes.Get(6)->GetObject<Ipv4> ());
    forthWifiApStaticRouting->AddNetworkRouteTo (Ipv4Address ("1.0.0.0"), Ipv4Mask ("255.255.255.0"), ga4Addresses.GetAddress(0), 1);   // Remote Host subnet

    Ptr<Ipv4StaticRouting> fifthWifiApStaticRouting = ipv4RoutingHelper.GetStaticRouting (p2pNodes.Get(7)->GetObject<Ipv4> ());
    fifthWifiApStaticRouting->AddNetworkRouteTo (Ipv4Address ("1.0.0.0"), Ipv4Mask ("255.255.255.0"), ga5Addresses.GetAddress(0), 1);   // Remote Host subnet

    Ptr<Ipv4StaticRouting> wfhStaticRouting;
    for(int i=0; i<userMobile+userStatic; i++) {
        wfhStaticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> (allNodes.Get(i)->GetObject<Ipv4> ()->GetRoutingProtocol ());
        if(i<4) {
            wfhStaticRouting->SetDefaultRoute(wfaAddress1.GetAddress(0), 1);
        } else if(i<8) {
            wfhStaticRouting->SetDefaultRoute(wfaAddress2.GetAddress(0), 1);
        } else if(i<12) {
            wfhStaticRouting->SetDefaultRoute(wfaAddress3.GetAddress(0), 1);
        } else if(i<16) {
            wfhStaticRouting->SetDefaultRoute(wfaAddress4.GetAddress(0), 1);
        } else {
            wfhStaticRouting->SetDefaultRoute(wfaAddress5.GetAddress(0), 1);
        }
    }

    std::cout << "Before Populate Routing Tables" << std::endl;

    // Create router nodes, initialize routing database and set up the routing
    // tables in the nodes.
    // Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    std::cout << "Populating Routing Tables done." << std::endl;
    // ----------- End Routing ------------------------------------------------------------------------------------------------------------------------------------------

    NS_LOG_INFO ("Create Applications.");

    // VoIP port
    uint16_t portVoIP = 9;

    // Web port
    uint16_t portWeb = 12;

    // FTP port
    uint16_t portFTP = 15;

    // We put the echo server on the “rightmost” node

std::cout << "Before Servers on Remote Host" << std::endl;
    // VoIP server
    UdpEchoServerHelper echoServerVoIP (portVoIP);

    ApplicationContainer serverAppsVoIP = echoServerVoIP.Install (remoteHost);
    serverAppsVoIP.Start (Seconds (1.0));
    serverAppsVoIP.Stop (Seconds (simTime));

    // Web server
    UdpEchoServerHelper echoServerWeb (portWeb);

    ApplicationContainer serverAppsWeb = echoServerWeb.Install (remoteHost);
    serverAppsWeb.Start (Seconds (1.0));
    serverAppsWeb.Stop (Seconds (simTime));

    // FTP server
    UdpEchoServerHelper echoServerFTP (portFTP);

    ApplicationContainer serverAppsFTP = echoServerFTP.Install (remoteHost);
    serverAppsFTP.Start (Seconds (1.0));
    serverAppsFTP.Stop (Seconds (simTime));

    // UEs Applications

std::cout << "Before Applications" << std::endl;
    // VoIP Applications
    OnOffHelper onoffVoIP ("ns3::UdpSocketFactory",
                        InetSocketAddress (remoteHostAddr, portVoIP));
    onoffVoIP.SetConstantRate (DataRate ("56kbps")); // https://www.webart.gr/%CE%B1%CF%81%CE%B8%CF%81%CE%BF-%CF%84%CE%B9-%CE%B5%CE%B9%CE%BD%CE%B1%CE%B9-%CF%84%CE%BF-bandwidth/42
    onoffVoIP.SetAttribute ("PacketSize", UintegerValue (100));

    ApplicationContainer appsVoIP;
    for (int i=0; i<3; i++) {
        // UEs
        appsVoIP = onoffVoIP.Install (ueNodesStatic.Get(i));
        appsVoIP = onoffVoIP.Install (ueNodesMobile.Get(i));
        // Wi-Fi hosts
        appsVoIP = onoffVoIP.Install (wifiStaNodes.Get(i));
        appsVoIP = onoffVoIP.Install (wifiMobNodes.Get(i));
    }
    appsVoIP = onoffVoIP.Install (ueNodesMobile.Get(3));
    appsVoIP = onoffVoIP.Install (wifiMobNodes.Get(3));
    appsVoIP.Start (Seconds (1.0));
    appsVoIP.Stop (Seconds (simTime - 5));
    
    // Web Applications
    OnOffHelper onoffWeb ("ns3::UdpSocketFactory",
                        InetSocketAddress (remoteHostAddr, portWeb));
    onoffWeb.SetConstantRate (DataRate ("3Mbps"));
    onoffWeb.SetAttribute ("PacketSize", UintegerValue (10000));

    ApplicationContainer appsWeb;
    appsWeb = onoffWeb.Install (ueNodesStatic.Get(3));
    appsWeb = onoffWeb.Install (wifiStaNodes.Get(3));
    for (int i=4; i<7; i++) {
        // UEs
        appsWeb = onoffWeb.Install (ueNodesStatic.Get(i));
        appsWeb = onoffWeb.Install (ueNodesMobile.Get(i));
        // Wi-Fi hosts
        appsWeb = onoffWeb.Install (wifiStaNodes.Get(i));
        appsWeb = onoffWeb.Install (wifiMobNodes.Get(i));
    }
    appsWeb.Start (Seconds (1.0));
    appsWeb.Stop (Seconds (simTime - 5));

    // FTP Applications
    BulkSendHelper onoffFTP("ns3::TcpSocketFactory",     // http://repositorio.cedia.org.ec/bitstream/123456789/960/9/T9_Serviciosenns3_vf.pdf
                      InetSocketAddress (remoteHostAddr, portFTP));
    // onoffFTP.SetAttribute ("Remote", remoteHostAddr);
    onoffFTP.SetAttribute ("SendSize", UintegerValue (int(50000)));
    onoffFTP.SetAttribute("MaxBytes", UintegerValue(int(0)));
    
    ApplicationContainer appsFTP;
    for (int i=7; i<userMobile; i++) {
        // UEs
        appsFTP = onoffFTP.Install (ueNodesStatic.Get(i));
        appsFTP = onoffFTP.Install (ueNodesMobile.Get(i));
        // Wi-Fi hosts
        appsFTP = onoffFTP.Install (wifiStaNodes.Get(i));
        appsFTP = onoffFTP.Install (wifiMobNodes.Get(i));
    }
    appsFTP.Start (Seconds (1.0));
    appsFTP.Stop (Seconds (simTime - 5));

    p2p.EnablePcapAll ("P2P connections");
    phy.EnablePcapAll ("Wifi physicall 1", apDevices1.Get (0));
    phy2.EnablePcapAll ("Wifi physicall 2", apDevices2.Get (0));
    phy3.EnablePcapAll ("Wifi physicall 3", apDevices3.Get (0));
    phy4.EnablePcapAll ("Wifi physicall 4", apDevices4.Get (0));
    phy5.EnablePcapAll ("Wifi physicall 5", apDevices5.Get (0));
    lteHelper->EnableTraces();

    // NetAnim
    AnimationInterface anim("combination.xml");

    FlowMonitorHelper flowmonHelper;
    flowmonHelper.InstallAll ();

    // Trace routing tables 
    Ipv4GlobalRoutingHelper g;
    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("combination-routing.routes", std::ios::out);
    g.PrintRoutingTableAllAt (Seconds (12), routingStream);

    NS_LOG_INFO ("Run Simulation.");
    Simulator::Stop(Seconds(simTime));
    std::cout << "Done" << std::endl;
    Simulator::Run ();
    Simulator::Destroy ();
    NS_LOG_INFO ("Done.");

    flowmonHelper.SerializeToXmlFile ("combination.flowmon", false, true);
    
    return 0;
}