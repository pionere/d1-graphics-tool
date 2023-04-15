/**
 * @file objects.h
 *
 * Interface of object functionality, interaction, spawning, loading, etc.
 */
#ifndef __OBJECTS_H__
#define __OBJECTS_H__

extern int numobjects;
extern ObjectStruct objects[MAXOBJECTS];

void InitObjectGFX();
void FreeObjectGFX();
void InitLevelObjects();
void InitObjects();
int AddObject(int type, int ox, int oy);
// void ObjChangeMap(int x1, int y1, int x2, int y2 /*, bool hasNewObjPiece*/);
void GetObjectStr(int oi);

#endif /* __OBJECTS_H__ */
