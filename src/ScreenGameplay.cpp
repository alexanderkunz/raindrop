#include "Global.h"
#include <boost/format.hpp>
#include "Game_Consts.h"
#include "ScreenGameplay.h"
#include "ScreenEvaluation.h"
#include "NoteLoader.h"
#include "GraphicsManager.h"
#include "ImageLoader.h"
#include "Audio.h"

#define ComboSizeX 40
#define ComboSizeY 64

ScreenGameplay::ScreenGameplay(IScreen *Parent) :
	IScreen(Parent),
	Barline(this)
{
	Running = true;
	IsAutoplaying = false;
	ShouldChangeScreenAtEnd = true;
	SongInfo.LoadSkinFontImage("font.tga", glm::vec2(18, 32), glm::vec2(34,34), glm::vec2(10,16), 32);
	MyFont.LoadSkinFontImage("font.tga", glm::vec2(18, 32), glm::vec2(34,34), glm::vec2(ComboSizeX,ComboSizeY), 32);
	Music = NULL;
	FailEnabled = true;
	TappingMode = false;
	EditMode = false;
}

void ScreenGameplay::RemoveTrash()
{
	NotesHeld.clear();
	AnimateOnly.clear();
	NotesInMeasure.clear();
}



void ScreenGameplay::Cleanup()
{
	// Deleting the song's notes is ScreenSelectMusic's (or fileman's) job.
	if (Music != NULL)
	{
		MixerRemoveStream(Music);
		delete Music;
		Music = NULL;
	}
}


glm::vec2 ScreenGameplay::GetScreenOffset(float Alignment)
{
	float outx; 
	float outy;

	outx = (GraphMan.GetMatrixSize().x*Alignment);
	outy = (GraphMan.GetMatrixSize().y*Alignment);

	return glm::vec2(outx, outy);
}

void ScreenGameplay::StoreEvaluation(Judgement Eval)
{
	if (Eval > Bad || Eval == OK)
		Combo++;

	switch (Eval)
	{
	case Excellent:
		Evaluation.NumExcellents++;
		break;
	case Perfect:
		Evaluation.NumPerfects++;
		break;
	case Great:
		Evaluation.NumGreats++;
		break;
	case Bad:
		Evaluation.NumBads++;
		Combo = 0;
		break;
	case Miss:
		Evaluation.NumMisses++;
		Combo = 0;
		break;
	case NG:
		Evaluation.NumNG++;
		Combo = 0;
		break;
	case OK:
		Evaluation.NumOK++;
		break;
	case None:
		break;
	}
	Evaluation.MaxCombo = std::max(Evaluation.MaxCombo, Combo);
}

void ScreenGameplay::Init(Song *OtherSong, uint32 DifficultyIndex)
{
	MySong = OtherSong;
	CurrentDiff = MySong->Difficulties[DifficultyIndex];
	
	memset(&Evaluation, 0, sizeof(Evaluation));

	Measure = 0;
	Combo = 0;

	Cursor.setImage(ImageLoader::LoadSkin("cursor.png"));
	Barline.setImage(ImageLoader::LoadSkin("Barline.png"));
	MarkerA.setImage(ImageLoader::LoadSkin("barline_marker.png"));
	MarkerB.setImage(ImageLoader::LoadSkin("barline_marker.png"));
	Background.setImage(ImageLoader::Load(MySong->BackgroundDir));

	Background.alpha = 0.8f;
	Background.width = ScreenWidth;
	Background.height = ScreenHeight;
	Background.position.x = 0;
	Background.position.y = 0;
	Background.InitTexture();

	MarkerB.Centered = true;
	MarkerA.Centered = false;

	MarkerA.position.x = MarkerB.position.x = GetScreenOffset(0.5).x;
	MarkerA.rotation = 180;

	MarkerB.position.y = PlayfieldHeight + ScreenOffset + MarkerB.height/2 + Barline.height/2;
	MarkerA.position.x = 2*MarkerA.position.x;
	MarkerA.position.y = MarkerA.height - Barline.height/2;
	Lifebar.width = PlayfieldWidth;
	MarkerA.InitTexture();
	MarkerB.InitTexture();

	if (ShouldChangeScreenAtEnd)
		Barline.Init(CurrentDiff->Offset);
	else
	{
		Barline.Init(0); // edit mode
		Barline.alpha = 1;
	}

	
	Lifebar.UpdateHealth();

	if (CurrentDiff->Timing.size())
		MeasureTime = (60 * 4 / CurrentDiff->Timing[0].Value);
	else
		MeasureTime = 0;

	// We might be retrying- in that case we should probably clean up.
	RemoveTrash();

	// todo: not need to copy this whole thing. 
	NotesInMeasure.resize(CurrentDiff->Measures.size());
	for (uint32 i = 0; i < CurrentDiff->Measures.size(); i++)
	{
		NotesInMeasure[i] = CurrentDiff->Measures[i].MeasureNotes;
	}

	if (!Music)
	{
		Music = new VorbisStream(MySong->SongFilename.c_str());

		if (!Music || !Music->IsOpen())
		{
			// we can't use exceptions because they impact the framerate. What can we do?
			// throw std::exception( (boost::format ("couldn't open song %s") % MySong->SongFilename).str().c_str() );
		}
		
		MixerAddStream(Music);
		seekTime(0);
	}

	MeasureTimeElapsed = 0;
	ScreenTime = 0;

	Cursor.width = Cursor.height = 72;
	Cursor.Centered = true;
	Cursor.InitTexture();
}

int32 ScreenGameplay::GetMeasure()
{
	return Measure;
}

/* Note stuff */

void ScreenGameplay::RunVector(std::vector<GameObject>& Vec, float TimeDelta)
{
	Judgement Val;
	// For each note in current measure
	for (std::vector<GameObject>::iterator i = Vec.begin(); 
		i != Vec.end(); 
		i++)
	{
		// Run the note.
		if ((Val = i->Run(TimeDelta, SongTime, IsAutoplaying)) != None)
		{
			Lifebar.HitJudgement(Val);
			aJudgement.ChangeJudgement(Val);
			StoreEvaluation(Val);
			break;
		}
	}
}

void ScreenGameplay::RunMeasure(float delta)
{
	if (Measure < CurrentDiff->Measures.size())
	{
		RunVector(NotesInMeasure[Measure], delta);

		// For each note in PREVIOUS measure (when not in edit mode)
		if (Measure > 0 && !EditMode)
			RunVector(NotesInMeasure[Measure-1], delta);

		// Run the ones in the NEXT measure.
		if (Measure+1 < NotesInMeasure.size())
			RunVector(NotesInMeasure[Measure+1], delta);
	}

	if (NotesHeld.size() > 0)
	{
		Judgement Val;
		for (std::vector<GameObject>::iterator i = NotesHeld.begin();
			i != NotesHeld.end();
			i++)
		{
			// See if something's going on with the hold.
			if ((Val = i->Run(delta, SongTime, IsAutoplaying)) != None)
			{
				// Judge accordingly..
				Lifebar.HitJudgement(Val);
				aJudgement.ChangeJudgement(Val);
				StoreEvaluation(Val);
				AnimateOnly.push_back(*i); // Animate this one.
				i = NotesHeld.erase(i);
				break;
			}
		}
	}
}

void ScreenGameplay::HandleInput(int32 key, int32 code, bool isMouseInput)
{
	if (Next && Next->IsScreenRunning())
	{
		Next->HandleInput(key, code, isMouseInput);
		return;
	}

	/* Notes */
	if (Measure < CurrentDiff->Measures.size() && // if measure is playable
		(((key == 'Z' || key == 'X') && !isMouseInput) || // key is z or x and it's not mouse input or
		(isMouseInput && (key == GLFW_MOUSE_BUTTON_LEFT || key == GLFW_MOUSE_BUTTON_RIGHT))) // is mouse input and it's a mouse button..
		)
	{

		if (code == GLFW_PRESS)
			Cursor.scaleX = Cursor.scaleY = 0.85;
		else
			Cursor.scaleX = Cursor.scaleY = 1;

		// For all measure notes..
		if (Measure > 0 && JudgeVector(NotesInMeasure[Measure-1], code, key))
			return;
			

		if (JudgeVector(NotesInMeasure[Measure], code, key))
			return;

		if (Measure+1 < NotesInMeasure.size() && JudgeVector(NotesInMeasure[Measure+1], code, key))
			return;

		Judgement Val;
		glm::vec2 mpos = GraphMan.GetRelativeMPos();
		// For all held notes...
		for (std::vector<GameObject>::iterator i = NotesHeld.begin();
			i != NotesHeld.end();
			i++)
		{
			// See if something's going on with the hold.
			if ((Val = i->Hit(SongTime, mpos, code == GLFW_PRESS ? true : false, IsAutoplaying, key)) != None)
			{
				// Judge accordingly..
				Lifebar.HitJudgement(Val);
				aJudgement.ChangeJudgement(Val);
				StoreEvaluation(Val);
				i = NotesHeld.erase(i); // Delete this object. The hold is done!
				break;
			}
		}
		return;
	}

	/* Functions */
	if (code == GLFW_PRESS)
	{
#ifndef NDEBUG
		if (key == 'R') // Retry
		{
			Cleanup();
			Init(MySong, 0);
			return;
		}
#endif

		if (key == 'A') // Autoplay
		{
			IsAutoplaying = !IsAutoplaying;
		}else if (key == 'F')
		{
			FailEnabled = !FailEnabled;
		}
		else if (key == 'T')
		{
			TappingMode = !TappingMode;
		}
		else if (key == GLFW_KEY_ESC)
		{
			Running = false;
			Cleanup();
		}
	}
}

void ScreenGameplay::seekTime(float Time)
{
	Music->seek(Time);
	SongTime = Time - GetDeviceLatency();
	SongTimeLatency = Time;
	ScreenTime = 0;
}

void ScreenGameplay::startMusic()
{
	Music->Start();
}

void ScreenGameplay::stopMusic()
{
	Music->Stop();
}

/* TODO: Use measure ratios instead of song time for the barline. */
bool ScreenGameplay::Run(double TimeDelta)
{
	ScreenTime += TimeDelta;
	
	if (Music)
	{
		SongDelta = Music->GetPlaybackTime() - SongTimeLatency;
		SongTime += SongDelta;
		SongTimeLatency += SongDelta;
	}

	if ( ScreenTime > ScreenPauseTime || !ShouldChangeScreenAtEnd ) // we're over the pause?
	{
		if (SongTime <= 0)
			startMusic();

		if (Next) // We have a pending screen?
			return RunNested(TimeDelta); // use that instead.

		RunMeasure(SongDelta);

		Barline.Run(SongDelta, MeasureTime, MeasureTimeElapsed);

		if (SongTime > CurrentDiff->Offset)
		{
			MeasureTimeElapsed += SongDelta;

			if (SongDelta == 0)
			{
				if (!Music || Music->IsStopped())
					MeasureTimeElapsed += TimeDelta;
			}
			if (MeasureTimeElapsed > MeasureTime)
			{
				MeasureTimeElapsed -= MeasureTime;
				Measure += 1;
			}
			Lifebar.Run(SongDelta);
			aJudgement.Run(TimeDelta);
		}

	}

	RenderObjects(TimeDelta);

	// You died? Not an infinite screen? Failing is enabled?
	if (Lifebar.Health <= 0 && ShouldChangeScreenAtEnd && FailEnabled)
		Running = false; // It's over.

	return Running;
}

void ScreenGameplay::DrawVector(std::vector<GameObject>& Vec, float TimeDelta)
{
	for (std::vector<GameObject>::reverse_iterator i = Vec.rbegin(); 
		i != Vec.rend(); 
		i++)
	{
		i->Animate(TimeDelta, SongTime);
		i->Render();
	}
}
	
bool ScreenGameplay::JudgeVector(std::vector<GameObject>& Vec, int code, int key)
{
	Judgement Val;
	glm::vec2 mpos = GraphMan.GetRelativeMPos();

	for (std::vector<GameObject>::iterator i = Vec.begin(); 
			i != Vec.end(); 
			i++)
		{
			// See if it's a hit.
			if ((Val = i->Hit(SongTime, TappingMode ? i->position : mpos, code == GLFW_PRESS ? true : false, IsAutoplaying, key)) != None)
			{
				// Judge accordingly.
				Lifebar.HitJudgement(Val);
				aJudgement.ChangeJudgement(Val);
				StoreEvaluation(Val);

				// If it's a hold, keep running it until it's done.
				if (i->endTime && Val > Miss)
				{
					NotesHeld.push_back(*i);
					i = Vec.erase(i); // These notes are off the measure. We'll handle them somewhere else.
				}
				if (TappingMode)
					return true;
				else
					return false;
			}
		}
	return false;
}


void ScreenGameplay::RenderObjects(float TimeDelta)
{
	glm::vec2 mpos = GraphMan.GetRelativeMPos();
	Cursor.position = mpos;

	Cursor.rotation += 140 * TimeDelta;
	if (Cursor.rotation > 360)
		Cursor.rotation -= 360;

	// Rendering ahead.
	Background.Render();
	MarkerA.Render();
	MarkerB.Render();
	Lifebar.Render();

	DrawVector(NotesHeld, TimeDelta);
	DrawVector(AnimateOnly, TimeDelta);

	if (Measure > 0)
	{
		if (NotesInMeasure.size() && // there are measures and
			Measure-1 < NotesInMeasure.size() && // the measure is within the range and
			NotesInMeasure.at(Measure-1).size() > 0) // there are notes in this measure
		{
			DrawVector(NotesInMeasure[Measure-1], TimeDelta);
		}
	}

	// Render current measure on front of the next!
	if (Measure + 1 < CurrentDiff->Measures.size())
	{
		if (NotesInMeasure.size() && NotesInMeasure.at(Measure+1).size() > 0)
		{
			DrawVector(NotesInMeasure[Measure+1], TimeDelta);
		}
	}

	if (Measure < CurrentDiff->Measures.size())
	{
		if (NotesInMeasure.size() && NotesInMeasure.at(Measure).size() > 0)
		{
			DrawVector(NotesInMeasure[Measure], TimeDelta);
		}
	}else
	{
		if (ShouldChangeScreenAtEnd)
		{
			ScreenEvaluation *Eval = new ScreenEvaluation(this);
			Eval->Init(Evaluation);
			Next = Eval;
			Music->Stop();
		}
	}

	Barline.Render();
	aJudgement.Render();

	// Combo rendering.
	std::stringstream str;
	str << Combo;

	float textX = GetScreenOffset(0.5).x - (str.str().length() * ComboSizeX / 2);
	MyFont.DisplayText(str.str().c_str(), glm::vec2(textX, 0));

	/* Lengthy information printing code goes here.*/
	std::stringstream info;

	if (IsAutoplaying)
		info << "\nAutoplay";

#ifndef NDEBUG
	info << "\nSongTime: " << SongTime << "\nPlaybackTime: ";
	if (Music)
		info << Music->GetPlaybackTime();
	else
		info << "???";
	info << "\nStreamTime: ";
	if(Music)
		info << Music->GetStreamedTime();
	else
		info << "???";

	info << "\nSongDelta: " << SongDelta;
	info << "\nTimeBuffered: ";
	if (Music)
		info << Music->GetStreamedTime() - Music->GetPlaybackTime();
	else
		info << "???";

	info << "\nScreenTime: " << ScreenTime;
#endif
	if (TappingMode)
		info << "\nTapping mode";
	if (!FailEnabled)
		info << "\nFailing Disabled";

#ifdef NDEBUG
	if (EditMode)
#endif
		info << "\nMeasure: " << Measure;
	SongInfo.DisplayText(info.str().c_str(), glm::vec2(0,0));

	Cursor.Render();
}