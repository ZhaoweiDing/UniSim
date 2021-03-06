/* Copyright (C) 2009-2010 by Niels Holst [niels.holst@agrsci.dk] and co-authors.
** Copyrights reserved.
** Released under the terms of the GNU General Public License version 3.0 or later.
** See www.gnu.org/copyleft/gpl.html.
*/
#include <QMessageBox>
#include <QString>
#include <usbase/component.h>
#include <usbase/exception.h>
#include <usbase/pull_variable.h>
#include <usbase/utilities.h>
#include "rotation.h"
#include "weed.h"

using namespace UniSim;

namespace ess2009 {

Weed::Weed(UniSim::Identifier name, QObject *parent)
    : Model(name, parent)
{
    setRecursionPolicy(Component::Update, Component::ChildrenNot);

    new PullVariable("prevOutflowTotal", &_prevOutflowTotal, this, "description");
    new PullVariable("projectedDeqs", &_projectedDeqs, this, "description");
    new PullVariable("projectedYieldLossPct", &_projectedYieldLossPct, this, "description");
    new PullVariable("projectedMass", &_projectedMass, this, "description");
    new PullVariable("seedsDropping", &_seedsDropping, this, "description");
    new PullVariable("g", &_g, this, "description");
}

void Weed::initialize() {
    setParameter("cropLaiExp1", &_cropLaiExp1, 6.904, "description");
    setParameter("cropLaiExp2", &_cropLaiExp2, 0.7491, "description");
    setParameter("seedProdSlope", &_seedProdSlope, 2., "description");
    setParameter("seedProdExp", &_seedProdExp, 1., "description");
    setParameter("seedSpillAtHarvestPct", &_seedSpillAtHarvestPct, 100., "description");

    _seedBank = seekOneChild<Model*>("seedBank");
    _seedling = seekOneChild<Model*>("seedling");
    _juvenile = seekOneChild<Model*>("juvenile");
    _mature   = seekOneChild<Model*>("mature");
    _seedsOnPlant = seekOneChild<Model*>("seedsOnPlant");

    _calendar = seekOne<Model*>("calendar");
    _rotation = seekOne<Rotation*>("rotation");
    _weather = seekOne<Model*>("weather");

    if (!_rotation) throw Exception("Weed could not find rotation model");

    QList<Model*> crops = _rotation->cropModels();
    for (int i = 0; i < crops.size(); ++i)
        connect(crops[i], SIGNAL(event(QObject*, QString)), this, SLOT(handleEvent(QObject*, QString)));
}

void Weed::reset() {
    _projectedDeqs = _prevOutflowTotal = _seedsDropping = 0;
}

void Weed::update() {
    _seedBank->pushVariable("dormantInflow", _seedsDropping);
    _seedsDropping = 0;
    _seedBank->update();
    double newSeedlings = _seedBank->pullVariable("dailyEmergenceDensity");

    _seedling->pushVariable("inflowAsDensity", newSeedlings);
    _seedling->pushVariable("inflowAsDensityEqs", newSeedlings*cropEffectOnSeedlings());
	_seedling->update();

    _juvenile->pushVariable("inflowAsDensity",
                            _seedling->pullVariable("outflowAsDensity"));
    _juvenile->pushVariable("inflowAsDensityEqs",
                            _seedling->pullVariable("outflowAsDensityEqs"));
	_juvenile->update();

    _mature->pushVariable("inflowAsDensity",
                          _juvenile->pullVariable("outflowAsDensity"));
    _mature->pushVariable("inflowAsDensityEqs",
                          _juvenile->pullVariable("outflowAsDensityEqs"));
	_mature->update();

	projectCompetitionOutcome();
    double seedInflowAsMass = (_seedProdSlope > 0.) ? proportionDeqsEnteringMaturity()*_projectedMass : 0.;

	double g = (_projectedMass > 1e-6) ?  _seedProdSlope*pow(_projectedMass, _seedProdExp)/_projectedMass : 1.;
    _g = g;

    _seedsOnPlant->pushVariable("inflow", seedInflowAsMass);
    if (g > 0) _seedsOnPlant->pushVariable("growthRate", g);
    _seedsOnPlant->deepUpdate();

    _seedsDropping += _seedsOnPlant->pullVariable("outflow");
}

double Weed::cropEffectOnSeedlings() const {
    double lai = _rotation->pullVariable("lai");
	return exp(_cropLaiExp1*(exp(-lai/_cropLaiExp2)-1.)) - exp(-_cropLaiExp1);
}

void Weed::projectCompetitionOutcome() {
	_projectedDeqs = projectedDeqs();
    _projectedYieldLossPct = yieldLossPct(_projectedDeqs);

    Model *crop = _rotation->currentCrop();
    double maxYield = crop->parameter<double>("maxYield"),
           weedExchangeRate = crop->parameter<double>("weedExchangeRate");

	_projectedMass = maxYield*_projectedYieldLossPct/weedExchangeRate/100.;
}

double Weed::projectedDeqs() {
    Model *seedlingDEqs = _seedling->seekOneChild<Model*>("densityEqs"),
          *juvenileDEqs = _juvenile->seekOneChild<Model*>("densityEqs"),
          *matureDEqs   = _mature->seekOneChild<Model*>("densityEqs");

    return seedlingDEqs->pullVariable("number") +
           juvenileDEqs->pullVariable("number") +
           matureDEqs->pullVariable("number") +
           matureDEqs->pullVariable("outflowTotal") -
		   _prevOutflowTotal;
}

double Weed::yieldLossPct(double N) const
{
  Model *crop = _rotation->currentCrop();
  double a = crop->parameter<double>("maxYieldLossPct"),
         i = crop->parameter<double>("slopeYieldLossPct");

  return (a == 0) ? 0. : i*N/(1. + i*N/a);;
}

double Weed::proportionDeqsEnteringMaturity() const {
    double prop = (_projectedDeqs > 0) ?
                  _juvenile->pullVariable("outflowAsDensityEqs")/_projectedDeqs : 0.;

	if (prop > 1.){
		//Q_ASSERT(prop < 1.001);
		prop = 1.;
	}
	return prop;
}

void Weed::handleEvent(QObject *sender, QString event) {
    Model *matureDeqs = _mature->seekOneChild<Model*>("densityEqs");
    if (event == "sowing") {
        kill(_seedling, 100);
        kill(_juvenile, 100);
        kill(_mature, 100);
        killAndSpillSeedsOnPlant(0);

        _prevOutflowTotal = matureDeqs->pullVariable("outflowTotal");
    }
    else if (event == "harvest") {
        kill(_seedling, 100);
        kill(_juvenile, 100);
        kill(_mature, 100);
        killAndSpillSeedsOnPlant(100 - _seedSpillAtHarvestPct);

        _prevOutflowTotal = matureDeqs->pullVariable("outflowTotal");
    }
}

void Weed::kill(Model *stage, double mortalityPct) {
    stage->pushVariable("instantMortality", mortalityPct);
}

void Weed::killAndSpillSeedsOnPlant(double mortalityPct) {
    // spilled seeds could calcualted as
    // _seedsOnPlant->state("number")*(1. - mortalityPct/100.);
    _seedsOnPlant->pushVariable("instantMortality", 100.);
}

}
