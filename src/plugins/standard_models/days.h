/* Copyright (C) 2009-2010 by Niels Holst [niels.holst@agrsci.dk] and co-authors.
** Copyrights reserved.
** Released under the terms of the GNU General Public License version 3.0 or later.
** See www.gnu.org/copyleft/gpl.html.
*/
#ifndef UNISIM_DAY_S
#define UNISIM_DAY_S
#include <QObject>
#include <usbase/model.h>

namespace UniSim{

class Days : public UniSim::Model
{
	Q_OBJECT
public: 
    Days(UniSim::Identifier name, QObject *parent=0);
    // standard methods
    void reset();
    void update();
private:
    // state
    double step, total;
};

} //namespace
#endif
