/* Copyright (C) 2009-2010 by Niels Holst [niels.holst@agrsci.dk] and co-authors.
** Copyrights reserved.
** Released under the terms of the GNU General Public License version 3.0 or later.
** See www.gnu.org/copyleft/gpl.html.
*/
#include <usbase/pull_variable.h>
#include "assimilation_max_given_temp_sla.h"
#include "plant.h"

using namespace UniSim;

namespace intercom{

AssimilationMaxGivenTempSla::AssimilationMaxGivenTempSla(UniSim::Identifier name, QObject *parent)
	: Model(name, parent)
{
    new PullVariable("amax", &amax, this, "description");
}

void AssimilationMaxGivenTempSla::initialize()
{
    setParameter("pctN", &pctN, 0.040, "description");
    weather = seekOne<Model*>("weather");
    Model *plant = seekOneAscendant<Plant*>("*");
    specificLeafArea = plant->seekOneDescendant<Model*>("specificLeafArea");
}

void AssimilationMaxGivenTempSla::reset() {
    amax = 0.;
}

void AssimilationMaxGivenTempSla::update()
{
    double sla = specificLeafArea->pullVariable("sla");
    double Tday = weather->pullVariable("Tday");
    amax=(16.92-16.92*pow(0.88, Tday))*pctN/sla/10.;
    if (amax < 0.)
        amax = 0.;
}


} //namespace
