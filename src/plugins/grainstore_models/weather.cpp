/* Copyright (C) 2009-2010 by Niels Holst [niels.holst@agrsci.dk] and co-authors.
** Copyrights reserved.
** Released under the terms of the GNU General Public License version 3.0 or later.
** See www.gnu.org/copyleft/gpl.html.
*/
#include <usbase/pull_variable.h>
#include "weather.h"

using namespace std;
using namespace UniSim;

namespace grainstore{

Weather::Weather(UniSim::Identifier name, QObject *parent)
    : WeatherFile(name, parent)
{
    setColumn("gmc", 3);
    setColumn("Tmax", 4);
    setColumn("Tmin", 5);
    setColumn("marketPrice", 6);

    new PullVariable("Tavg", &Tavg, this, "desc");
}


void Weather::update()
{
    WeatherFile::update();
    Tavg = (pullVariable("Tmin") + pullVariable("Tmax"))/2.;
}

} //namespace

