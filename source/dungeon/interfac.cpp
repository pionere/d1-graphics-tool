/**
 * @file interfac.cpp
 *
 * Implementation of load screens.
 */
#include "all.h"

#include <QApplication>
#include <QString>

#include "../progressdialog.h"

int ViewX;
int ViewY;
bool IsMultiGame;
bool IsHellfireGame;
bool HasTileset;
bool PatchDunFiles;
int ddLevelPlrs;
int dnLevel;
int dnType;
QString assetPath;
char infostr[256];
char tempstr[256];

typedef struct ObjStruct {
    int otype;
    int animFrame;
    QString name;

    bool operator==(const ObjStruct & oval) const {
        return otype == oval.otype && animFrame == oval.animFrame && name == oval.name;
    };
    bool operator!=(const ObjStruct & oval) const {
        return otype != oval.otype || animFrame != oval.animFrame || name != oval.name;
    };
} ObjStruct;

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
