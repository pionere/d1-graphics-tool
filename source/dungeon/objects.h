/**
 * @file objects.h
 *
 * Interface of object functionality, interaction, spawning, loading, etc.
 */
#ifndef __OBJECTS_H__
#define __OBJECTS_H__

extern int numobjects;
extern ObjectStruct objects[MAXOBJECTS];

void InitLevelObjects();
void InitObjectGFX();
void FreeObjectGFX();
void InitObjects();
void SetMapObjects();
void SetObjMapRange(int oi, int x1, int y1, int x2, int y2, int v);
int AddObject(int type, int ox, int oy);
// void ObjChangeMap(int x1, int y1, int x2, int y2 /*, bool hasNewObjPiece*/);
void GetObjectStr(int oi);

#endif /* __OBJECTS_H__ */
