#include "Global.h"
#include "Game_Consts.h"
#include "Configuration.h"
#include "GraphObject2D.h"
#include "ActorLifebar.h"
#include "ImageLoader.h"
#include "GameWindow.h"

ActorLifebar::ActorLifebar() : GraphObject2D()
{
	Health = 50; // Out of 100!
	pending_health = 0;
	time = 0;

	SetImage(ImageLoader::LoadSkin("healthbar.png"));

	SetWidth(PlayfieldHeight / 2);
	SetHeight(Configuration::GetSkinConfigf("Height", "Lifebar" ));
	Centered = Configuration::GetSkinConfigf( "Centered", "Lifebar" ) != 0;
	SetPosition( Configuration::GetSkinConfigf("X", "Lifebar" ), Configuration::GetSkinConfigf("Y", "Lifebar" ));
	SetRotation( Configuration::GetSkinConfigf("Rotation", "Lifebar" ) );
	AffectedByLightning = true;

	UpdateHealth();
}

void ActorLifebar::UpdateHealth()
{
	if (Health < 0)
	{
		Health = 0;
	}

	SetCrop2(glm::vec2(Health / 100, 1));
	Red = Green = Blue = Health / 100;
	SetWidth((Health / 100) * PlayfieldHeight);
}

void ActorLifebar::HitJudgement(Judgement Hit)
{
	switch (Hit)
	{
		case Excellent:
			pending_health += 2;
			break;
		case Perfect:
			pending_health += 1;
			break;
		case Great:
			break;
		case Bad:
			pending_health -= 3;
			break;
		case Miss:
		case NG:
			pending_health -= 12;
	}
}

void ActorLifebar::Run(double delta)
{
	time += delta;
	if (pending_health != 0)
	{
		Health += pending_health * delta;
		pending_health -= pending_health * delta;

		if (pending_health > 200) // only up to 2x health
			pending_health = 200;

		/* Accomulate health. Better you do, more forviging to mistakes. */
		if (Health > 100)
		{
			pending_health += Health - 100;
			Health = 100;
		}
	}

	UpdateHealth();
}