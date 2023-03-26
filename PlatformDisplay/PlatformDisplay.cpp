#include "pch.h"
#include "PlatformDisplay.h"
#include <map>
#include <iostream>

BAKKESMOD_PLUGIN(PlatformDisplay, "write a plugin description here", plugin_version, PLUGINTYPE_FREEPLAY)

std::map<int, std::string> PlatformMap{
	{ 0,  "[Unknown]" },
	{ 1,  "[Steam]"   },
	{ 2,  "[PS4]"     },
	{ 3,  "[PS4]"     },
	{ 4,  "[XboxOne]" },
	{ 6,  "[Switch]"  },
	{ 7,  "[Switch]"  },
	{ 9,  "[Deleted]" },
	{ 11, "[Epic]"    },
};
std::map<int, int> PlatformImageMap{
	{ 0,  0 },
	{ 1,  1 },
	{ 2,  2 },
	{ 3,  2 },
	{ 4,  3 },
	{ 6,  4 },
	{ 7,  4 },
	{ 9,  0 },
	{ 11, 5 }
};
float scale = 0.65f;

// for both teams
// { {"Player1Name", "OS"}, {"Player2Name", "OS"}, {"Player3Name", "OS"}, }
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
std::vector<std::vector<std::string>> BlueTeamValues;
std::vector<std::vector<std::string>> OrangeTeamValues;
std::vector<int> blueTeamLogos;
std::vector<int> orangeTeamLogos;

float Gap = 0.0f;
bool show = false;
std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
void PlatformDisplay::onLoad()
{
	_globalCvarManager = cvarManager;
	for (int i = 0; i < 6; i++) {
		logos[i] = std::make_shared<ImageWrapper>(gameWrapper->GetDataFolder() / "PlatformDisplayImages" / (std::to_string(i) + ".png"), true, false);
		logos[i]->LoadForCanvas();
	}


	//Sounds like a lot of HOOPLA!!!
	cvarManager->registerCvar("PlatformDisplay_BlueTeamPos_X", "524.5", "Set the X position of the Blue teams PlatformDisplay");
	cvarManager->registerCvar("PlatformDisplay_BlueTeamPos_Y", "680", "Set the Y position of the Blue teams PlatformDisplay");
	cvarManager->registerCvar("PlatformDisplay_OrangeTeamPos_X", "1050", "Set the X position of the Orange teams PlatformDisplay");
	cvarManager->registerCvar("PlatformDisplay_OrangeTeamPos_Y", "680", "Set the Y position of the Orange teams PlatformDisplay");
	cvarManager->registerCvar("PlatformDisplay_ColorPickerBlueTeam", "#FFFFFF", "Changes the color of the text for blue team");
	cvarManager->registerCvar("PlatformDisplay_ColorPickerOrangeTeam", "#FFFFFF", "Changes the color of the text for Orange Team");
	//Make thing go yes
	gameWrapper->RegisterDrawable([this](CanvasWrapper canvas) {
		Render(canvas);
	});

	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed", [this](std::string eventName) {
		BlueTeamValues.clear();
		OrangeTeamValues.clear();
	});
	gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnOpenScoreboard", [this](std::string eventName) {
		show = true;
		updateScores();
	});
	gameWrapper->HookEvent("Function TAGame.GFxData_GameEvent_TA.OnCloseScoreboard", [this](std::string eventName) {
		show = false;
	});

	//std::bind instead
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Countdown.BeginState", [this](std::string eventName) {
		updateScores();
	});
	gameWrapper->HookEvent("Function TAGame.GameEvent_TA.AddCar", [this](std::string eventName) {
		updateScores();
	});
	//
}

void PlatformDisplay::updateScores(bool keepOrder) {
#ifdef _DEBUG
	if (!gameWrapper->IsInGame() && !gameWrapper->IsInOnlineGame() || gameWrapper->IsInFreeplay() || gameWrapper->IsInReplay()) return;
	ServerWrapper sw = gameWrapper->IsInOnlineGame() ? gameWrapper->GetOnlineGame() : gameWrapper->GetGameEventAsServer();
#else
	if (!gameWrapper->IsInOnlineGame()) return;
	ServerWrapper sw = gameWrapper->GetOnlineGame();
#endif

	if (sw.IsNull()) return;
	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
	if (mmrWrapper.GetCurrentPlaylist() == 6) return;

	int currentPlaylist = mmrWrapper.GetCurrentPlaylist();
	std::vector<std::string> currentUids;
	std::vector<std::string> currentNames;
	bool isThereNew = false;
	int blues = 0;
	int oranges = 0;
	ArrayWrapper<PriWrapper> players = sw.GetPRIs();

	for (size_t i = 0; i < players.Count(); i++)
	{
		PriWrapper priw = players.Get(i);
		if (priw.IsNull()) continue;

		UniqueIDWrapper uidw = priw.GetUniqueIdWrapper();
		int score = priw.GetMatchScore();
		int size = leaderboard.size();
		int index = size;
		int toRemove = -1;
		bool isNew = true;
		bool isBot = priw.GetbBot();

		UniqueIDWrapper uniqueID = priw.GetUniqueIdWrapper(); //here
		OnlinePlatform platform = uniqueID.GetPlatform(); //here
		if (!platform) { continue; }

		int mmr = 0;
		if (mmrs.find(uidw.GetIdString()) != mmrs.end()) { mmr = mmrs[uidw.GetIdString()]; }
		else {
			if (mmrWrapper.IsSynced(uidw, currentPlaylist) && !isBot) mmr = mmrWrapper.GetPlayerMMR(uidw, currentPlaylist);
			mmrs[uidw.GetIdString()] = mmr;
		}

		std::string name = priw.GetPlayerName().ToString(); currentNames.push_back(name);
		currentUids.push_back(uidw.GetIdString());

		for (size_t j = 0; j < size; j++)
		{
			if (index == size) {
				if (leaderboard[j].score < score) {
					index = j;
				}
				else if (leaderboard[j].score == score) {
					if (compareName(mmr, name, mmrs[leaderboard[j].uid.GetIdString()], leaderboard[j].name) && !isBot) index = j;
				}
			}
			if (uidw.GetIdString() == leaderboard[j].uid.GetIdString() && name == leaderboard[j].name) {
				toRemove = j;
				isNew = false;
			}
		}

		if (isNew) { isThereNew = true; getNamesAndPlatforms(); }
		if (toRemove > -1 && !keepOrder) {
			leaderboard.erase(leaderboard.begin() + toRemove);
			if (index > toRemove) index--;
		}
		if (isNew || !keepOrder) {
			leaderboard.insert(leaderboard.begin() + index, { uidw, score, priw.GetTeamNum(), isBot, name, platform });
		}
		else {
			leaderboard[toRemove].team = priw.GetTeamNum();
		}
		if (priw.GetTeamNum() == 0) blues++;
		else if (priw.GetTeamNum() == 1) oranges++;
	}
	num_blues = blues;
	num_oranges = oranges;

	//remove pris, that no longer exist
	if (isThereNew || players.Count() != leaderboard.size()) {
		for (size_t i = leaderboard.size(); i >= 1; i--)
		{
			bool remove = true;
			for (size_t j = 0; j < currentNames.size(); j++)
			{
				if (currentNames[j] == leaderboard[i - 1].name && currentUids[j] == leaderboard[i - 1].uid.GetIdString()) {
					remove = false;
					break;
				}
			}
			// if (remove) { player_ranks.erase(leaderboard[i - 1].uid.GetIdString()); leaderboard.erase(leaderboard.begin() + i - 1); }
		}
	}
}

bool PlatformDisplay::compareName(int mmr1, std::string name1, int mmr2, std::string name2) {
	if (mmr1 < mmr2) return true;
	else if (mmr1 == mmr2) {
		return to_lower(name1).compare(to_lower(name2)) == -1;
	}
	else return false;
}



std::string PlatformDisplay::to_lower(std::string s) {
	std::for_each(s.begin(), s.end(), [this](char& c) {
		c = std::tolower(c);
		});
	return s;
}

void PlatformDisplay::Render(CanvasWrapper canvas) {
	if (show) {
		//get the pos and color for the blue team

		//cvars
		CVarWrapper BlueXLoc = cvarManager->getCvar("PlatformDisplay_BlueTeamPos_X");
		CVarWrapper BlueYLoc = cvarManager->getCvar("PlatformDisplay_BlueTeamPos_Y");
		CVarWrapper BlueColorPicker = cvarManager->getCvar("PlatformDisplay_ColorPickerBlueTeam");
		if (!BlueXLoc) { return; }
		if (!BlueYLoc) { return; }
		if (!BlueColorPicker) { return; }
		// the values
		float BlueX = BlueXLoc.getFloatValue();
		float BlueY = BlueYLoc.getFloatValue();
		LinearColor BlueColor = BlueColorPicker.getColorValue();

		Gap = 0.0f;
		Vector2F BluePos = { BlueX, BlueY };

		CVarWrapper OrangeXLoc = cvarManager->getCvar("PlatformDisplay_OrangeTeamPos_X");
		CVarWrapper OrangeYLoc = cvarManager->getCvar("PlatformDisplay_OrangeTeamPos_Y");
		CVarWrapper OrangeColorPicker = cvarManager->getCvar("PlatformDisplay_ColorPickerOrangeTeam");
		if (!OrangeXLoc) { return; }
		if (!OrangeYLoc) { return; }
		if (!OrangeColorPicker) { return; }
		float OrangeX = OrangeXLoc.getFloatValue();
		float OrangeY = OrangeYLoc.getFloatValue();
		LinearColor OrangeColor = OrangeColorPicker.getColorValue();
		Gap = 0.0f;
		Vector2F OrangePos = { OrangeX, OrangeY };
		canvas.SetPosition(OrangePos);
		canvas.SetColor(OrangeColor);
		
		// move to that pos and set color
		canvas.SetPosition(BluePos);
		canvas.SetColor(BlueColor);
		int it = 0; 
		if (!gameWrapper->IsInOnlineGame()) return;
		ServerWrapper sw = gameWrapper->GetOnlineGame();

		if (sw.IsNull()) return;
		MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
		if (sw.GetbMatchEnded()) return;

		//-----Black Magic, thanks BenTheDan------------
		Vector2 canvas_size = gameWrapper->GetScreenSize();
		if (float(canvas_size.X) / float(canvas_size.Y) > 1.5f) scale = 0.507f * canvas_size.Y / SCOREBOARD_HEIGHT;
		else scale = 0.615f * canvas_size.X / SCOREBOARD_WIDTH;

		uiScale = gameWrapper->GetInterfaceScale();
		mutators = mmrWrapper.GetCurrentPlaylist() == 34;
		Vector2F center = Vector2F{ float(canvas_size.X) / 2, float(canvas_size.Y) / 2 };
		float mutators_center = canvas_size.X - 1005.0f * scale * uiScale;
		if (mutators_center < center.X && mutators) center.X = mutators_center;
		int team_difference = num_blues - num_oranges;
		center.Y += IMBALANCE_SHIFT * (team_difference - ((num_blues == 0) != (num_oranges == 0)) * (team_difference >= 0 ? 1 : -1)) * scale * uiScale;

		image_scale = 0.48f;
		float tier_X = -SCOREBOARD_LEFT - IMAGE_WIDTH * image_scale;
		float tier_Y_blue = -BLUE_BOTTOM + (6 * (4 - num_blues));
		float tier_Y_orange = ORANGE_TOP;
		int div_X = int(std::roundf(center.X + (-SCOREBOARD_LEFT - 100.0f * image_scale) * scale * uiScale));
		image_scale *= scale * uiScale;

		// based on 100x100 img
		//------End Black Magic---------
		int blues = -1;
		int oranges = -1;
		for (auto pri : leaderboard) {
			if (pri.team == 0) {
				blues++;
			}
			else if (pri.team == 1){
				oranges++;
			}
			if (pri.isBot || pri.team > 1) {
				continue;
			}

			std::string playerName = pri.name; // "playername"
			std::string playerOS = PlatformMap.at(pri.platform); // "os"
			std::string playerString = playerOS + playerName; // "[OS]playername"

			float Y;
			if (pri.team == 0) { Y = tier_Y_blue - BANNER_DISTANCE * (num_blues - blues) + 11; }
			else { Y = tier_Y_orange + BANNER_DISTANCE * (oranges); }
			float X = tier_X + 100.0f * 0.48f + 36;

			Y = center.Y + Y * scale * uiScale;
			X = center.X + X * scale * uiScale;
			canvas.SetPosition(Vector2{ (int)X, (int)Y });
			//canvas.FillBox(Vector2{(int)(100.0*image_scale),(int)( 100.0*image_scale)});

			// 2x scale
			//canvas.DrawString(playerString, 2.0, 2.0);
			int platformImage = pri.platform;
			std::shared_ptr<ImageWrapper> image = logos[PlatformImageMap[platformImage]];
			if (image->IsLoadedForCanvas()) {
				canvas.SetPosition(Vector2{ (int)X, (int)Y });
				canvas.DrawTexture(image.get(), 100.0f/48.0f*image_scale); // last bit of scale b/c imgs are 48x48
			}
			else {
				LOG("not loaded for canvas");
			}
			Gap += 25.0f;
			// move the next player down 1
			canvas.SetPosition(Vector2F{ BlueX, BlueY + Gap });
		}

		

		// int oranges = -1;
		/*for (std::vector<std::string> OrangeTeam : OrangeTeamValues) {
			oranges++;
			std::string playerName = OrangeTeam[0];
			std::string playerOS = OrangeTeam[1];

			std::string playerString = playerOS + playerName;

			float Y;
			Y = tier_Y_orange + BANNER_DISTANCE * (oranges);
			float X = tier_X + 100.0f * 0.48f + 37;

			Y = center.Y + Y * scale * uiScale;
			X = center.X + X * scale * uiScale;
			canvas.SetPosition(Vector2{ (int)X, (int)Y });
			canvas.FillBox(Vector2{ (int)(100.0 * image_scale),(int)(100.0 * image_scale) });

			canvas.DrawString(playerString, 2.0, 2.0);
			int platformImage = orangeTeamLogos[it];
			std::shared_ptr<ImageWrapper> image = logos[PlatformImageMap[platformImage]];
			if (image->IsLoadedForCanvas()) {
				canvas.SetPosition(Vector2F {OrangeX - (float)(48 * scale), OrangeY + Gap - 5});
				canvas.DrawTexture(image.get(), scale);
			}
			else {
				LOG("not loaded for canvas");
			}

			Gap += 25.0f;
			canvas.SetPosition(Vector2F{ OrangeX, OrangeY + Gap });
		}*/
	}
}
void PlatformDisplay::getNamesAndPlatforms() {
	//reset the vectors so it doesnt grow
	BlueTeamValues.clear();
	OrangeTeamValues.clear();
	orangeTeamLogos.clear();
	blueTeamLogos.clear();

	//get server
	ServerWrapper server = gameWrapper->GetCurrentGameState();
	if (!server) { return; }

	//get the cars in the sever
	ArrayWrapper<CarWrapper> cars = server.GetCars(); // here

	//for car in {}
	for (CarWrapper car : cars) {


		// idk what a pri is but we get it here
		PriWrapper pri = car.GetPRI();
		if (!pri) { return; }

		//get the uniqueID and the platform -> int <0-13>
		UniqueIDWrapper uniqueID = pri.GetUniqueIdWrapper(); //here
		OnlinePlatform platform = uniqueID.GetPlatform(); //here
		if (!platform) { return; }

		// declare in iterator and an os string
		std::map<int, std::string>::iterator it;
		std::string OS;

		//wanting to check what team the player is on so we can add them to the right vector
		int teamNum = car.GetTeamNum2();
		//get the owner name
		std::string name = car.GetOwnerName();
		//for len
		for (it = PlatformMap.begin(); it != PlatformMap.end(); it++) {
			
			int key = it->first; //int <0-12>

			if (key == platform) {
				OS = it->second; // what ever the number is mapped to
				break;
			}
		}
		//add correspondingly with the name and os
		if (teamNum == 0) { BlueTeamValues.push_back({ name, OS }); blueTeamLogos.push_back(platform); }
		if (teamNum == 1) { OrangeTeamValues.push_back({ name, OS }); orangeTeamLogos.push_back(platform); }

	}
}
void PlatformDisplay::onUnload()
{
}
