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
BOOL TCPEcho( char* lpszHost, char* lpszPort );

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
        printf( "Usage: tcpEchoClient [host] [port]\n" );
        printf( "ex: tcpEchoClient 127.0.0.1 5226\n" );
        exit( 1 );
    }

    TCPEcho( pszHost, pszPort );
    
    return 0;
}

/// @brief 
/// @param lpszHost 
/// @param lpszPort 
/// @return 
BOOL TCPEcho( char* lpszHost, char* lpszPort )
{
    if( !lpszHost || !lpszPort )
        return FALSE;

    int s = 0;
    struct sockaddr_in sin = { 0 };

    //
    sin.sin_family      = AF_INET;
    sin.sin_port        = htons( atoi( lpszPort ) );
    sin.sin_addr.s_addr = inet_addr( lpszHost );

    //
    s = socket( PF_INET, SOCK_STREAM, 0 );
    if( s < 0 )
    {
        printf( "Create socket failed! ( Err: '%s' )\n", strerror(errno) );
        exit( 1 );
    }

    if( connect( s, (struct sockaddr*)&sin, sizeof( sin ) ) < 0 )
    {
        printf( "Connect failed! ( Err: '%s' )\n", strerror(errno) );
        exit( 1 );
    }

    printf( "%s:%s connected\n", lpszHost, lpszPort );

    //
    int iLen = 0, iStart = 0, iOut = 0;
    char szBuf[ LEN_BUF ] = { 0 };

    while( fgets( szBuf, sizeof( szBuf ), stdin ) )
    {
        // send to server
        szBuf[ LEN_BUF ] = '\0';
        iOut = strlen( szBuf ) - 1;
        
        send( s, szBuf, iOut, 0 );

        // recv from server
        memset( szBuf, 0, sizeof( szBuf ) );

        for( iStart = 0; iStart < iOut; iStart += iLen )
        {
            iLen = recv( s, szBuf, sizeof( char ) * LEN_BUF, 0 );

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
