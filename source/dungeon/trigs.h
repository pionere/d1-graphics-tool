/**
 * @file trigs.h
 *
 * Interface of functionality for triggering events when the player enters an area.
 */
#ifndef __TRIGS_H__
#define __TRIGS_H__

extern int numtrigs;
extern TriggerStruct trigs[MAXTRIGGERS];

void InitView(int entry);
void InitTriggers();

#endif /* __TRIGS_H__ */
