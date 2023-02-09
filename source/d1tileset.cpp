#include "d1tileset.h"

D1Tileset::D1Tileset(D1Gfx *g)
    : gfx(g)
{
    this->min = new D1Min();
    this->til = new D1Til();
    this->sol = new D1Sol();
    this->amp = new D1Amp();
    this->tmi = new D1Tmi();
}

D1Tileset::~D1Tileset()
{
    delete min;
    delete til;
    delete sol;
    delete amp;
    delete tmi;
}

void D1Tileset::save(const SaveAsParam &params)
{
    this->min->save(params);
    this->til->save(params);
    this->sol->save(params);
    this->amp->save(params);
    this->tmi->save(params);
}
