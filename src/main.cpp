#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include "packet_sniffer.h"




/**
 * @brief �������
 * ������ʾhttp�ٳּ���ԭ��������ָ��ԴIP��·������Ϊ25�����󣬿���ʵ������޸�
 *
 * ע���������ݰ�ȫ���������ڷǷ�֮Ŀ��
 */
int main( int argc, char **argv )
{
    if( argc < 4 )
    {
        printf( "Usage:%s eth0 type ip\n", basename(argv[0]) );
        printf( "       eth0 The nic will be collect\n" );
        printf( "       type How to collect traffic��1.rawsocket 2.libpcap\n" );
        printf( "       ip   Source IP\n" );

        return 1;
    }

    // �ɼ���
    PacketSniffer pSniffer;
    pSniffer.Start( argv[1], atoi(argv[2]), argv[3] );

    return 0;
}
