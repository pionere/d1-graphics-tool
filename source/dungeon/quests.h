/**
 * @file quests.cpp
 *
 * Interface of functionality for handling quests.
 */
#ifndef __QUESTS_H__
#define __QUESTS_H__

extern QuestStruct quests[NUM_QUESTS];

#define LEVEL_MASK(x)   (x < 32 ? (uint32_t)1 << (x) : 0)

void InitQuests(int seed);
bool QuestStatus(int qn);

#endif /* __QUESTS_H__ */
