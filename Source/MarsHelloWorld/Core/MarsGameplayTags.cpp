// Copyright (c) 2026 Phil Burns. MIT License (see LICENSE).

#include "Core/MarsGameplayTags.h"

namespace MarsTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Entering,   "Mars.State.Entering",   "Running / flying / hovering into the arena");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Greeting,   "Mars.State.Greeting",   "Holding for a conversation line");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Ready,      "Mars.State.Ready",      "Arrived, waiting for the fight");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Fighting,   "Mars.State.Fighting",   "Free-for-all combat");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead,       "Mars.State.Dead",       "Collapsed");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Victorious, "Mars.State.Victorious", "Last one standing");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Melee,  "Mars.Damage.Melee",  "Claws, punches");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Acid,   "Mars.Damage.Acid",   "Martian acid spit");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Laser,  "Mars.Damage.Laser",  "X-Bot eye laser");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Plasma, "Mars.Damage.Plasma", "Venusian plasma orb");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Sonic,  "Mars.Damage.Sonic",  "Venusian sonic pulse");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Rifle,  "Mars.Damage.Rifle",  "Astronaut rifle");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Species_Martian,  "Mars.Species.Martian",  "Native of Mars");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Species_XBot,     "Mars.Species.XBot",     "Robot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Species_Venusian, "Mars.Species.Venusian", "Visitor from Venus");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Species_Astronaut,"Mars.Species.Astronaut","Earth astronaut (player)");
}
