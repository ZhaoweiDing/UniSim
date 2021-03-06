/* Copyright (C) 2009-2010 by Niels Holst [niels.holst@agrsci.dk] and co-authors.
** Copyrights reserved.
** Released under the terms of the GNU General Public License version 3.0 or later.
** See www.gnu.org/copyleft/gpl.html.
*/
#include "ambrosia_model_maker.h"
#include <usbase/object_pool.h>
#include <usbase/utilities.h>
#include "plant.h"
#include "time.h"
#include "weather.h"

using namespace UniSim;

namespace ambrosia{

UniSim::Identifier AmbrosiaModelMaker::pluginName() const {
    return "ambrosia";
}

QString AmbrosiaModelMaker::pluginDesc() const {
    return
    "The @F ambrosia plugin is an implementation of the phenology model for @I {Ambrosia artemisiifolia}, "
    "as described by @Cite{$deen_et_al_1998a, $deen_et_al_1998b}.";
}

QStringList AmbrosiaModelMaker::authors() const {
    return QStringList()
        << "Niels Holst, Aarhus University, Denmark";
}

const QMap<Identifier, QString>& AmbrosiaModelMaker::supportedClasses() {
    if (!desc.isEmpty())
        return desc;

    desc["Plant"] =
    "@I pending";

    desc["Time"] =
    "@I pending";

    desc["Weather"] =
    "@I pending";

    return desc;
}

void AmbrosiaModelMaker::useObjectPool(ObjectPool *pool) const {
	objectPool()->deferTo(pool);
}

Model* AmbrosiaModelMaker::create(Identifier modelType, Identifier objectName, QObject *parent)
{
	setSimulationObjectFromDescendent(parent);
	Model *model = 0;
	if (modelType.equals("time"))
		model = new Time(objectName, parent);
    else if (modelType.equals("Plant"))
        model = new Plant(objectName, parent);
    else if (modelType.equals("Weather"))
        model = new Weather(objectName, parent);
    return model;
}

Q_EXPORT_PLUGIN2(ambrosia_model_maker, AmbrosiaModelMaker)

} //namespace
