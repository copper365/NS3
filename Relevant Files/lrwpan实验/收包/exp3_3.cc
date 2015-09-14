#include <ns3/test.h>
#include <ns3/log.h>
#include <ns3/callback.h>
#include <ns3/packet.h>
#include <ns3/simulator.h>
#include <ns3/lr-wpan-error-model.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/lr-wpan-net-device.h>
#include <ns3/spectrum-value.h>
#include <ns3/lr-wpan-spectrum-value-helper.h>
#include <ns3/lr-wpan-mac.h>
#include <ns3/node.h>
#include <ns3/net-device.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/mac16-address.h>
#include <ns3/constant-position-mobility-model.h>
#include <ns3/uinteger.h>
#include <ns3/nstime.h>
#include <ns3/abort.h>
#include <ns3/command-line.h>
#include <ns3/gnuplot.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace ns3;
using namespace std;

static uint32_t g_received = 0;

NS_LOG_COMPONENT_DEFINE ("LrWpanErrorDistancePlot");

static void
LrWpanErrorDistanceCallback (McpsDataIndicationParams params, Ptr<Packet> p)
{
  g_received++;
}

int main (int argc, char *argv[])
{
  std::ostringstream os;
  std::ofstream berfile ("exp3_3-802.15.4-psr-txPower.plt");

  // int minDistance = 1;
  // int maxDistance = 200;  // meters
  int distance = 150;
  double increment = 0.1;          // increment of txPower
  int maxPackets = 1000;
  int packetSize = 50;
  double txPower = 1;
  double minTxPower = 0;
  double maxTxPower = 10;
  uint32_t channelNumber = 11;

  CommandLine cmd;

  cmd.AddValue ("txPower", "transmit power (dBm)", txPower);
  cmd.AddValue ("packetSize", "packet (MSDU) size (bytes)", packetSize);
  cmd.AddValue ("channelNumber", "channel number", channelNumber);

  cmd.Parse (argc, argv);

  os << "Packet (MSDU) size = " << packetSize << " bytes; distance = " << distance << " m; channel = " << channelNumber;

  Gnuplot psrplot = Gnuplot ("exp3_3-802.15.4-psr-txPower.eps");
  Gnuplot2dDataset psrdataset ("exp3_3-802.15.4-psr-vs-txPower");

  Ptr<Node> n0 = CreateObject <Node> ();
  Ptr<Node> n1 = CreateObject <Node> ();
  Ptr<LrWpanNetDevice> dev0 = CreateObject<LrWpanNetDevice> ();
  Ptr<LrWpanNetDevice> dev1 = CreateObject<LrWpanNetDevice> ();
  dev0->SetAddress (Mac16Address ("00:01"));
  dev1->SetAddress (Mac16Address ("00:02"));
  Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel> ();
  Ptr<LogDistancePropagationLossModel> model = CreateObject<LogDistancePropagationLossModel> ();
  channel->AddPropagationLossModel (model);
  dev0->SetChannel (channel);
  dev1->SetChannel (channel);
  n0->AddDevice (dev0);
  n1->AddDevice (dev1);
  Ptr<ConstantPositionMobilityModel> mob0 = CreateObject<ConstantPositionMobilityModel> ();
  dev0->GetPhy ()->SetMobility (mob0);
  Ptr<ConstantPositionMobilityModel> mob1 = CreateObject<ConstantPositionMobilityModel> ();
  dev1->GetPhy ()->SetMobility (mob1);

  LrWpanSpectrumValueHelper svh;
  Ptr<SpectrumValue> psd = svh.CreateTxPowerSpectralDensity (txPower, channelNumber);
  dev0->GetPhy ()->SetTxPowerSpectralDensity (psd);

  McpsDataIndicationCallback cb0;
  cb0 = MakeCallback (&LrWpanErrorDistanceCallback);
  dev1->GetMac ()->SetMcpsDataIndicationCallback (cb0);

  McpsDataRequestParams params;
  params.m_srcAddrMode = SHORT_ADDR;
  params.m_dstAddrMode = SHORT_ADDR;
  params.m_dstPanId = 0;
  params.m_dstAddr = Mac16Address ("00:02");
  params.m_msduHandle = 0;
  params.m_txOptions = 0;

  Ptr<Packet> p;
  mob0->SetPosition (Vector (0,0,0));
  mob1->SetPosition (Vector (distance,0,0));
  for (double j = minTxPower; j < maxTxPower;  )
    {
      for (int i = 0; i < maxPackets; i++)
        {
          p = Create<Packet> (packetSize);
          Simulator::Schedule (Seconds (i),
                               &LrWpanMac::McpsDataRequest,
                               dev0->GetMac (), params, p);
        }
      Simulator::Run ();
      NS_LOG_DEBUG ("Received " << g_received << " packets for txPower " << j);
      psrdataset.Add (j, g_received / double(maxPackets));
      g_received = 0;
      j += increment;
      psd = svh.CreateTxPowerSpectralDensity (j, channelNumber);
      dev0->GetPhy ()->SetTxPowerSpectralDensity (psd);
      // mob1->SetPosition (Vector (j,0,0));
    }

  psrplot.AddDataset (psrdataset);

  psrplot.SetTitle (os.str ());
  psrplot.SetTerminal ("postscript eps color enh \"Times-BoldItalic\"");
  psrplot.SetLegend ("txPower (dBm)", "Packet Success Rate (PSR)");
  psrplot.SetExtra  ("set xrange [0:10]\n\
set yrange [0:1]\n\
set grid\n\
set style line 1 linewidth 5\n\
set style increment user");
  psrplot.GenerateOutput (berfile);
  berfile.close ();

  Simulator::Destroy ();
  return 0;
}

