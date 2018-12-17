/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  Project: PHCOMP - PSION HELP COMPILER    *  Written by: Nick Matthews  *
 *   Module: MAIN COMPILER SETUP FUNCTIONS   *  Date Started: 13 Aug 1996  *
 *     File: PHCOMP.C      Type: C SOURCE    *  Date Revised: 13 Aug 1996  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

 Revision History.
 13aug96  D1.00  Compiler completed for keywords Page, Topic and End.
 26aug96  D1.10  Include keyword and -L option added.
*/

#define VERSION   "D1.10"

const char* Title = "Psion Help Compiler  Version " VERSION "\n"
                    "Copyright (c) 1996 Nick Matthews\n\n" ;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

const char* Version = VERSION;

#define TRUE  1
#define FALSE 0

int Trim;
int Verbose;
int Silent;

FILE* OutFP;   

#define MAX_INC_ARRAY   10
/*char IncPath[ _MAX_PATH+1 ];*/
char* IncArray[ MAX_INC_ARRAY ];
int NumIncPath;

#define MAX_ERRNRS  10
/*char InFileName[ _MAX_PATH+1 ];*/
int CurLine;
char* CurFile;

int InPage;
int HasTopics;
int HasText;

#define ID_LEN      31
#define MAX_TOPICS  50

char TopicID[MAX_TOPICS][ID_LEN+1];
int NumTopics;

#define INBUF_SIZE   250
char InBuf[ INBUF_SIZE ];
char PageID[ID_LEN+1];

#define MAX_ERRORS  10
int ExitValue;

static void DoUsage( void );
static FILE* OpenOutFile( char* outfile, const char* infile );

void  Error( const char* message );
char* DoLeftTrim( char* sz );
char* DoRightTrim( char* sz );
int   IsBlankLine( const char* sz );
FILE* OpenIncFile( const char* name );
void  CompileFile( const char* filename, int incflag );
void  StartPage( char* id, char* text );
void  AddTopic( char* id );
void  AddText( void );
void  EndPage( void );

/*#*************************************************************************
 **  main
 **  ~~~~
 */

int main( int argn, char** argv )
{
    int i;
    static char outfile[ _MAX_PATH+1 ];
    static char infile[ _LAX_PATH+1 ];
    char* path;
    char* dot;

    for( i = 1 ; i < argn ; i++ )
    {
        if( argv[i][0] == '/' || argv[i][0] == '-' )
        {
            switch( argv[i][1] )
            {
            case 'L': case 'l':
                Trim = TRUE;
                break;
            case 'V': case 'v':
                Verbose = TRUE;
                break;
            case 'S': case 's':
                Silent = TRUE;
                break;
            case 'O': case 'o':
                if( argv[i][2] == '\0' ) 
                    break;
                strncpy( outfile, &argv[i][2], _MAX_PATH );
                break;
            case 'I': case 'i':
                if( argv[i][2] == '\0' )  break;
                IncArray[0] = &argv[i][2];
                NumIncPath++;
                path = strtok( &argv[1][2], ";" );
                while( path != NULL && NumIncPath != MAX_INC_ARRAY )
                {
                    IncArray[NumIncPath++] = path;
                    path = strtok( NULL, ";" );
                }
                break;
            default:
                printf( "\nInvalid Command line switch %c\n", argv[i][1] );
                return 2;
            }
        }
        else
        {
            strncpy( infile, argv[i], _MAX_PATH );
            if( strchr( infile, '.' ) == NULL )
            {
                strncat( infile, ".rht", _MAX_PATH );
            }
        }
    }
    if(infile[0] == '\0' )
    {
        DoUsage();
    }
    if( ! Silent ) printf( Title );
    if( outfile[0] == '\0' )
    {
        strcpy( outfile, infile );
        dot = strchr( outfile, '.' );
        if( dot != NULL ) 
        {
            *dot = '\0';
        }
    }
    OutFP = OpenOutFile( outfile, infile );
    InPage = FALSE;
    CompileFile( infile, FALSE );
    if( InPage )
    {
        printf( "Error: Missing #End\n" );
        exit( 2 );
    }
    printf( "\n" );
    return ExitValue;
}

/*#*************************************************************************
 **  DoUsage  Display usage if no input file given. Exits program without
 **  ~~~~~~~  error.
 */

static void DoUsage( void )
{
    printf( "%s%s", Title,
        " Usage:-\n"
        "PHCOMP filename[.RHT] [-Ofilename[.RHI]] [-Ipath] [-L] [-S] [-V]\n"
        "\n"
        "filename  Input File Name. The default extension is .RHT\n"
        "-O  Output filename. The default is the input filename.\n"
        "    The default extension is .RHI.\n"
        "-I  Path(s) to search for include files\n"
        "-L  Left trim whitespace from text\n"
        "-S  Silent, suppress startup banner\n"
        "-V  Verbose, outputs compiler progress to stdout.\n"
        "\n"
        "   Case or order for command line switches is not significant\n"
        "   and either \"-\" or \"/\" may be used.\n"
        "\n"
    );
    exit( 0 );
}

/***************************************************************************
 **  OpenOutFile  Open output file and write header. Returns file pointer
 **  ~~~~~~~~~~~  or exits if error.
 */
 
static FILE* OpenOutFile( char* filename, const char* infile )
{
    FILE* fp;
    
    if( strchr( filename, '.' ) == NULL )
    {
        strncat( filename, ".rhi", _MAX_PATH );
    }
    fp = fopen( filename, "w" );
    if( fp == NULL )
    {
        printf( "Error: Unable to open output file \"%s\"\n", filename );
        exit( 2 );
    }
    fprintf( fp,
        "/* %s */\n"
        "/* Created by PHCOMP Version %s from %s */\n\n",
        filename, Version, infile
    );
    return fp;
}

/*#*********************************************************************
 **  DoLeftTrim  Strip all leading whitespace. Returns sz.
 **  ~~~~~~~~~~
 */

char* DoLeftTrim( char* sz )
{
    char* p = sz;
    char* r = sz;

    while( isspace( *p++ ) );
    if( --p != r )
    {
        while( ( *r++ = *p++ ) );
    }
    return sz;
}

/***********************************************************************
 **  DoRightTrim  Strip all trailing whitespace. Returns sz.
 **  ~~~~~~~~~~~
 */

char* DoRightTrim( char* sz )
{
    char* p ;

    p = sz + strlen( sz ) - 1;
    while( isspace( *p ) && p >= sz )
    {
        *p = '\0';
        --p;
    }
    return sz;
}

/***************************************************************************
 **  IsBlankLine  Returns TRUE if sz is a blank line, otherwise FALSE.
 **  ~~~~~~~~~~~
 */

int IsBlankLine( const char* sz )
{
    while( *sz )
    {
        if( ! isspace( *sz ) )
        {
            return FALSE;
        }
        sz++;
    }
    return TRUE;
}

/***************************************************************************
 **  Error  Write the current input file name and line to stdout with error
 **  ~~~~~  message. Uses the same format as TSC error output. If the number
 **  of errors exceed MAX_ERRORS the program exits with errorlevel 1.
 */

void Error( const char* message )
{
    static count;
    
    printf( "(%s %d,1) ERROR: %s\n", CurFile, CurLine, message );
    count++;
    if( count == MAX_ERRORS )
    {
        printf( "ERROR: Too many errors\n" );
        exit( 1 );
    }
    ExitValue = 1;
}

/***************************************************************************
 **  OpenIncFile  Open an include file, trying the current directory first
 **  ~~~~~~~~~~~  and the going through the include paths.
 */

FILE* OpenIncFile( const char* name )
{
   static char file[ _MAX_PATH+1 ];
   FILE* fp;
   int i = 0;

   fp = NULL;

   while( fp == NULL && i != NumIncPath )
   {
       strncpy( file, IncArray[i++], _MAX_DRIVE+_MAX_DIR-1 );
       if( file[ strlen(file)-1 ] != '\\' )
       {
           strcat( file, "\\" );
       }
       strncat( file, name, _MAX_EXT+_MAX_FNAME );
       fp = fopen( file, "r" );
   }
   return fp;
}

/***************************************************************************
 **  CompileFile  Open and read filename, sending copiled output to OutFP.
 **  ~~~~~~~~~~~
 */

void CompileFile( const char* filename, int incflag )
{
    FILE* ifp;
    char* keyword;
    char* word2;
    char* text;
    static char sep[] = " \t\n";
    int prevline;
    char infile[ _MAX_PATH+1 ];

    strncpy( infile, filename, _MAX_PATH );
    ifp = fopen( filename, "r" );
    if( ifp == NULL && incflag == TRUE )
    {
        ifp = OpenIncFile( filename );
    }
    if( ifp == NULL )
    {
        if( incflag == TRUE )
        {
            Error( "Unable to open Include file" );
            return;
        }
        printf( "Error: Unable to open input file \"%s\"\n", infile );
        exit( 2 );
    }
    if( Verbose )
    {
        printf( "Reading \"%s\"\n", infile );
    }
    CurFile = infile;
    CurLine = 0;
    for(;;)
    {
        if( fgets( InBuf, INBUF_SIZE, ifp ) == NULL )
        {
            if( feof( ifp ) )
            {
                fclose( ifp );
                return;
            }
            else
            {
                printf( "Error: Reading input file \"%s\"\n", infile );
                exit( 2 );
            }
        }
        CurLine++;
        InBuf[ strlen(InBuf)-1 ] = '\0';
        if( InBuf[0] == '#' )
        {
            if( InBuf[1] == '\0' || isspace( InBuf[1] ) )
            {
                continue;
            }
            
            keyword = strtok( &InBuf[1], sep );
            word2 = strtok( NULL, sep );
            if( stricmp( keyword, "page" ) == 0 )
            {
                if( word2 == NULL )
                {
                    Error( "No ID for Page" );
                    continue;
                }
                text = strtok( NULL, "\t\n" );
                if( text == NULL )
                {
                    Error( "No Title for Page" );
                    continue;
                }
                StartPage( word2, text );
            }
            else if( stricmp( keyword, "topic" ) == 0 )
            {
                if( word2 == NULL )
                {
                    Error( "No ID for Topic" );
                    continue;
                }
                AddTopic( word2 );
            }
            else if( stricmp( keyword, "end" ) == 0 )
            {
                EndPage();
            }
            else if( stricmp( keyword, "include" ) == 0 )
            {
                if( word2 == NULL )
                {
                    Error( "No Filename with Include" );
                    continue;
                }
                prevline = CurLine;
                CompileFile( word2, TRUE );
                CurFile = infile;
                CurLine = prevline;
            }
            else
            {
                Error( "Unrecognised Keyword" );
            }
        }
        else if( InPage )
        {
            AddText();
        }
    }
}

/***************************************************************************
 **  St`rtPage  Process the start of the page (finishing off the previous
 **  ~~~~~~~~~  page, if any).
 */

void StartPage( char* id, char* title )
{
    DoLeftTrim( title );
    if( Verbose )
    {
        printf( "Compiling Page \"%s\"\n", title );
    }
    if( strlen( id ) > ID_LEN - 3 )
    {
        Error( "Page ID too long (trucated)" );
        id[ID_LEN-3] = '\0';
    }
    if( InPage == TRUE )
    {
        EndPage();
    }
    fprintf( OutFP, "RESOURCE HELP_ARRAY %s\n", id );
    fprintf( OutFP, "{\n" );
    fprintf( OutFP, "  topic = \"%s\";\n", title );
    strcpy( PageID, id );
    InPage = TRUE;
    HasTopics = FALSE;
    HasText = FALSE;
}

/***************************************************************************
 **  AddTopic  Add a topic to page and record it for later topic_array.
 **  ~~~~~~~~
 */

void AddTopic( char* id )
{
    if( HasText )
    {
        Error( "Topics must come before Text" );
        return;
    }
    if( ! HasTopics )
    {
        fprintf( OutFP, "  topic_id = %s__i;\n", PageID );
        HasTopics = TRUE;
        NumTopics = 0;
    }
    if( NumTopics == MAX_TOPICS )
    {
        Error( "Max number of Topics reached" );
        return;
    }
    strcpy( TopicID[ NumTopics++ ], id );
}

/***************************************************************************
 **  AddText  Write out the help text to output file.
 **  ~~~~~~~
 */

void AddText( void )
{
    static int prevblank;
    
    if( Trim )
    {
        DoLeftTrim( InBuf );
    }
    if( ! HasText )
    {
        if( IsBlankLine( InBuf ) )
        {
            return;  /* Ignore leading blank lines */
        }
        fprintf( OutFP, "  strlst =\n" );
        fprintf( OutFP, "  {\n" );
        fprintf( OutFP, "    STRING { str=\"%s\"; }", InBuf );
        HasText = TRUE;
        prevblank = FALSE;
    }
    else
    {
        if( IsBlankLine( InBuf ) )
        {                        /* Keep a note in case it's not the last *.
            prevblank = TRUE;    /* This will also remove multiple blanks */
            return;           
        }                     
        if( prevblank == TRUE )
        {
            fprintf( OutFP, ",\n    STRING { str=\"\"; }" );
            prevblank = FALSE;
        }
        fprintf( OutFP, ",\n    STRING { str=\"%s\"; }", InBuf );
    }
}

/***************************************************************************
 **  EndPage  Finish writing page and write topic_array if necessary.
 **  ~~~~~~~
 */

void EndPage( void )
{
    int i;
    
    if( ! InPage )
    {
        Error( "#End without #Page" );
        return;
    }
    if( HasText )
    {
        fprintf( OutFP, "\n  };\n" );
    }
    fprintf( OutFP, "}\n\n" );
    if( HasTopics )
    {
        /* write topic struct */
        fprintf( OutFP, "RESOURCE TOPIC_ARRAY %s__i\n", PageID );
        fprintf( OutFP, "{\n" );
        fprintf( OutFP, "  id_lst =\n" );
        fprintf( OutFP, "  {\n" );
        fprintf( OutFP, "    %s", TopicID[0] );
        for( i = 1 ; i < NumTopics ; i++ )
        {
            fprintf( OutFP, ",\n    %s", TopicID[i] );
        }
        fprintf( OutFP, "\n  };\n" );
        fprintf( OutFP, "}\n\n" );
        HasTopics = FALSE;
    }
    InPage = FALSE;
}

/* End of PHCOMP.C file */
