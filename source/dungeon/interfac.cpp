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

static void LogErrorF(const char* msg, ...)
{
	char tmp[256];
	/*snprintf(tmp, sizeof(tmp), "c:\\logdebug%d.txt", 0);
	FILE* f0 = fopen(tmp, "a+");
	if (f0 == NULL)
		return;*/

	va_list va;

	va_start(va, msg);

	vsnprintf(tmp, sizeof(tmp), msg, va);

	va_end(va);

	dProgress() << QString(tmp);
	/*fputs(tmp, f0);

	fputc('\n', f0);

	fclose(f0);*/
}
