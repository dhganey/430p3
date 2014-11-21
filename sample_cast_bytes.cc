#include	<stdlib.h>
#include	<string.h>
#include	<stdio.h>

struct example
{
	// A structure that is 52 bytes in size.

	int i; 			// 4 bytes
	char c[6];	   // 6 bytes
	short s; 		// 2 bytes
	int a[10];		// 40 bytes
};

struct overlay
{
	// Another structure 52 bytes in size.

	char c[12];	// 12 bytes
	int i[10];	// 40 bytes
};
	

void print_example_struct( struct example *e )
{
	printf( "Printing contents of example struct:\n" );
	printf( "\ti = %d\n", e->i );
	printf( "\tc = %s\n", e->c );
	printf( "\ts = %d\n", e->s );
	for( int i = 0; i < 10; i++ )
		printf("\ta[%d] = %d\n", i, e->a[i] );
	return;
}

void print_overlay_struct( struct overlay *s )
{
	printf( "Printing contents of overlay struct:\n" );
	printf( "\tc = %s\n", s->c );
	for( int j = 0; j < 10; j ++ )
		printf("\ti[%d] = %d\n", j, s->i[j] );
	return;
}

int main()
{
	printf( "sizeof( struct example ) = %d\n", sizeof( struct example ) );
	printf( "sizeof( struct overlay ) = %d\n", sizeof( struct overlay ) );

	char *bytes = (char *) malloc( sizeof( struct example ) );	

	((struct example *) bytes)->i = 100;
	strcpy( ((struct example *) bytes)->c, "hi" );
	((struct example *) bytes)->s = 200;
	for( int i = 0; i < 10; i++ )
		((struct example *) bytes)->a[i] = i + i;

	print_example_struct( (struct example *) bytes );

	strcpy( ((struct overlay *) bytes)->c, "hello" );
	for( int j = 0; j < 10; j++ )
		((struct overlay *) bytes)->i[j] = j * 9;

	print_overlay_struct( (struct overlay *) bytes );

	free( (void *) bytes );
	exit( 1 );
}
