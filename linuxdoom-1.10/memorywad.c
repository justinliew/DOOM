#include "doom1.wad.h"

unsigned char*
D_GetInMemoryWad()
{
	return doom1_wad;
}

char* web =
#include "index.html"
;

char*
D_GetIndex()
{
	return web;
}