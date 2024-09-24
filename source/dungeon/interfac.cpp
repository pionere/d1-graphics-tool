/**
 * @file interfac.cpp
 *
 * Implementation of load screens.
 */
#include "all.h"

#include <QApplication>
#include <QString>

#include "../progressdialog.h"

bool IsMultiGame;
bool IsHellfireGame;
uint8_t gbGameLogicProgress;
int gnTicksRate = SPEED_NORMAL;
QString assetPath;
char infostr[256];
char tempstr[256];

void LogErrorF(const char* msg, ...)
{
	char tmp[256];
	char tmsg[256];
	va_list va;

	va_start(va, msg);

	vsnprintf(tmsg, sizeof(tmsg), msg, va);

	va_end(va);

	// dProgressErr() << QString(tmsg);
	
	snprintf(tmp, sizeof(tmp), "c:\\logdebug%d.txt", 0);
	FILE* f0 = fopen(tmp, "a+");
	if (f0 == NULL)
		return;

	fputs(tmsg, f0);

	fputc('\n', f0);

	fclose(f0);
}
