// Copyright (C) 2018 David Helkowski
// License GNU AGPLv3

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<libgen.h>
#include<stdint.h>
#include<sys/types.h>
#include<sys/stat.h>

// Text to print before every hexidecimal byte representation
#define HEX_PREFIX '~'

// Characters to show normally instead of in hex
#define OK_CHARACTERS " {}\"'=-()<>,./_![]:&;?%\n"

// Comment this out to disable acceptance of alpha-numeric characters
#define ALPHA_NUM
#ifdef ALPHA_NUM
    #define OK_CHARS OK_CHARACTERS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
#else
    #define OK_CHARS
#endif

// Minimum number of sequential acceptable character needed to show text instead of hex
#define MIN_SEQUENCE 3

#define bool char

FILE *data_handle;
uint32_t get_file_blocksize( FILE *fh ) {
    int fno = fileno( data_handle );
    struct stat fi;
    fstat(fno, &fi);
    if( !fi.st_blksize ) {
        fprintf(stderr, "File blocksize unclear; defaulting to 512\n" );
    }
    return fi.st_blksize || 512;
}

uint32_t blocksize;
char *buffer;

uint32_t bufferpos = 0;
uint32_t buffered = 0;
bool get_byte( uint8_t *byte ) {
    if( !buffered ) {
        size_t bytes_read = fread( buffer, blocksize, 1, data_handle );
        if( !bytes_read ) return 0;
        buffered = bytes_read;
        bufferpos = 0;
    }
    *byte = buffer[ bufferpos++ ];
    buffered--;
    return 1;
}

int main( int argc, char *argv[] ) {
    data_handle = 0;
    bool showhelp = 0;
    if( argc < 2 ) {
        if( isatty( STDIN_FILENO ) )
            showhelp = 1;
        else {
            fprintf(stderr, "Reading input from redirection\n" );
            data_handle = stdin;
        }
    }
    
    if( argc > 1 ) {
        if( !strncmp( argv[1], "--help", 6 ) ) showhelp = 1;
        fprintf(stderr, "Using file '%s' for input\n", argv[1] );
        data_handle = fopen( argv[1], "r" );
        if( !data_handle ) {
            fprintf(stderr, "File does not exist\n" );
            return 1;
        }
    }
    
    if( showhelp ) {
        char *base = basename( argv[0] );
        printf("=== Text Hack ===\nTakes input data and outputs specific characters as regular text and hex for other characters.\n\n");
        
        printf("Characters configured to be output normally:\n");
        #ifdef ALPHA_NUM
        printf("  Alphanumerics\n" );
        #endif
        printf("  %s\n\n", OK_CHARACTERS );
        
        printf("Usage:\n  %s [filename]\n  %s < filename\n", base, base );
        
        return 0;
    }
        
    if( !data_handle ) return 1;
    
    blocksize = get_file_blocksize( data_handle );
    buffer = malloc( blocksize );
        
    #define STACK_SIZE MIN_SEQUENCE + 1
    char stack[ STACK_SIZE ];
    memset( stack, 0, STACK_SIZE );
    
    // Build accept charmap
    char accept[ 256 ];
    memset( accept, 0, 256 );
    char accept_these[] = OK_CHARS;
    for( unsigned int i=0; i < ( sizeof( accept_these ) - 1 ); i++ ) {
        char let = accept_these[ i ];
        accept[ let ] = 1;
    }
    
    uint8_t prev_ok = 0;
    
    int stack_fill = 0; // how much of the stack is filled
    bool stack_full = 0;
    uint8_t byte;
    while( 1 ) {
        if( !get_byte( &byte ) ) break;
        
        if( !stack_full ) {
            if( ++stack_fill == MIN_SEQUENCE ) stack_full = 1;
        }
        
        // shift out the lowest char
        uint8_t n_ago = stack[ 0 ];
        
        // shift off the beginning of the stack
        for( int i=0;i<(MIN_SEQUENCE-1);i++ ) 
            stack[ i ] = stack[ i + 1 ];
        
        // "push" the new character onto the end of the stack
        stack[ MIN_SEQUENCE - 1 ] = byte;
        
        char ok_in_stack = 0;
        if( accept[ n_ago ] ) {
            ok_in_stack++;
            for( int i=1;i<MIN_SEQUENCE;i++ ) {
                if( accept[ stack[ i ] ] ) ok_in_stack++;
                else break;
            }
        }
        else
            prev_ok = 0;
        
        if( ( ok_in_stack + prev_ok ) >= MIN_SEQUENCE ) {
            putchar( n_ago );
            prev_ok++;
        }
        else {
            printf( "%c%02X", HEX_PREFIX, n_ago );
            prev_ok = 0;
        }
    }
    for( int i=0;i<stack_fill;i++ )
        putchar( stack[ i ] );

    fclose( data_handle );
    if( buffer ) free( buffer );
}