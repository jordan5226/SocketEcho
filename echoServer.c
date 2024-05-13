#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/select.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define LEN_BUF 2048
#define	NOSOCK	-1

enum PROTO
{
    PROTO_UDP = 0,
    PROTO_TCP,
};

struct ConnInfo
{
    enum PROTO eProtocal;
    int sock;
    struct sockaddr_in sin;
};

int InitServer( enum PROTO eProtocal, int iPort, int iConnNum );
int InitTCPServer( int iPort, int iConnNum );
int InitUDPServer( int iPort, int iConnNum );
void RunService( struct ConnInfo* lpConnInfo );
void TCPEcho( struct ConnInfo* lpConnInfo );
void UDPEcho( struct ConnInfo* lpConnInfo );

int main( int argc, char* argv[] )
{
    //
    int i = 0, nfds = 0, cs = NOSOCK, iAddrLen = 0;
    fd_set rfds;
    struct sockaddr_in sin = { 0 };
    struct ConnInfo arrConn[ NOFILE ];

    for( i = 0; i < NOFILE; ++i )
    {
        arrConn[ i ].eProtocal = PROTO_TCP;
        arrConn[ i ].sock      = NOSOCK;
    }

    //
    int tcps = InitTCPServer( 5226, ( NOFILE - 2 ) / 2 );
    arrConn[ tcps ].eProtocal = PROTO_TCP;
    arrConn[ tcps ].sock      = tcps;

    int udps = InitUDPServer( 5227, ( NOFILE - 2 ) / 2 );
    arrConn[ udps ].eProtocal = PROTO_UDP;
    arrConn[ udps ].sock      = udps;

    iAddrLen = sizeof( sin );

    while( 1 )
    {
        //
        FD_ZERO( &rfds );
        nfds = NOSOCK;

        // Add child sockets to read fd set
        for( i = 0; i < NOFILE; ++i )
        {
            if( arrConn[ i ].sock > 0 )
                FD_SET( arrConn[ i ].sock, &rfds );

            if( arrConn[ i ].sock > nfds )
                nfds = arrConn[ i ].sock;
        }

        // Select fd
        if( select( nfds + 1, &rfds, NULL, NULL, NULL ) < 0 )
        {
            printf( "select error! ( Err: '%s' )\n", strerror(errno) );
            exit( 1 );
        }

        // Accept
        if( FD_ISSET( tcps, &rfds ) )
        {
            if( ( cs = accept( tcps, (struct sockaddr*)&sin, (socklen_t*)&iAddrLen ) ) < 0 )
            {
                printf( "accept failed! ( Err: '%s' )\n", strerror(errno) );
                continue;
            }

            printf("New connection, fd: %d, IP:%s, Port:%d\n", cs, inet_ntoa( sin.sin_addr ), ntohs( sin.sin_port ) );

            arrConn[ cs ].eProtocal = PROTO_TCP;
            arrConn[ cs ].sock      = cs;
            arrConn[ cs ].sin       = sin;
        }

        // Handle packet
        for( i = 0; i < NOFILE; ++i )
        {
            if( i == tcps )
                continue;

            if( FD_ISSET( arrConn[ i ].sock, &rfds ) )
            {
                RunService( &arrConn[ i ] );
            }
        }
    }

    return 0;
}

///
///
///
int InitServer( enum PROTO eProtocal, int iPort, int iConnNum )
{
    int s = 0, iType = 0;
    struct sockaddr_in sin = { 0 };
    struct protoent* pPE = NULL;

    //
    sin.sin_family      = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port        = htons( iPort );

    //
    if( PROTO_UDP == eProtocal )
    {
        iType = SOCK_DGRAM;
        pPE = getprotobyname( "udp" );
    }
    else if( PROTO_TCP == eProtocal )
    {
        iType = SOCK_STREAM;
        pPE = getprotobyname( "tcp" );
    }

    if( !pPE )
    {
        printf( "Get protocal entry failed!\n" );
        exit( 1 );
    }

    //
    s = socket( PF_INET, iType, pPE->p_proto );
    if( s < 0 )
    {
        printf( "Create socket failed! ( Err: '%s' )\n", strerror(errno) );
        exit( 1 );
    }

    int opt = 1;
    if( setsockopt( s, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof( opt ) ) < 0 )
    {
        printf( "setsockopt failed! ( Err: '%s' )\n", strerror(errno) );
        exit( 1 );
    }

    if( bind( s, (struct sockaddr*)&sin, sizeof( sin ) ) < 0 )
    {
        printf( "Bind on port:%d failed! ( Err: '%s' )\n", iPort, strerror(errno) );
        exit( 1 );
    }

    if( ( iType == SOCK_STREAM ) && ( listen( s, iConnNum ) < 0 ) )
    {
        printf( "Listen on port:%d failed! ( Err: '%s' )\n", iPort, strerror(errno) );
        exit( 1 );
    }

    if( PROTO_UDP == eProtocal )
        printf( "Init UDP server on port: %d\n", iPort );
    else if( PROTO_TCP == eProtocal )
        printf( "Init TCP server on port: %d\n", iPort );

    return s;
}

int InitTCPServer( int iPort, int iConnNum )
{
    return InitServer( PROTO_TCP, iPort, iConnNum );
}

int InitUDPServer( int iPort, int iConnNum )
{
    return InitServer( PROTO_UDP, iPort, iConnNum );
}

///
///
///
void RunService( struct ConnInfo* lpConnInfo )
{
    if( !lpConnInfo )
        return;

    // switch( fork() )
    // {
    // case 0:
    //     break;

    // case -1:
    //     {
    //         printf( "fork error! ( Err: '%s' )\n", strerror(errno) );
    //     }
    //     exit( 1 );

    // default:
    //     {
    //         close( lpConnInfo->sock );
    //         lpConnInfo->sock = 0;
    //     }
    //     return;  // parent
    // }

    // Child
    if( PROTO_TCP == lpConnInfo->eProtocal )
    {
        TCPEcho( lpConnInfo );
    }
    else if( PROTO_UDP == lpConnInfo->eProtocal )
    {
        UDPEcho( lpConnInfo );
    }

    //exit( 0 );
}

///
///
///
void TCPEcho( struct ConnInfo* lpConnInfo )
{
    if( !lpConnInfo )
        return;

    int iLen = 0;
    char szBuf[ LEN_BUF ] = { 0 };

    if( ( iLen = recv( lpConnInfo->sock, szBuf, ( sizeof( char ) * LEN_BUF ), 0 ) ) <= 0 )
    {   // Disconnected
        printf( "Host disconnect, IP:%s, Port:%d\n", inet_ntoa( lpConnInfo->sin.sin_addr ), ntohs( lpConnInfo->sin.sin_port ) );

        //
        close( lpConnInfo->sock );
        lpConnInfo->sock = 0;

        return;
    }
    
    // Echo
    szBuf[ iLen ] = '\0';
    send( lpConnInfo->sock, szBuf, strlen( szBuf ), 0 );
}

///
///
///
void UDPEcho( struct ConnInfo* lpConnInfo )
{
    int iLen = 0, iAddrLen = sizeof( struct sockaddr );
    struct sockaddr_in sin = { 0 };
    char szBuf[ LEN_BUF ] = { 0 };

    iLen = recvfrom( lpConnInfo->sock, szBuf, ( sizeof( char ) * LEN_BUF ), 0, (struct sockaddr*)&sin, (socklen_t*)&iAddrLen );
    printf( "recvfrom IP:%s, Port:%d, Data:'%s'\n", inet_ntoa( sin.sin_addr ), ntohs( sin.sin_port ), szBuf );

    szBuf[ iLen ] = '\0';

    sendto( lpConnInfo->sock, szBuf, strlen( szBuf ), 0, (struct sockaddr*)&sin, iAddrLen );
}
