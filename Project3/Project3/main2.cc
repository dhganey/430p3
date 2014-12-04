//#include	<stdio.h>
//#include	<string.h>
//#include	<sys/stat.h>
//#include	<sys/types.h>
//#include	<unistd.h>
//#include	<errno.h>
//#include	<fcntl.h>
//
//#include	"LibFS.h"
//#include	"LibDisk.h"
//
//extern Disk_Error_t diskErrno;
//
//void
//usage( char *image )
//{
//	fprintf( stderr, "usage: %s <disk image file>\\n", image );
//	exit( 1 );
//}
//
//int
//main( int argc, char *argv[] )
//{
//	int rc, file_ptr, fd, len;
//	ssize_t count;
//	struct stat buf;
//	char file_name[ 16 ], dir_name[256], actual_path[ 256 ], path[ 256 ], buffer[ SECTOR_SIZE ], s[ 32 ];
//    
//	if( argc != 2 )
//	{
//		usage( argv[ 0 ] );
//	}
//    
//	char *image = argv[ 1 ];
//    
//	rc = stat( image, &buf );
//
//	if( ( rc == -1 ) && ( errno == ENOENT ) )
//	{
//		// The named file does not exist, create the file system.
//        
//		printf( "The image %s does not exist, initialize it...\n", image );
//
//		if( ( rc = FS_Boot( image ) ) == -1 )
//		{
//			printf( "Error initializing file system...\n" );
//			printf( "\tdiskErrno = %d\n", diskErrno );
//			osErrno = E_GENERAL;
//			exit( -1 );
//		}
//	}
//	else
//	{
//		if( ( rc = FS_Boot( image ) ) == -1 )
//		{
//			printf( "Error loading file system from disk image...\n" );
//			printf( "\tdiskErrno = %d\n", diskErrno );
//			osErrno = E_GENERAL;
//			exit( -1 );
//		}
//	}
//    
//	// File system created or restored from the file image; synchronize it before working with the FS.
//    
//	if( ( rc = FS_Sync() ) == -1 )
//	{
//		printf( "Error synch-ing file systems to disk...\n" );
//		printf( "\tdiskErrno = %d\n", diskErrno );
//		osErrno = E_GENERAL;
//		exit( -1 );
//	}
//    
//	// Create some directories
//
//	strcpy( dir_name, "/dir1" );
//
//	if( ( rc = Dir_Create( dir_name ) ) == -1 )
//	{
//		printf( "Error creating directory %s...\n", dir_name );
//		printf( "\tosErrno = %d\n", osErrno );
//		exit( -1 );
//	}
//	
//	strcpy( dir_name, "/dir2" );
//	if( ( rc = Dir_Create( dir_name ) ) == -1 )
//	{
//		printf( "Error creating directory %s...\n", dir_name );
//		printf( "\tosErrno = %d\n", osErrno );
//		exit( -1 );
//	}
//
//	strcpy( path, "/dir1/one.txt" );
//
//	// This file creation should succeed.
//
//	rc = File_Create( path );
//	if( rc == 0 )
//		printf( "Creation of file %s is successful.\n", path ); 
//	else
//		if( osErrno == E_CREATE )
//			printf( "Creation of file %s is not successful; file already exists.", path ); 
//
//	// This one should fail because we already created /dir1/one.txt.
//
//	rc = File_Create( path );
//	if( rc == 0 )
//		printf( "Creation of file %s is successful.\n", path ); 
//	else
//		if( osErrno == E_CREATE )
//			printf( "Creation of file %s is not successful; file already exists.\n", path ); 
//
//	
//	// Try opening a file that doesn't exist.
//
//	strcpy( path, "/x/x");
//	rc = File_Open( path );
//
//	if( rc == 0 )
//		printf( "File %s is opened successfully.\n", path ); 
//	else
//		if( osErrno == E_NO_SUCH_FILE )
//			printf( "Attempted to open file %s; file does not exist.\n", path ); 
//
//
//	// Try writing to a file that is not open.
//
//	fd = -1;
//	strcpy( s, "hello" );
//	len = strlen( s );
//	rc = File_Write( fd, s, len );
//
//	if( rc == 0 )
//		printf( "Write of string %s to file with file descriptor %d is successful.\n", "hello", fd ); 
//	else
//		if( osErrno == E_BAD_FD )
//			printf( "The file with file descriptor %d is not open; cannot write.\n", fd ); 
//
//	// Try to close a file that is not open.
//
//	rc = File_Close( fd );
//
//	if( rc == 0 )
//		printf( "Close of file with file descriptor %d is successful.\n", fd ); 
//	else
//		if( osErrno == E_BAD_FD )
//			printf( "Close of file with file descriptor %d is not successful; file not open.\n", fd ); 
//
//	// Synchronize file system before exiting.
//    
//	if( ( rc = FS_Sync() ) == -1 )
//	{
//		printf( "Error synch-ing file systems to disk...\n" );
//		printf( "\tdiskErrno = %d\n", diskErrno );
//		osErrno = E_GENERAL;
//		exit( -1 );
//	}
//    
//	return( 0 );
//}
