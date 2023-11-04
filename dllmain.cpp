#include <Windows.h>
#include <iostream>
#include "mem.h"
#include "stdafx.h"
#include "entity.h"
#include "entitylist.h"
#include "sniperIds.h"
#include <math.h>

//creating struct for csgo glow obj
struct GlowStruct
{
    BYTE base[4];
    float red;
    float green;
    float blue;
    float alpha;
    BYTE buffer[16];
    bool renderWhenOccluded;
    bool renderWhenUnOccluded;
    bool fullBloom;
    BYTE buffer1[5];
    int glowStyle;
};

//creating cords struct
struct vec3 { 

    float x, y, z; 

	vec3 operator+(vec3 d) {
		return { x + d.x, y + d.y, z + d.z };
	}
	vec3 operator-(vec3 d) {
		return { x - d.x, y - d.y, z - d.z };
	}
	vec3 operator*(float d) {
		return { x * d, y * d, z * d };
	}
	void Normalize() {
		while (y < -180) { y += 360; }
		while (y > 180) { y -= 360; }
		if (x > 89) { x = 89; }
		if (x < -89) { x = -89; }
	}

};

//get module base addess
uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandle(L"client.dll");

//get engine.dll address
uintptr_t engineBaseAddress = (uintptr_t)GetModuleHandle(L"engine.dll");

//get local player
uintptr_t* localPlayerPtr{ (uintptr_t*)(moduleBaseAddress + offsets::dwLocalPlayer) };

//getting shots fired
int* shotsFired = (int*)((*localPlayerPtr) + offsets::m_iShotsFired);

//get view angles
vec3* viewAngles = (vec3*)(*(uintptr_t*)(engineBaseAddress + offsets::dwClientState) + offsets::dwClientState_ViewAngles);

//getting view offset
vec3 viewOffset = *(vec3*)((*localPlayerPtr)+offsets::m_vecViewOffset);

//getting bone position
vec3* getBonePos(int boneId, uintptr_t* ent) {
    uint32_t boneMatrix = *((uint32_t*)((*ent) + offsets::m_dwBoneMatrix));
    vec3 bonePos;
    bonePos.x = *(float*)(boneMatrix + 0x30 * boneId + 0x0C);
    bonePos.y = *(float*)(boneMatrix + 0x30 * boneId + 0x1C);
    bonePos.z = *(float*)(boneMatrix + 0x30 * boneId + 0x2C);

    return &bonePos;
}

//creating get distance function (between 2 entites)  getDistance(vec3 ent1, vec3 ent2) 
float getDistance(vec3 ent1, vec3 ent2) {
    float distance = sqrt(pow(ent1.x - ent2.x, 2) + pow(ent1.z - ent2.z, 2) + pow(ent1.y - ent2.y, 2)) * 0.0254;
    return distance;
}

//Aim at function arg1;Target
void aimAt(vec3* targetPos) {
    double PI = 3.14159265358;
    vec3* localEntPos = ((vec3*)((*localPlayerPtr) + offsets::m_vecOrigin));
    vec3 deltaVec = { targetPos->x - localEntPos->x, targetPos->y - localEntPos->y, targetPos->z - localEntPos->z };
    float deltaVecLength = sqrt(deltaVec.x * deltaVec.x + deltaVec.y * deltaVec.y + deltaVec.z * deltaVec.z);

    float pitch = -asin(deltaVec.z / deltaVecLength) * (180 / PI);
    float yaw = atan2(deltaVec.y, deltaVec.x) * (180 / PI);

    //checks if pitch and yaw are in VAC range, otherwise ban
    if (pitch >= -89 && pitch <= 89 && yaw >= -180 && yaw <= 180)
    {
        viewAngles->x = pitch;
        viewAngles->y = yaw;
    }
}

//returns index in entlist of closest enemy
int getClosestEnemy(uintptr_t* localPlayerPtr) {

    int closestPlayerIndex{ 1337 };
    float tempDist{ 99999 };

    for (short int i = 0; i < 64; i++)
    {
        uintptr_t* pCurEntity = (uintptr_t*)(moduleBaseAddress + offsets::dwEntityList + i * 0x10);

        if (*pCurEntity != NULL) {
            if (*((bool*)((*pCurEntity) + offsets::m_bDormant)) == false && *((int*)((*pCurEntity) + offsets::m_iHealth)) > 0 && *((int*)((*pCurEntity) + offsets::m_iTeamNum)) != *((int*)((*localPlayerPtr) + offsets::m_iTeamNum))) {
                // if entity !dormant, alive and not in localplayer's team

                float playerDist = getDistance(*((vec3*)((*pCurEntity) + offsets::m_vecOrigin)), *((vec3*)((*localPlayerPtr) + offsets::m_vecOrigin)));

                if (playerDist < tempDist) {
                    closestPlayerIndex = i;
                    tempDist = playerDist;
                }

            }

        }

    }
    return closestPlayerIndex;
}

//getting, aim punch
vec3* aimPunchAngle = (vec3*)((*localPlayerPtr) + offsets::m_aimPunchAngle);

//getting weapon id
int getMyWeaponId()
{
    int weapon = *(int*)((*localPlayerPtr) + offsets::m_hActiveWeapon);
    uintptr_t weaponEntity = *(int*)(moduleBaseAddress + offsets::dwEntityList + (weapon & 0xFFF) * 0x10);
    if (weaponEntity != NULL)
        return *(int*)(weaponEntity + 0x258);
    else
        return 1337;
}

//creating shoot function, shoot(double msShootDelay = 0)
void shoot(int msShootDelay = 0) {
    int weaponId = getMyWeaponId();
    bool bShoot = true;
    for (size_t i = 0; i < 4; ++i) {
        if (weaponId == snipers::snipers[i]) {
            if (*(bool*)((*localPlayerPtr) + offsets::m_bIsScoped) == false){
                bShoot = false;
            }
        }
    }
    if (bShoot) {
        Sleep(msShootDelay);
        *((int*)(moduleBaseAddress + offsets::dwForceAttack)) = 5;
        Sleep(100);
        *((int*)(moduleBaseAddress + offsets::dwForceAttack)) = 4;
    }
}

DWORD WINAPI HackThread(HMODULE hModule)
{
    //Create Console
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    SetConsoleTitleA("alythesniper's Super Trainer");

    bool bNoFlash{ false }, bMinimap{ false }, bTrigBot{ false }, bAimbot{ false }, bRcs{ false };
    
    //get force jump address
    uintptr_t* fJump{ (uintptr_t*)(moduleBaseAddress + offsets::dwForceJump) };

    //player count
    int playerCount = 5; 

    //original punch location
    vec3 orPunch{ 0, 0, 0 };

    //checking if the localplayer exists or not in menu
    while (*localPlayerPtr != NULL) {
        BYTE bOnFloor = *((BYTE*)((*localPlayerPtr) + offsets::m_fFlags));

        //exit hack
        if (GetAsyncKeyState(VK_END) & 1) {
            break;
        }

        //bhop hack
        if (GetAsyncKeyState(VK_SPACE) and bOnFloor == 1) {
            *fJump = 6;
        }

        //no flash hack
        if (GetAsyncKeyState(VK_F1) & 1) {
            bNoFlash = !bNoFlash;
        }
        //super minimap hack
        if (GetAsyncKeyState(VK_F2) & 1) {
            bMinimap = !bMinimap;
        }
        //triggerbot hack
        if (GetAsyncKeyState(VK_F3) & 1) {
            bTrigBot = !bTrigBot;
        }
        //glow hack
        if (GetAsyncKeyState(VK_F4) & 1) {
            bAimbot = !bAimbot;
        }
        //rcs hack
        if (GetAsyncKeyState(VK_F5) & 1) {
            bRcs = !bRcs;
        }
        
        //for all continuous write hacks
        if (bMinimap) {
            for (short int i = 0; i < 64; i++)
            {
                uintptr_t* entity = (uintptr_t*)(moduleBaseAddress + offsets::dwEntityList + i * 0x10);
                if (*entity != NULL) {
                    bool* bSpotted = (bool*)((bool*)((*entity) + offsets::m_bSpotted));
                    *bSpotted = true;
                }

            }
        }
        if (bNoFlash) {
            int* flashDuration = ((int*)((*localPlayerPtr) + offsets::m_flFlashDuration));
            if (*flashDuration > 0)
                *flashDuration = 0;
        }
        if (bTrigBot) {
            for (short int i = 0; i < 64; i++)
            {
                //localplayer properties
                int* pCrossHairEnt = ((int*)((*localPlayerPtr) + offsets::m_iCrosshairId));
                int* pMyTeam = ((int*)((*localPlayerPtr) + offsets::m_iTeamNum));

                //current entity in loop properties
                uintptr_t* pCurEntity = (uintptr_t*)(moduleBaseAddress + offsets::dwEntityList + i * 0x10);
                int* pCurEntityTeam = ((int*)((*pCurEntity) + offsets::m_iTeamNum));

                if (*pCurEntity != NULL) {
                    int* pCurEntId = (int*)((*pCurEntity) + 0x64);
                    if (*pCurEntId == *pCrossHairEnt && *pCurEntityTeam != *pMyTeam) {

                        i = i;
                        vec3* localEntPos = ((vec3*)((*localPlayerPtr) + offsets::m_vecOrigin));
                        vec3* enemyEntPos = ((vec3*)((*pCurEntity) + offsets::m_vecOrigin));
                        shoot((getDistance(*localEntPos, *enemyEntPos) * 3.3));
                    }
                }

            }
        }
        if (bAimbot) {
            uintptr_t* pCurEntity = (uintptr_t*)(moduleBaseAddress + offsets::dwEntityList + getClosestEnemy(localPlayerPtr) * 0x10);

            aimAt((vec3*)((*pCurEntity) + offsets::m_vecOrigin));
        }
        if (bRcs) {
            vec3 punchAngle = *aimPunchAngle * 2;
            if (*shotsFired > 1) {
                vec3 newAngles = *viewAngles + orPunch - punchAngle;
                newAngles.Normalize();
                *viewAngles = newAngles;
            }
            orPunch = punchAngle;
        }
    }

    //close console and exit thread when loop is broken/exited
    fclose(f);
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}


BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        //DO NOT PUT CODE HERE!
        CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)HackThread, hModule, 0, nullptr));
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
