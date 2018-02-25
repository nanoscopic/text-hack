// Copyright (C) 2018 David Helkowski
// License GNU AGPLv3

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<libgen.h>

// Text to print before every hexidecimal byte representation
#define HEX_PREFIX '~'

// Characters to show normally instead of in hex
#define OK_CHARACTERS " {}\"'=-()<>,./_![]:&;?"

// Comment this out to disable acceptance of alpha-numeric characters
#define ALPHA_NUM
#ifdef ALPHA_NUM
    #define OK_CHARS OK_CHARACTERS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
#else
    #define OK_CHARS
#endif

// Minimum number of sequential acceptable character needed to show text instead of hex
#define MIN_SEQUENCE 3

int main( int argc, char *argv[] ) {
    FILE *data_handle = 0;
    bool showhelp = 0;
    if( argc < 2 ) {
        if( isatty( STDIN_FILENO ) ) {
            showhelp = 1;
        }
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
        
    if( !data_handle ) {
        return 1;
    }
    
    #define STACK_SIZE MIN_SEQUENCE + 1
    unsigned char stack[ STACK_SIZE ];
    memset( stack, 0, STACK_SIZE );
    
    // Build accept charmap
    unsigned char accept[ 256 ];
    memset( accept, 0, 256 );
    unsigned char accept_these[] = OK_CHARS;
    for( int i=0; i < sizeof( accept_these ); i++ ) {
        unsigned char let = accept_these[ i ];
        accept[ let ] = 1;
    }
    
    unsigned char ok_cnt = 0;
    
    int stack_fill = 0; // how much of the stack is filled
    bool stack_full = 0;
    while( 1 ) {
        unsigned char let = fgetc( data_handle );
        if( feof( data_handle ) ) break;
        
        if( !stack_full ) {
            stack_fill++;
            if( stack_fill == MIN_SEQUENCE ) {
                stack_full = 1;
            }
        }
        
        // shift out the lowest char
        unsigned char n_ago = stack[ 0 ];
        
        for( int i=0;i<(MIN_SEQUENCE-1);i++ ) {
            stack[ i ] = stack[ i + 1 ];
        }
        
        // push the new character onto the stack and pop the last one
        stack[ MIN_SEQUENCE - 1 ] = let;
        
        unsigned char num_ok = 0;
        if( accept[ n_ago ] ) {
            num_ok++;
            for( int i=0;i<=(MIN_SEQUENCE-1);i++ ) {
                if( accept[ stack[ i ] ] ) num_ok++;
                else i=100;
            }
        }
        else {
            ok_cnt = 0;
        }
        
        if( ( num_ok + ok_cnt ) >= MIN_SEQUENCE ) {
            putchar( n_ago );
            ok_cnt++;
        }
        else {
            printf( "%c%02X", HEX_PREFIX, n_ago );
            ok_cnt = 0;
        }
    }
    for( int i=0;i<stack_fill;i++ ) {
        putchar( stack[ i ] );
    }
    fclose( data_handle );
}