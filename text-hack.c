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

typedef struct {
    FILE *data_handle;
    uint32_t blocksize;
    char *buffer;
    bool needclose;
    uint32_t bufferpos;
    uint32_t buffered;
} data_c;

data_c *data_c__new() {
    data_c *self = malloc( sizeof( data_c ) );
    memset( self, 0, sizeof( data_c ) );
    return self;
}

void data_c__delete( data_c *self ) {
    if( self->needclose ) {
        fclose( self->data_handle );
    }
    if( self->buffer ) {
        free( self->buffer );
    }
    free( self );
}

void data_c__read_blocksize( data_c *self ) {
    int fno = fileno( self->data_handle );
    struct stat fi;
    fstat(fno, &fi);
    if( !fi.st_blksize ) {
        fprintf(stderr, "File blocksize unclear; defaulting to 512\n" );
    }
    self->blocksize = fi.st_blksize || 512;
}

bool data_c__open_file( data_c *self, char *filename ) {
    self->data_handle = fopen( filename, "r" );
    if( !self->data_handle ) {
        fprintf(stderr, "File does not exist\n" );
        return 1;
    }
    self->needclose = 1;
    data_c__read_blocksize( self );
    self->buffer = malloc( self->blocksize );
    return 0;
}

void data_c__use_fh( data_c *self, FILE *fh ) {
    self->data_handle = fh;
    data_c__read_blocksize( self );
    self->buffer = malloc( self->blocksize );
}

bool data_c__get_byte( data_c *self, uint8_t *byte ) {
    if( !self->buffered ) {
        size_t bytes_read = fread( self->buffer, self->blocksize, 1, self->data_handle );
        if( !bytes_read ) return 0;
        self->buffered = bytes_read;
        self->bufferpos = 0;
    }
    *byte = self->buffer[ self->bufferpos++ ];
    self->buffered--;
    return 1;
}

typedef struct {
    bool accept[ 256 ];
} byte_match_c;

byte_match_c *byte_match_c__new() {
    byte_match_c *self = malloc( sizeof( byte_match_c ) );
    memset( self, 0, sizeof( byte_match_c ) );
    return self;
}

void byte_match_c__delete( byte_match_c *self ) {
    free( self );
}

void byte_match_c__accept_chars_w_len( byte_match_c *self, char *accept_these, int len ) {
    //char accept_these[] = OK_CHARS;
    for( unsigned int i=0; i < len; i++ ) {
        char let = accept_these[ i ];
        self->accept[ let ] = 1;
    }
}

bool byte_match_c__accept( byte_match_c *self, uint8_t byte ) {
    return self->accept[ byte ];
}

bool data_c__source_valid( data_c *self ) {
    return self->data_handle ? 1 : 0;
}

int main( int argc, char *argv[] ) {
    data_c *data_ob = data_c__new();
    
    bool showhelp = 0;
    if( argc < 2 ) {
        if( isatty( STDIN_FILENO ) )
            showhelp = 1;
        else {
            fprintf(stderr, "Reading input from redirection\n" );
            data_c__use_fh( data_ob, stdin );
        }
    }
    
    if( argc > 1 ) {
        if( !strncmp( argv[1], "--help", 6 ) ) showhelp = 1;
        fprintf(stderr, "Using file '%s' for input\n", argv[1] );
        bool err = data_c__open_file( data_ob, argv[1] );
        if( err ) {
            data_c__delete( data_ob );
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
        
        data_c__delete( data_ob );
        return 0;
    }
        
    if( !data_c__source_valid( data_ob ) ) {
        data_c__delete( data_ob );
        return 1;
    }
    
    #define STACK_SIZE MIN_SEQUENCE + 1
    char stack[ STACK_SIZE ];
    memset( stack, 0, STACK_SIZE );
    
    byte_match_c *byte_match_ob = byte_match_c__new();
    byte_match_c__accept_chars_w_len( byte_match_ob, OK_CHARS, sizeof( OK_CHARS ) - 1 );
        
    uint8_t prev_ok = 0;
    
    int stack_fill = 0; // how much of the stack is filled
    bool stack_full = 0;
    uint8_t byte;
    while( 1 ) {
        if( !data_c__get_byte( data_ob, &byte ) ) break;
        
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
        if( byte_match_c__accept( byte_match_ob, n_ago ) ) {
            ok_in_stack++;
            for( int i=1;i<MIN_SEQUENCE;i++ ) {
                if( byte_match_c__accept( byte_match_ob, stack[ i ] ) ) ok_in_stack++;
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

    byte_match_c__delete( byte_match_ob );
    data_c__delete( data_ob );
    
    return 0;
}