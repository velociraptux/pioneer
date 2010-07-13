#include "libs.h"
#include "PiLuaAPI.h"
#include "Pi.h"
#include "Space.h"
#include "ShipCpanel.h"
#include "Ship.h"
#include "Player.h"
#include "SpaceStation.h"
#include "StarSystem.h"
#include "Sound.h"
#include "LuaChatForm.h"
#include "NameGenerator.h"
#include "HyperspaceCloud.h"
#include "Polit.h"

void ship_randomly_equip(Ship *ship, double power)
{
	const shipstats_t *stats;
	const ShipType &type = ship->GetShipType();

	// SLOT_ENGINE
	ship->m_equipment.Set(Equip::SLOT_ENGINE, 0, type.hyperdrive);
	stats = ship->CalcStats();

	// go through equipment in random order, seeing if we want the damn stuff
	int *equip_types = (int*)alloca(sizeof(int)*Equip::TYPE_MAX);
	for (int i=0; i<Equip::TYPE_MAX; i++) equip_types[i] = i;
	for (int i=0; i<Equip::TYPE_MAX; i++) { // randomize order
		int swap_with = Pi::rng.Int32(Equip::TYPE_MAX);
		int temp = equip_types[i];
		equip_types[i] = equip_types[swap_with];
		equip_types[swap_with] = temp;
	}
	
	for (int i=0; i<Equip::TYPE_MAX; i++) {
		const Equip::Type t = (Equip::Type)equip_types[i];
		const EquipType &e = EquipType::types[t];
		// leave space for fuel
		if (e.mass > stats->free_capacity - type.hyperdrive) continue;
		switch (e.slot) {
		case Equip::SLOT_CARGO:
		case Equip::SLOT_ENGINE:
			break;
		case Equip::SLOT_LASER:
			if (e.mass > stats->free_capacity/2) break;
			if (Pi::rng.Double() < power) {
				ship->m_equipment.Set(e.slot, 0, t);
			}
			break;
		case Equip::SLOT_MISSILE:
			if (Pi::rng.Double() < power) {
				ship->m_equipment.Add(t);
			}
			break;
		case Equip::SLOT_ECM:
		case Equip::SLOT_SCANNER:
		case Equip::SLOT_RADARMAPPER:
		case Equip::SLOT_HYPERCLOUD:
		case Equip::SLOT_HULLAUTOREPAIR:
		case Equip::SLOT_ENERGYBOOSTER:
		case Equip::SLOT_ATMOSHIELD:
		case Equip::SLOT_FUELSCOOP:
		case Equip::SLOT_LASERCOOLER:
		case Equip::SLOT_CARGOLIFESUPPORT:
		case Equip::SLOT_AUTOPILOT:
			if (Pi::rng.Double() < power) {
				ship->m_equipment.Add(t);
			}
			break;
		case Equip::SLOT_MAX: break;
			
		}
		stats = ship->CalcStats();
	}
	
	int amount = MIN(EquipType::types[type.hyperdrive].pval, stats->free_capacity);
	while (amount--) ship->m_equipment.Add(Equip::HYDROGEN);
}

////////////////////////////////////////////////////////////

EXPORT_OOLUA_FUNCTIONS_12_NON_CONST(ObjectWrapper,
		ShipAIDoKill,
		ShipAIDoFlyTo,
		ShipAIDoLowOrbit,
		ShipAIDoMediumOrbit,
		ShipAIDoHighOrbit,
		SetMoney,
		AddMoney,
		GetEquipmentPrice,
		SpaceStationAddAdvert,
		SpaceStationRemoveAdvert,
		GetDockedWith,
		GetSBody)
EXPORT_OOLUA_FUNCTIONS_3_CONST(ObjectWrapper,
		IsBody,
		GetMoney,
		GetLabel)

ObjectWrapper::ObjectWrapper(Object *o): m_obj(o) {
	if (o) {
		m_delCon = o->onDelete.connect(sigc::mem_fun(this, &ObjectWrapper::OnDelete));
	}
}
bool ObjectWrapper::IsBody() const {
	return Is(Object::BODY);
}
double ObjectWrapper::GetMoney() const {
	if (Is(Object::SHIP)) {
		Ship *s = static_cast<Ship*>(m_obj);
		return 0.01 * s->GetMoney();
	} else {
		return 0;
	}
}
void ObjectWrapper::ShipAIDoKill(ObjectWrapper &o)
{
	if (Is(Object::SHIP) && o.m_obj) {
		Ship *s = static_cast<Ship*>(m_obj);
		s->AIClearInstructions();
		s->AIInstruct(Ship::DO_KILL, o.m_obj);
	}
}
void ObjectWrapper::ShipAIDoFlyTo(ObjectWrapper &o)
{
	if (Is(Object::SHIP) && o.m_obj) {
		Ship *s = static_cast<Ship*>(m_obj);
		s->AIClearInstructions();
		s->AIInstruct(Ship::DO_FLY_TO, o.m_obj);
	}
}
void ObjectWrapper::ShipAIDoLowOrbit(ObjectWrapper &o)
{
	if (Is(Object::SHIP) && o.m_obj) {
		Ship *s = static_cast<Ship*>(m_obj);
		s->AIClearInstructions();
		s->AIInstruct(Ship::DO_LOW_ORBIT, o.m_obj);
	}
}
void ObjectWrapper::ShipAIDoMediumOrbit(ObjectWrapper &o)
{
	if (Is(Object::SHIP) && o.m_obj) {
		Ship *s = static_cast<Ship*>(m_obj);
		s->AIClearInstructions();
		s->AIInstruct(Ship::DO_MEDIUM_ORBIT, o.m_obj);
	}
}
void ObjectWrapper::ShipAIDoHighOrbit(ObjectWrapper &o)
{
	if (Is(Object::SHIP) && o.m_obj) {
		Ship *s = static_cast<Ship*>(m_obj);
		s->AIClearInstructions();
		s->AIInstruct(Ship::DO_HIGH_ORBIT, o.m_obj);
	}
}
void ObjectWrapper::SetMoney(double m) {
	if (Is(Object::SHIP)) {
		Ship *s = static_cast<Ship*>(m_obj);
		s->SetMoney((Sint64)(m*100.0));
	}
}
void ObjectWrapper::AddMoney(double m) {
	if (Is(Object::SHIP)) {
		Ship *s = static_cast<Ship*>(m_obj);
		s->SetMoney(s->GetMoney() + (Sint64)(m*100.0));
	}
}
double ObjectWrapper::GetEquipmentPrice(int equip_type) {
	MarketAgent *m = 0;
	if (Is(Object::SHIP)) {
		m = static_cast<MarketAgent*>(static_cast<Ship*>(m_obj));
	}
	else if (Is(Object::SPACESTATION)) {
		m = static_cast<MarketAgent*>(static_cast<SpaceStation*>(m_obj));
	} else {
		return 0;
	}
	Sint64 cost = m->GetPrice((Equip::Type)equip_type);
	return 0.01 * (double)cost;
}
const char *ObjectWrapper::GetLabel() const {
	if (Is(Object::BODY)) {
		return static_cast<Body*>(m_obj)->GetLabel().c_str();
	} else {
		return "";
	}
}
void ObjectWrapper::SpaceStationAddAdvert(const char *luaMod, int luaRef, const char *description) {
	if (Is(Object::SPACESTATION)) {
		static_cast<SpaceStation*>(m_obj)->BBAddAdvert(BBAdvert(luaMod, luaRef, description));
	}
}
void ObjectWrapper::SpaceStationRemoveAdvert(const char *luaMod, int luaRef) {
	if (Is(Object::SPACESTATION)) {
		static_cast<SpaceStation*>(m_obj)->BBRemoveAdvert(luaMod, luaRef);
	}
}
SBodyPath *ObjectWrapper::GetSBody()
{
	const SBody *sbody = 0;
	if (Is(Object::BODY)) {
		sbody = static_cast<Body*>(m_obj)->GetSBody();
		if (sbody) {
			SBodyPath *path = new SBodyPath;
			Pi::currentSystem->GetPathOf(sbody, path);
			return path;
		}
	}
	return 0;
}
ObjectWrapper *ObjectWrapper::GetDockedWith()
{
	if (Is(Object::SHIP) && static_cast<Ship*>(m_obj)->GetDockedWith()) {
		return new ObjectWrapper(static_cast<Ship*>(m_obj)->GetDockedWith());
	} else {
		return 0;
	}
}

ObjectWrapper::~ObjectWrapper() {
//	printf("ObjWrapper for %s is being deleted\n", GetLabel());
	m_delCon.disconnect();
}
bool ObjectWrapper::Is(Object::Type t) const {
	return m_obj && m_obj->IsType(t);
}
void ObjectWrapper::OnDelete() {
	// object got deleted out from under us
	m_obj = 0;
	m_delCon.disconnect();
}

/////////////////////////////////////////////////////////////
class Rand: public MTRand {
public:
	// don't add members, only methods (because dirty cast Pi::rng to Rand)
	Rand(): MTRand() {}
	Rand(unsigned long seed): MTRand(seed) {}
	std::string PersonName(bool isfemale) {
		return NameGenerator::FullName(*this, isfemale);
	}
	std::string Surname() {
		return NameGenerator::Surname(*this);
	}
};
OOLUA_CLASS_NO_BASES(Rand)
	OOLUA_NO_TYPEDEFS
	OOLUA_CONSTRUCTORS_BEGIN
		OOLUA_CONSTRUCTOR_1(unsigned long)
	OOLUA_CONSTRUCTORS_END
	OOLUA_MEM_FUNC_2_RENAME(Real, double, Double, double, double)
	OOLUA_MEM_FUNC_2_RENAME(Int, unsigned int, Int32, int, int)
	OOLUA_MEM_FUNC_1(std::string, PersonName, bool)
	OOLUA_MEM_FUNC_0(std::string, Surname)
OOLUA_CLASS_END

EXPORT_OOLUA_FUNCTIONS_0_CONST(Rand)
EXPORT_OOLUA_FUNCTIONS_4_NON_CONST(Rand,
		Real, Int, PersonName, Surname)

/////////////////////////////////////////////////////////////

// oolua doesn't like namespaces
class SoundEvent: public Sound::Event {};

OOLUA_CLASS_NO_BASES(SoundEvent)
	OOLUA_NO_TYPEDEFS
	OOLUA_ONLY_DEFAULT_CONSTRUCTOR
	OOLUA_MEM_FUNC_4(void, Play, const char *, float, float, Uint32)
	OOLUA_MEM_FUNC_0(bool, Stop)
OOLUA_CLASS_END

EXPORT_OOLUA_FUNCTIONS_2_NON_CONST(SoundEvent,
		Play, Stop)
EXPORT_OOLUA_FUNCTIONS_0_CONST(SoundEvent)

///////////////////////////////////////////////////////////////

static int UserDataSerialize(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TUSERDATA);
	if (mylua_checkudata(L, 1, "ObjectWrapper")) {
		char buf[256];
		ObjectWrapper *o;
		OOLUA::pull2cpp(L, o);
		snprintf(buf, sizeof(buf), "ObjectWrapper\n%d\n", Serializer::LookupBody((Body*)o->m_obj));
		lua_pushstring(L, buf);
		return 1;
	} else if (mylua_checkudata(L, 1, "SBodyPath")) {
		Serializer::Writer wr;
		SBodyPath *path;
		OOLUA::pull2cpp(L, path);
		path->Serialize(wr);
		std::string out = "SBodyPath\n";
		out += wr.GetData();
		OOLUA::push2lua(L, out);
		return 1;
	} else if (mylua_checkudata(L, 1, "SysLoc")) {
		Serializer::Writer wr;
		SysLoc *systemid;
		OOLUA::pull2cpp(L, systemid);
		systemid->Serialize(wr);
		std::string out = "SysLoc\n";
		out += wr.GetData();
		OOLUA::push2lua(L, out);
		return 1;
	} else {
		Error("Tried to serialize unknown userdata type.");
		return 0;
	}
}

static int UserDataUnserialize(lua_State *L)
{
	std::string str;
	OOLUA::pull2cpp(L, str);
	if (str.substr(0, 14) == "ObjectWrapper\n") {
		int idx = atoi(str.substr(14).c_str());
		Body *b = Serializer::LookupBody(idx);
		push2luaWithGc(L, new ObjectWrapper(b));
		return 1;
	} else if (str.substr(0, 10) == "SBodyPath\n") {
		Serializer::Reader r(str.substr(10));
		SBodyPath *p = new SBodyPath;
		SBodyPath::Unserialize(r, p);
		push2luaWithGc(L, p);
		return 1;
	} else if (str.substr(0, 7) == "SysLoc\n") {
		Serializer::Reader r(str.substr(7));
		SysLoc *p = new SysLoc;
		SysLoc::Unserialize(r, p);
		push2luaWithGc(L, p);
		return 1;
	}
	return 0;
}


struct UnknownShipType {};
/**
 * power 0 = unarmed, power 1 = armed to the teeth
 */
static std::string get_random_ship_type(double power, int minMass, int maxMass)
{
	// find a ship that fits in the mass range
	std::vector<ShipType::Type> candidates;

	for (std::map<ShipType::Type, ShipType>::iterator i = ShipType::types.begin();
			i != ShipType::types.end(); ++i) {
		int hullMass = (*i).second.hullMass;
		if ((hullMass >= minMass) && (hullMass <= maxMass)) {
			candidates.push_back((*i).first);
		}
	}
	printf("%d candidates\n", candidates.size());
	if (candidates.size() == 0) throw UnknownShipType();

	for (int i=0; i<candidates.size(); i++) {
		printf("%s\n", candidates[i].c_str());
	}

	return candidates[ Pi::rng.Int32(candidates.size()) ];
}

namespace LuaPi {
	static int GetPlayer(lua_State *l) {
		push2luaWithGc(l, new ObjectWrapper((Object*)Pi::player));
		return 1;
	}
	static int GetGameTime(lua_State *l) {
		OOLUA_C_FUNCTION_0(double, Pi::GetGameTime)
	}
	static int Message(lua_State *l) {
		std::string from, msg;
		OOLUA::pull2cpp(l, msg);
		OOLUA::pull2cpp(l, from);
		Pi::cpan->MsgLog()->Message(from, msg);
		return 0;
	}
	static int ImportantMessage(lua_State *l) {
		std::string from, msg;
		OOLUA::pull2cpp(l, msg);
		OOLUA::pull2cpp(l, from);
		Pi::cpan->MsgLog()->ImportantMessage(from, msg);
		return 0;
	}
	static int GetCurrentSystem(lua_State *l) {
		// sadly must rebuild for the mo
		StarSystem *cur = Pi::currentSystem;
		SysLoc *s = new SysLoc(cur->SectorX(), cur->SectorY(), cur->SystemIdx());
		push2luaWithGc(l, s);
		return 1;
	}
	static int FormatDate(lua_State *l) {
		double t;
		OOLUA::pull2cpp(l, t);
		std::string s = format_date(t);
		OOLUA::push2lua(l, s.c_str());
		return 1;
	}
	static int _spawn_ship(lua_State *l, std::string type, double due, double power) {
		if (ShipType::Get(type.c_str()) == 0) {
			throw UnknownShipType();
		} else {
			// for the mo, just put it near the player
			const vector3d pos = Pi::player->GetPosition() +
				10000.0 * vector3d(Pi::rng.Double(-1.0, 1.0), Pi::rng.Double(-1.0, 1.0), Pi::rng.Double(-1.0, 1.0));
			if (due <= Pi::GetGameTime()) {
				// already entered
				if (!Space::IsSystemBeingBuilt()) {
					lua_pushnil(l);
					lua_pushstring(l, "Insufficient time to generate ship entry");
					return 2;
				}
				if ((due <= 0) || (due < Pi::GetGameTime()-HYPERCLOUD_DURATION)) {
					// ship is supposed to have entered some time
					// ago and the hyperspace cloud is gone
					Ship *ship = new Ship(type.c_str());
					ship_randomly_equip(ship, power);
					ship->SetFrame(Pi::player->GetFrame());
					ship->SetPosition(pos);
					ship->SetVelocity(Pi::player->GetVelocity());
					Space::AddBody(ship);
					push2luaWithGc(l, new ObjectWrapper(ship));
					return 1;
				} else {
					// hypercloud still present
					Ship *ship = new Ship(type.c_str());
					ship_randomly_equip(ship, power);
					HyperspaceCloud *cloud = new HyperspaceCloud(ship, due, true);
					cloud->SetFrame(Pi::player->GetFrame());
					cloud->SetPosition(pos);
					cloud->SetVelocity(Pi::player->GetVelocity());
					Space::AddBody(cloud);
					push2luaWithGc(l, new ObjectWrapper(ship));
					return 1;
				}
			} else {
				// to hyperspace in shortly
				Ship *ship = new Ship(type.c_str());
				ship_randomly_equip(ship, power);
				HyperspaceCloud *cloud = new HyperspaceCloud(ship, due, true);
				cloud->SetFrame(Pi::player->GetFrame());
				cloud->SetPosition(pos);
				cloud->SetVelocity(Pi::player->GetVelocity());
				Space::AddBody(cloud);
				push2luaWithGc(l, new ObjectWrapper(ship));
				return 1;
			}
		}
	}
	static int SpawnShip(lua_State *l) {
		double due;
		std::string type;
		OOLUA::pull2cpp(l, type);
		OOLUA::pull2cpp(l, due);
		int ret;
		try {
			ret = _spawn_ship(l, type, due, 0.0);
		} catch (UnknownShipType) {
			lua_pushnil(l);
			lua_pushstring(l, "Unknown ship type");
			return 2;
		}
		return ret;
	}
	static int SpawnRandomShip(lua_State *l) {
		double due, power;
		int minMass, maxMass;
		std::string type;
		OOLUA::pull2cpp(l, maxMass);
		OOLUA::pull2cpp(l, minMass);
		OOLUA::pull2cpp(l, power);
		OOLUA::pull2cpp(l, due);
		printf("power %f, mass %d to %d\n", power, minMass, maxMass);
		int ret;
		try {
			type = get_random_ship_type(power, minMass, maxMass);
			ret = _spawn_ship(l, type, due, power);
		} catch (UnknownShipType) {
			lua_pushnil(l);
			lua_pushstring(l, "Unknown ship type");
			return 2;
		}
		return ret;
	}
	static int AddPlayerCrime(lua_State *l) {
		Sint64 crimeBitset;
		double fine;
		OOLUA::pull2cpp(l, fine);
		OOLUA::pull2cpp(l, crimeBitset);
		Sint64 _fine = (Sint64)(100.0*fine);
		Polit::AddCrime(crimeBitset, _fine);
		return 0;
	}
}

#define REG_FUNC(fnname, fnptr) \
	lua_pushcfunction(l, fnptr);\
	lua_setfield(l, -2, fnname)

void RegisterPiLuaAPI(lua_State *l)
{
//	printf("XXXXXXXXXXXXXXXXXXXXXXXX GET RID OF THIS SHIT!!!!!!!! XXXXXXXXXXXXX\n");
//	make_random_ship(1, 10, 100);
//	make_random_ship(1, 100, 1000);

	OOLUA::register_class<ObjectWrapper>(l);
	OOLUA::register_class<LuaChatForm>(l);
	OOLUA::register_class<SoundEvent>(l);
	OOLUA::register_class<SysLoc>(l);
	OOLUA::register_class<SBodyPath>(l);
	OOLUA::register_class<Rand>(l);
	
	lua_register(l, "UserDataSerialize", UserDataSerialize);
	lua_register(l, "UserDataUnserialize", UserDataUnserialize);

	lua_newtable(l);
	REG_FUNC("AddPlayerCrime", &LuaPi::AddPlayerCrime);
	REG_FUNC("GetCurrentSystem", &LuaPi::GetCurrentSystem);
	REG_FUNC("GetPlayer", &LuaPi::GetPlayer);
	REG_FUNC("GetGameTime", &LuaPi::GetGameTime);
	REG_FUNC("Message", &LuaPi::Message);
	REG_FUNC("ImportantMessage", &LuaPi::ImportantMessage);
	REG_FUNC("SpawnShip", &LuaPi::SpawnShip);
	REG_FUNC("SpawnRandomShip", &LuaPi::SpawnRandomShip);
	lua_setglobal(l, "Pi");
	
	lua_newtable(l);
	REG_FUNC("Format", &LuaPi::FormatDate);
	lua_setglobal(l, "Date");
}
