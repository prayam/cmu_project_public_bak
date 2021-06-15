#ifndef PBOX_H
#define PBOX_H
#include <stdlib.h>
#include <iostream>
#include <glib.h>

using namespace std;
#define mydataFmt float


struct pBox
{
	mydataFmt *pdata;
	gint width;
	gint height;
	gint channel;
};
struct Bbox
{
	float score;
	gint x1;
	gint y1;
	gint x2;
	gint y2;
	float area;
	gboolean exist;
	mydataFmt ppoint[10];
	mydataFmt regreCoord[4];
};

struct orderScore
{
	mydataFmt score;
	gint oriOrder;
};
#endif
