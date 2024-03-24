#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

//
#define TRUE    1
#define FALSE   0
#define LEN_BUF 2048

//
typedef int BOOL;

//
BOOL UDPEcho( char* lpszHost, char* lpszPort );

int main( int argc, char* argv[] )
{
    char* pszHost = NULL;
    char* pszPort = NULL;

    switch (argc)
    {
    case 3:
        {
            pszHost = argv[ 1 ];
            pszPort = argv[ 2 ];
        }
        break;
    
    default:
        printf( "Usage: udpEchoClient [host] [port]\n" );
        printf( "ex: udpEchoClient 127.0.0.1 5227\n" );
        exit( 1 );
    }

    UDPEcho( pszHost, pszPort );
    
    return 0;
}

/// @brief 
/// @param lpszHost 
/// @param lpszPort 
/// @return 
BOOL UDPEcho( char* lpszHost, char* lpszPort )
{
    if( !lpszHost || !lpszPort )
        return FALSE;

    int s = -1, iAddrLen = 0;
    struct sockaddr_in sout = { 0 };

    // Set output socket
    sout.sin_family      = AF_INET;
    sout.sin_port        = htons( atoi( lpszPort ) );
    sout.sin_addr.s_addr = inet_addr( lpszHost );

    s = socket( PF_INET, SOCK_DGRAM, 0 );
    if( s < 0 )
    {
        printf( "Create output socket failed! ( Err: '%s' )\n", strerror(errno) );
        exit( 1 );
    }

    //
    int iLen = 0, iStart = 0, iOut = 0;
    char szBuf[ LEN_BUF ] = { 0 };

    while( fgets( szBuf, sizeof( szBuf ), stdin ) )
    {
        // send to server
        szBuf[ LEN_BUF ] = '\0';
        iOut = strlen( szBuf ) - 1;

        if( sendto( s, szBuf, iOut, 0, (struct sockaddr*)&sout, sizeof( sout ) ) < 0 )
        {
            printf( "sendto failed! ( Err: '%s' )\n", strerror(errno) );
            exit( 1 );
        }

        // recv from server
        memset( szBuf, 0, sizeof( szBuf ) );

        for( iStart = 0; iStart < iOut; iStart += iLen )
        {
            iLen = recvfrom( s, szBuf + iStart, ( sizeof( char ) * LEN_BUF ), 0, (struct sockaddr*)&sout, (socklen_t*)&iAddrLen );

            if( iLen < 0 )
            {
                printf( "recv failed: %s\n", strerror( errno ) );
                exit( 1 );
            }
        }

        szBuf[ iOut ] = '\0';
        printf( "Echo from sevrer: '%s'\n", szBuf );
    }

    return TRUE;
}
