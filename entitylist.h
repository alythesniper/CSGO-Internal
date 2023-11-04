#include <cstdint>

class clientInfo
{
public:
	class Ent* EntPtr; //0x0000
	int32_t wierdNum; //0x0004
	class clientInfo* backLink; //0x0008
	class clientInfo* forwardLink; //0x000C
}; //Size: 0x0010
static_assert(sizeof(clientInfo) == 0x10);

class cBaseEntityList
{
public:
	class clientInfo EntList[64]; //0x0000
	char pad_0400[376]; //0x0400
}; //Size: 0x0578
static_assert(sizeof(cBaseEntityList) == 0x578);

class Ent
{
public:
	char pad_0000[237]; //0x0000
	bool N00000112; //0x00ED
	char pad_00EE[18]; //0x00EE
	int32_t Health; //0x0100
	char pad_0104[2104]; //0x0104
	int8_t N00000312; //0x093C
	bool bSpotted; //0x093D
	char pad_093E[1030]; //0x093E
}; //Size: 0x0D44
static_assert(sizeof(Ent) == 0xD44);

class N000001D8
{
public:
	char pad_0000[4]; //0x0000
}; //Size: 0x0004
static_assert(sizeof(N000001D8) == 0x4);

class N00000248
{
public:
	char pad_0000[4]; //0x0000
}; //Size: 0x0004
static_assert(sizeof(N00000248) == 0x4);
