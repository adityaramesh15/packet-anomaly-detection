#include "packet_parser.hpp"
#include <chrono> 
#include <iostream> 

using namespace Tins; 
using json = nlohmann::json;

//Initialization of static variables for linking
std::unordered_map<std::string, int> PacketParser::connection_count_;
std::unordered_map<std::string, int> PacketParser::service_count_;

PacketParser::PacketParser() : duration_(0), protocol_type_(""), service_(""), flag_(""),
      src_bytes_(0), count_(0), srv_count_(0), src_ip_("0.0.0.0"), dst_ip_("0.0.0.0"){}

std::string PacketParser::parse(const Packet& packet) {
    auto timestamp = std::chrono::system_clock::now();
    static auto last_packet_time = timestamp;
    duration_ = std::chrono::duration<double>(timestamp - last_packet_time).count();
    last_packet_time = timestamp;

    if(const IP* ip = packet.pdu()->find_pdu<IP>()) {
        return parseIPv4(*ip);
    } else if(const IPv6* ipv6 = packet.pdu()->find_pdu<IPv6>()) {
        return parseIPv6(*ipv6);
    } else {
        std::cout << pduName(packet.pdu()->inner_pdu()->pdu_type()) << " Packet Encountered." << std::endl;
        return "";
    }
}

std::string PacketParser::parseIPv4(const IP& ip) {
    src_bytes_ = ip.tot_len();
    
    src_ip_ = ip.src_addr().to_string();
    dst_ip_ = ip.dst_addr().to_string();

    std::string connection_key = src_ip_ + "-" + dst_ip_;
    count_ = connection_count_[connection_key]++;

    parseTcpUdp(ip);

    std::string service_key = dst_ip_ + "-" + service_;
    srv_count_ = service_count_[service_key]++;
    
    return toJSON();
}

std::string PacketParser::parseIPv6(const IPv6& ipv6) {
    src_bytes_ = ipv6.payload_length() + 40; 
    
    src_ip_ = ipv6.src_addr().to_string();
    dst_ip_ = ipv6.dst_addr().to_string();

    std::string connection_key = src_ip_ + "-" + dst_ip_;
    count_ = connection_count_[connection_key]++;

    parseTcpUdp(ipv6);

    std::string service_key = dst_ip_ + "-" + service_;
    srv_count_ = service_count_[service_key]++;
    
    return toJSON();
}


std::string PacketParser::toJSON() {
    json j;
    j["duration"] = duration_;
    j["protocol_type"] = protocol_type_;
    j["service"] = service_;
    j["flag"] = flag_;
    j["src_bytes"] = src_bytes_;
    j["count"] = count_;
    j["srv_count"] = srv_count_; 
    j["src_ip"] = src_ip_;
    j["dst_ip"] = dst_ip_;
    return j.dump();
}

std::string PacketParser::serviceName(int port) {
    switch(port) {
        case 80: return "http";
        case 443: return "https";
        case 20: return "ftp";
        case 22: return "ssh";
        case 23: return "telnet";
        case 25: return "smtp";
        case 110: return "pop3";
        case 123: return "ntp";
        case 143: return "imap";
        case 161: return "snmp";
        case 53: return "dns";
        case 67: return "bootstrap/dhcp";
        case 137: return "netbios";
        case 5353: return "multicast dns";
        case 5223: return "apn"; 
        default: 
            if(port >= 49152 && port <= 65535) {
                return "ephemeral - " + std::to_string(port);
            } else {
                return std::to_string(port);
            }
    }

}

std::string PacketParser::pduName(PDU::PDUType type) {
    static const std::unordered_map<PDU::PDUType, std::string> pdu_type_map = {
        { PDU::ETHERNET_II, "Ethernet II" },
        { PDU::IP, "IP" },
        { PDU::IPv6, "IPv6" },
        { PDU::ARP, "ARP" },
        { PDU::ICMP, "ICMP" },
        { PDU::ICMPv6, "ICMPv6" },
        { PDU::TCP, "TCP" },
        { PDU::UDP, "UDP" },
        { PDU::DNS, "DNS" },
        { PDU::DHCP, "DHCP" },
        { PDU::DHCPv6, "DHCPv6" },
        { PDU::EAPOL, "EAPOL" },
        { PDU::RAW, "RawPDU" }
    };

    auto it = pdu_type_map.find(type);
    if (it != pdu_type_map.end()) {
        return it->second;
    } else {
        return "Unknown";
    }

}