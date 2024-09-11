/**
 * @file spelldat.h
 *
 * Interface of all spell data.
 */
#ifndef __SPELLDAT_H__
#define __SPELLDAT_H__

DEVILUTION_BEGIN_NAMESPACE

#define SPELL_NA 0
/* Minimum level requirement of a book. */
#define BOOK_MIN 1
/* Minimum level requirement of a staff. */
#define STAFF_MIN 2
/* Minimum level requirement of a scroll. */
#define SCRL_MIN 1
/* Minimum level requirement of a rune. */
#define RUNE_MIN 1
/* The cooldown period of the rage skill. */
#define RAGE_COOLDOWN_TICK 1200

#define SPELL_MASK(sn)     ((uint64_t)1 << (sn - 1))

#ifdef HELLFIRE
static_assert((int)SPL_RUNESTONE + 1 == (int)NUM_SPELLS, "SPELL_RUNE expects ordered spell_id enum");
#define SPELL_RUNE(sn) (sn >= SPL_RUNEFIRE)
#else
#define SPELL_RUNE(sn) (FALSE)
#endif

extern const SpellData spelldata[NUM_SPELLS];

DEVILUTION_END_NAMESPACE

#endif /* __SPELLDAT_H__ */
