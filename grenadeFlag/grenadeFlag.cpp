/*
 Custom flag: Grenade (+GN)
 First shot fires the grenade, second shot detonates it.
 
 Server Variables:
 _grenadeSpeedAdVel - multiplied by normal shot speed to determine speed
 _grenadeVerticalVelocity - whether or not the grenades use vertical velocity
 _grenadeWidth - distance from middle shot to side grenade PZ shot
 _grenadeAccuracy - level of accuracy of the grenade. Lower number is better; zero is perfect accuracy.
 
 Extra notes:
 - The player world weapon shots make use of metadata 'type' and 'owner'. Type is
   is GN and owner is the playerID.
 
 Copyright 2022 Quinn Carmack
 May be redistributed under either the LGPL or MIT licenses.
 
 ./configure --enable-custom-plugins=grenadeFlag
*/
 
#include "bzfsAPI.h"
#include <math.h>
#include <map>
using namespace std;

/*
 * Grenade object
 * This part actually stores the information needed with each grenade fired
*/
class Grenade
{
private:
	bool active = false;
	float origin[3];
	float velocity[3];
	double initialTime;
	
public:
	Grenade();
	void init(float*, float*);
	void clear();
	bool isActive();
	
	float* calculatePosition();
	bool isExpired();
};

Grenade::Grenade() {}

void Grenade::init(float* pos, float* vel)
{
	active = true;
	origin[0] = pos[0];
	origin[1] = pos[1];
	origin[2] = pos[2];
	velocity[0] = vel[0];
	velocity[1] = vel[1];
	velocity[2] = vel[2];
	initialTime = bz_getCurrentTime();
}

void Grenade::clear()
{
	active = false;
	origin[0] = 0;
	origin[1] = 0;
	origin[2] = 0;
	velocity[0] = 0;
	velocity[1] = 0;
	velocity[2] = 0;
}

bool Grenade::isActive() { return active; }

/*
 * calculatePosition
 * This method calculates the position of where the grenade should be if it
 * continues on its projected trajectory
*/
float* Grenade::calculatePosition()
{
	double elapsedTime = bz_getCurrentTime() - initialTime;
	float* pos = new float[3];
	
	pos[0] = origin[0] + velocity[0]*elapsedTime*bz_getBZDBInt("_shotSpeed");
	pos[1] = origin[1] + velocity[1]*elapsedTime*bz_getBZDBInt("_shotSpeed");
	pos[2] = origin[2] + velocity[2]*elapsedTime*bz_getBZDBInt("_shotSpeed");
	
	return pos;
}

/*
 * isExpired
 * This method calculates if the grenade PZ shots are expired by this time.
*/
bool Grenade::isExpired()
{
	if (calculatePosition()[2] <= 0)
		return true;
	else if ((bz_getCurrentTime() - initialTime)*bz_getBZDBInt("_shotSpeed") < bz_getBZDBInt("_shotRange"))
		return false;
	else
		return true;
}

/*************************************
***     Grenade Flag Plugin       ****
**************************************/

class GrenadeFlag : public bz_Plugin
{
	virtual const char* Name()
	{
		return "Grenade Flag";
	}
	virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	~GrenadeFlag();

	virtual void Cleanup(void)
	{
		Flush();
	}
	
protected:
    map<int, Grenade*> grenadeMap; // playerID, Grenade
};

BZ_PLUGIN(GrenadeFlag)

void GrenadeFlag::Init(const char*)
{
	bz_RegisterCustomFlag("GN", "Grenade", "First shot fires the grenade, second shot detonates.", 0, eGoodFlag);
	
	bz_registerCustomBZDBDouble("_grenadeSpeedAdVel", 4.0);
	bz_registerCustomBZDBDouble("_grenadeVerticalVelocity", false);
	bz_registerCustomBZDBDouble("_grenadeWidth", 2.0);
	bz_registerCustomBZDBDouble("_grenadeAccuracy", 0.02);		// Lower number is better accuracy
	
	Register(bz_eShotFiredEvent);
	Register(bz_ePlayerJoinEvent);
	Register(bz_ePlayerPartEvent);
	Register(bz_ePlayerDieEvent);
}

GrenadeFlag::~GrenadeFlag() {}

float randomFloat(float a, float b)
{
    float random = ((float) rand()) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

void GrenadeFlag::Event(bz_EventData *eventData)
{
	switch (eventData->eventType)
	{
		case bz_eShotFiredEvent:
		{
			bz_ShotFiredEventData_V1* data = (bz_ShotFiredEventData_V1*) eventData;
			bz_BasePlayerRecord* playerRecord = bz_getPlayerByIndex(data->playerID);
			
			if (playerRecord && playerRecord->currentFlag == "GreNade (+GN)")
			{
				// If an active grenade is expired, clear it from the records.
				if (grenadeMap[data->playerID]->isActive())
					if (grenadeMap[data->playerID]->isExpired())
						grenadeMap[data->playerID]->clear();
			
				// If this player does not have an active grenade, we can launch
				// one.
				if (grenadeMap[data->playerID]->isActive() == false)
				{
					float vel[3]; // PZ shot's velocity
					float pos[3];      // PZ shot's base position
					float offset[2];   // PZ shot's offset from base position
					float pos1[3];     // Position of one PZ shot
					float pos2[3];     // Position of the other PZ shot
					float innacuracy = randomFloat(-bz_getBZDBDouble("_grenadeAccuracy"), bz_getBZDBDouble("_grenadeAccuracy"));
					
					// Base/center position of the two PZ shots
					pos[0] = playerRecord->lastKnownState.pos[0] + cos(playerRecord->lastKnownState.rotation)*bz_getBZDBDouble("_muzzleFront");
					pos[1] = playerRecord->lastKnownState.pos[1] + sin(playerRecord->lastKnownState.rotation)*bz_getBZDBDouble("_muzzleFront");
					pos[2] = playerRecord->lastKnownState.pos[2] + bz_getBZDBDouble("_muzzleHeight");
					
					// The offset of the PZ shots
					offset[0] = -sin(playerRecord->lastKnownState.rotation)*bz_getBZDBDouble("_grenadeWidth");
					offset[1] = cos(playerRecord->lastKnownState.rotation)*bz_getBZDBDouble("_grenadeWidth");
					
					// Velocity of the PZ shots
					vel[0] = cos(playerRecord->lastKnownState.rotation+innacuracy)*bz_getBZDBDouble("_grenadeSpeedAdVel");
					vel[1] = sin(playerRecord->lastKnownState.rotation+innacuracy)*bz_getBZDBDouble("_grenadeSpeedAdVel");
					vel[2] = sin(abs(innacuracy));
					
					// If the vertical velocity option is turned on, then apply
					// it to the PZ shots.
					if (bz_getBZDBBool("_grenadeVerticalVelocity"))
						vel[2] += playerRecord->lastKnownState.velocity[2]/bz_getBZDBDouble("_shotSpeed");
						
					// PZ shot 1
					pos1[0] = pos[0] + offset[0];
					pos1[1] = pos[1] + offset[1];
					pos1[2] = pos[2];
					bz_fireServerShot("PZ", pos1, vel, bz_getPlayerTeam(data->playerID));
					
					// PZ shot 2
					pos2[0] = pos[0] - offset[0];
					pos2[1] = pos[1] - offset[1];
					pos2[2] = pos[2];
					bz_fireServerShot("PZ", pos2, vel, bz_getPlayerTeam(data->playerID));
					
					grenadeMap[data->playerID]->init(pos, vel);
				}
				// If not, then we detonate it.
				else
				{
					float vel[3] = { 0, 0, 0 };
					float* pos = grenadeMap[data->playerID]->calculatePosition();
					
					uint32_t shotGUID = bz_fireServerShot("SW", pos, vel, bz_getPlayerTeam(data->playerID));
				  	bz_setShotMetaData(shotGUID, "type", "GN");
					bz_setShotMetaData(shotGUID, "owner", data->playerID);
					
					grenadeMap[data->playerID]->clear();
				}
			}
		
			bz_freePlayerRecord(playerRecord);
		} break;
		case bz_ePlayerDieEvent:
		{
			bz_PlayerDieEventData_V1* data = (bz_PlayerDieEventData_V1*) eventData;
			// Grab the shot's meta data
		    uint32_t shotGUID = bz_getShotGUID(data->killerID, data->shotID);

		    // First check whether this shot has a "type" and "owner" assigned.
		    if (bz_shotHasMetaData(shotGUID, "type") && bz_shotHasMetaData(shotGUID, "owner"))
		    {
		        std::string flagType = bz_getShotMetaDataS(shotGUID, "type");

				// This checks if it really is one of the GK shockwaves we
				// created in part (1).
		        if (flagType == "GN")
		        {
		            // Assign the killer ID to be that in the shot.
		            data->killerID = bz_getShotMetaDataI(shotGUID, "owner");
		            data->killerTeam = bz_getPlayerTeam(data->killerID);
		        }
		    }
        } break;
		case bz_ePlayerJoinEvent:
		{
		    grenadeMap[((bz_PlayerJoinPartEventData_V1*) eventData)->playerID] = new Grenade();
		} break;
		case bz_ePlayerPartEvent:
		{
			bz_PlayerJoinPartEventData_V1* data = (bz_PlayerJoinPartEventData_V1*) eventData;
			delete grenadeMap[data->playerID];		// Release the memory
		    grenadeMap.erase(data->playerID);
		} break;
		default:
			break;
	}
}



