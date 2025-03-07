#include "fuel.h"
#include "Coolant.h"
#include "attachment.h"
using namespace std;

HANDLE& theOutputMutex()
{
	static HANDLE outputMutex;
	return outputMutex;
}

engine& theEngine()
{
	static engine myEngine;
	return myEngine;
}

fuelTank& theFuelTank()
{
	static fuelTank myTank;
	return myTank;
}

fuelPump& theFuelPump()
{
	static fuelPump myFuelPump;
	return myFuelPump;
}

void engine::setSpeed(int newSpeed)
{
	speed = newSpeed;
}

engine::engine()
{
	//Some things we need to do when the engine is first created
	active = false;
	speed = 0;
	engineTemperature = baseEngineTemp;
	coolantAmount = 0;
	coolantTemp = baseEngineTemp;
	altitude = 0;
	incline = 0;
	simCool = false;
}

void engine::reset()
{
	active = false;
	speed = 0;
	engineTemperature = baseEngineTemp;
	coolantAmount = 0;
	coolantTemp = baseEngineTemp;
	altitude = 0;
	incline = 0;
	simCool = false;
}
engine::~engine()
{
	//Nothing to do here in the destructor
}

int engine::getSpeed()
{
	//Set the speed of the engine
	return speed;
}

bool engine::engineStatus()
{
	//Query if the engine is on or off
	return active;
}

bool engine::startEngine()
{
	//Start the engine
	active = true;

	//Also update the engine temperature
	engineTemperature = baseEngineOpsTemp;

	//And start the coolant pump...
	theCoolantPump().setFlowAmt(1.5);
	return true;
}
bool engine::stopEngine()
{
	//Turn off the engine
	active = false;

	//Stop the coolant pump
	theCoolantPump().setFlowAmt(0);
	return false;
}

float engine::getCoolantAmount()
{
	return coolantAmount;
}
void engine::setCoolantAmount(float coolantValue, float temperature)
{
	coolantAmount = coolantValue;
	coolantTemp = temperature;
}

void engine::addCoolant(float coolantValue, float temperature)
{
	float heat = (coolantAmount * coolantTemp) + (coolantValue * temperature); //should avg out the coolant temperature

	coolantAmount = coolantAmount + coolantValue;
	temperature = heat / coolantAmount;

	coolantTemp = temperature;
}
float engine::getCoolantTemp()
{
	return coolantTemp;
}
void engine::setCoolantTemp(float temperature)
{
	coolantTemp = temperature;
}
float engine::getEngineTemperature()
{
	return engineTemperature;
}
void engine::setEngineTemperature(float newTemp)
{
	engineTemperature = newTemp;
}

void engine::setAltitudeKilometres(float newAltitudeValue)
{
	if (newAltitudeValue >= 0)
	{
		altitude = newAltitudeValue;
	}
}
float engine::getAltitudeKilometres()
{
	return altitude;
}
float engine::getAltitudeMetres()
{
	float altitudeMetres = altitude * 1000;
	return altitudeMetres;
}

void engine::setCool(bool newCool)
{
	simCool = newCool;
}

bool engine::returnCool()
{
	return simCool;
}

void engine::setIncline(float newIncline)
{
	incline = newIncline;
}
float engine::returnIncline()
{
	return incline;
}

fuelTank::fuelTank()
{
	currentVolume = tempFuelStart;
	capacity = tankCapacity;
	isPetrol = true;
}

void fuelTank::reset()
{
	currentVolume = tempFuelStart;
	capacity = tankCapacity;
	isPetrol = true;
}

fuelTank::~fuelTank()
{
	//Nothing to do here
}

void fuelTank::setFuel(float newVolume)
{
	//Replaces the current volume with a new volume
	currentVolume = newVolume;
}

float fuelTank::getCapacity()
{
	return capacity;
}

int fuelTank::getCurrentFuelLevelPercent()
{
	//Calculate a percentage value of the fuel level
	int fuelLevel = (currentVolume / capacity) * 100;

	return fuelLevel;
}

float fuelTank::getCurrentFuelLevel()
{
	return currentVolume;
}

void fuelTank::addFuel(float amountToAdd, bool isPetrolFilled)
{
	//Tops off the fuel level...
	currentVolume = currentVolume + amountToAdd;
	isPetrol = isPetrolFilled;
}

bool fuelTank::getFuelIsPetrol()
{
	//Return if there is petrol or diesel
	//If petrol this returns as true, else it will return as false
	return isPetrol;
}

fuelPump::fuelPump()
{
	//Constructor
	pressure = idlePressure;
	isPetrol = true;

	//cout << endl << "Constructor has been called..." << endl;
}

void fuelPump::reset()
{
	pressure = idlePressure;
	isPetrol = true;
}

fuelPump::~fuelPump()
{
	//Destructor
	//Nothing to do here
}
float fuelPump::getPressure()
{
	//Returns the current pressure set in the fuel pump
	return pressure;
}
bool fuelPump::fuelIsPetrol()
{
	//Returns if the fuel is petrol or not
	return isPetrol;

	//if return true, then it is petrol, else it is diesel
}
void fuelPump::setPressure(float newPressure)
{
	//Saves a new pressure for the fuel pump
	pressure = newPressure;

	getPressure();

}
void fuelPump::setFuelIsPetrol(bool newFuelStatus)
{
	//Saves a new fuel type to the fuel pump
	isPetrol = newFuelStatus;
}

unsigned int __stdcall engineManager(void* data)
{
	//Makes requests for fuel to the fuelPump when engine is active and based on speed - also check if there is fuel in the first place
	float pressureToSet = 0, engineFactor;
	float engineSpeed = 0;
	float engineTemp, coolantTemp, heatReduced, newTemp;
	float comparisonTemp;
	bool cool;
	float maxTemp;

	for (;;)
	{
		if (theEngine().engineStatus() == true)
		{
			//Engine is running
			//Request for fuel based on the speed
			//Ensure there is fuel... if there isn't, then should be that the engine cuts off now.
			if (theFuelTank().getCurrentFuelLevel() == 0)
			{
				//Fuel tank is empty
				theEngine().stopEngine();

				//Encase this cout in a mutex..

				WaitForSingleObject(theOutputMutex(), INFINITE);
				cout << endl << "Engine has run out of fuel!" << endl;
				ReleaseMutex(theOutputMutex());

				//Continue to the next iteration of the loop
				continue;
			}

			if (theFuelPump().fuelIsPetrol() == false)
			{
				//Stop the engine cause we are not using petrol anymore.
				theEngine().stopEngine();
				continue;
			}

			if (theEngine().getCoolantTemp() >= 239)
			{
				//Stop the engine cause the coolant is now overheeated
				cout << "Coolant temp is too high! " << endl;
				theEngine().stopEngine();
				continue;
			}

			//At this point, all our checks are OK. Need to see is we are simulating cooling or not
			cool = theEngine().returnCool();

			//By default, there is a minimum pressure for the fuel as long as the engine is idling - that is vehicle speed is 0. If not, the pressure will be proportionate to the speed
			if (theEngine().getSpeed() == 0)
			{
				//Engine is in idle
				pressureToSet = idlePressure;
				pressureToSet = pressureToSet + (theEngine().getAltitudeKilometres() / 10); //Consider the altitude here as well... Note that the altitude never goes below anyway
				theFuelPump().setPressure(pressureToSet);

				if (cool == true)
				{
					//Add a fixed value to the current engine temperature...
					engineTemp = theEngine().getEngineTemperature();

					if (engineTemp <= baseEngineOpsTemp)
					{
						engineTemp = engineTemp + (engineTemp * 0.1); //Increment by 10%

					}

					theEngine().setEngineTemperature(engineTemp);
				}
				else
				{
					//Not simulating the cooling system, just leave the engine temperature to be at the baseEngineOpsTemp
					theEngine().setEngineTemperature(baseEngineOpsTemp);
					engineTemp = baseEngineOpsTemp;
				}

				WaitForSingleObject(theOutputMutex(), INFINITE);
				cout << endl << "Engine temp is " << engineTemp << endl;
				ReleaseMutex(theOutputMutex());
			}
			else
			{
				//Set the fuel pressure based on the engine speed
				engineSpeed = theEngine().getSpeed();
				pressureToSet = idlePressure;

				engineFactor = engineSpeed / 100;
				engineFactor = engineFactor + (theEngine().getAltitudeKilometres() / 10);  //Consider the altitude here as well... Note that the altitude never goes below  0 anyway
				engineFactor = engineFactor + (theEngine().returnIncline() / 10); //Consider the incline of the slope...


				
				//Need to also check if there are any trailer attachments as well
				//if there is a trailer attachment, need to consider that as well
				if (theTowbar().getStatus() == true)
				{
					//Something is being attached.
					//Should therefore raise the enginefactor

					engineFactor = engineFactor * 1.5;

					//If there is no attachment, the engine factor is based on the speed only anyway so nothing for us to do on that side...
				}

				//Note if engineFactor becomes a -ve value, we need to adjust to at least 0

				if (engineFactor < 0)
				{
					engineFactor = 0; //override and set to 0
				}

				pressureToSet = pressureToSet + engineFactor;

				cout << endl << "Setting pressure to " << pressureToSet << endl;
				cout << endl << "Engine factor is " << engineFactor << endl;

				theFuelPump().setPressure(pressureToSet);

				engineTemp = theEngine().getEngineTemperature();

				if (cool == true)
				{
					//if we are simulating the cooling system
					//We need to set a limit to the engine temperature
					//Say temperature should not be double a certain speed (at the speed, it wont go higher...)
					comparisonTemp = baseEngineOpsTemp;
					maxTemp = 2 * engineSpeed;
					maxTemp = maxTemp + (2 * theEngine().getAltitudeKilometres()) + (theEngine().returnIncline());
					if (comparisonTemp < maxTemp)
					{
						comparisonTemp = maxTemp;
					}
					if (theEngine().getEngineTemperature() <= comparisonTemp)
					{
						//If less than double of the speed, then only add, else, dont add..
						WaitForSingleObject(theOutputMutex(), INFINITE);
						cout << endl << "Heating up engine... " << endl;
						ReleaseMutex(theOutputMutex());


						engineTemp = engineTemp + (engineTemp * (engineFactor*0.25)); //Increment based on the engine factor...

					}
					theEngine().setEngineTemperature(engineTemp);
				}
				else
				{
					//We are not simulating the cooling system.
					theEngine().setEngineTemperature(baseEngineOpsTemp);
				}
			
			}


			//Again, only do the coolant bits if we are simulating the coolant system
			if (cool == true)
			{
				//Update the coolant temperature based on engine temperature
				//Note that if the coolant hits the max temp, do not increase.
				WaitForSingleObject(allowRadiatorCoolantLevelModficiation(), INFINITE);
				WaitForSingleObject(allowEngineCoolantLevelModification(), INFINITE);

				WaitForSingleObject(theOutputMutex(), INFINITE);
				cout << endl << "Before adding, the engine coolant temp is " << theEngine().getCoolantTemp() << endl;
				ReleaseMutex(theOutputMutex());

				WaitForSingleObject(theOutputMutex(), INFINITE);
				cout << endl << "Before adding, engine temperature is  " << theEngine().getEngineTemperature() << endl;
				ReleaseMutex(theOutputMutex());

				coolantTemp = theEngine().getCoolantTemp() + theEngine().getEngineTemperature();
				coolantTemp = coolantTemp / 2; // average out the temperature

				if (coolantTemp > engineCoolantMaxTemp)
				{
					//Max temperature acheived
					heatReduced = engineCoolantMaxTemp - theEngine().getCoolantTemp();

					//Reduce that amount of heat from the engine
					newTemp = theEngine().getEngineTemperature();
					newTemp = newTemp - heatReduced;

					theEngine().setEngineTemperature(newTemp);

					//And update the coolant temperature
					coolantTemp = engineCoolantMaxTemp;
					theEngine().setCoolantTemp(coolantTemp);
				}
				else
				{
					//coolant temperature is not above maximum temperature
					//As a result coolant temperature will be the same as the engine temperature

					theEngine().setCoolantTemp(coolantTemp);
					theEngine().setEngineTemperature(coolantTemp);

					//Now, remove the coolant and put into the radiator
					//Only remove 80% of the coolant out...
					theRadiator().addCoolant((theEngine().getCoolantAmount() * 0.8), coolantTemp);
					theEngine().setCoolantAmount((theEngine().getCoolantAmount()*0.2), coolantTemp);

					WaitForSingleObject(theOutputMutex(), INFINITE);
					cout << endl << "New engine coolant temp is " << theEngine().getCoolantTemp() << endl;
					ReleaseMutex(theOutputMutex());

					WaitForSingleObject(theOutputMutex(), INFINITE);
					cout << endl << "New engine coolant amt is " << theEngine().getCoolantAmount() << endl;
					ReleaseMutex(theOutputMutex());

					WaitForSingleObject(theOutputMutex(), INFINITE);
					cout << endl << "New radiator coolant amt is " << theRadiator().getCoolantAmount() << endl;
					ReleaseMutex(theOutputMutex());

					ReleaseMutex(allowRadiatorCoolantLevelModficiation());
					ReleaseMutex(allowEngineCoolantLevelModification());

				}
			}	

			cout << endl << "ENGINE TEMP regardsless is now;" << theEngine().getEngineTemperature() << endl;
		}
		else
		{
			//Check to see if we are simulating the cooling system
			cool = theEngine().returnCool();

			
			//Engine is not on...

			//Ensure that the fuel pump pressure is set to 0
			theFuelPump().setPressure(0);

			if (cool == true)
			{
				// we are simulating the cooling system
				WaitForSingleObject(allowRadiatorCoolantLevelModficiation(), INFINITE);
				WaitForSingleObject(allowEngineCoolantLevelModification(), INFINITE);
				//Send away all the coolant to the radiator
				theRadiator().addCoolant(theEngine().getCoolantAmount(), theEngine().getCoolantTemp());

				//Remove the coolant from the engine
				theEngine().setCoolantAmount(0, ambientTemperature);
				//nothing else to do there.

				ReleaseMutex(allowRadiatorCoolantLevelModficiation());
				ReleaseMutex(allowEngineCoolantLevelModification());
			}
			
		}

		//Sleep
		Sleep(500); //Don't want to query it always, once every half second should be fine...

	}
	return 0;
}
unsigned int __stdcall fuelPumpManager(void* data)
{
	//Gets fuel from  the fuel tank, based on pressure update the value of the tank

	float amountToReduce;
	float newFuelLevel;

	//The idea is that the fuel tank level will reduce faster as the fuel pressure increases...
	for (;;)
	{
		//As long as there is fuel available, we should try to get the fuel out
		if (theEngine().engineStatus() == true)
		{
			//Only when engine is on...
			//First, check and see what type of fuel we are getting
			theFuelPump().setFuelIsPetrol(theFuelTank().getFuelIsPetrol());


			//Since we know the type of fuel at the pump, the engine can then find out the type of fuel as it needs to...

			//Reduce the amount of fuel accordingly... Right now, we shall set the amount to be reduced is 10% of the corresponding pressure...

			amountToReduce = theFuelPump().getPressure() * 0.1;

			//Update the fuel quantity

			newFuelLevel = theFuelTank().getCurrentFuelLevel() - amountToReduce;


			//Note, should never let this become a negative. If it becomes a negative, pad it to 0
			if (newFuelLevel <= 0)
			{
				newFuelLevel = 0;
			}
			theFuelTank().setFuel(newFuelLevel);

			WaitForSingleObject(theOutputMutex(), INFINITE);

			cout << endl << "Pressure is " << theFuelPump().getPressure() << endl;
			cout << endl << "Amount reduced is : " << amountToReduce << endl << "Fuel level is: " << theFuelTank().getCurrentFuelLevel() << endl;

			ReleaseMutex(theOutputMutex());
		}
		//Sleep for a while

		Sleep(500);

	}
	return 0;
}
unsigned int __stdcall  fuelTankManager(void* data)
{
	//make calls to car control panel as the fuel level goes down...

	//While there is still fuel, the pump will pull out the fuel from the tank
	//nothing much to do here for now
	for (;;)
	{

	}
	return 0;


}