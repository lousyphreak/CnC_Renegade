/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/****************************************************************************\
*        C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S         *
******************************************************************************
Project Name: Generic Game Results Server
File Name   : rengameres.h
Author      : Joe Howes (jhowes@westwood.com)
Start Date  : Apr 5, 2000
Last Update :
------------------------------------------------------------------------------

Wraps game results for a game of multiplayer Renegade and provides method
for sending to a WOL game results server.

\****************************************************************************/

#ifndef __RENGAMERES_H_
#define __RENGAMERES_H_

#include <cstdint>


/*----------------------------------------------------------------------.
| MACROS                                                                |
`----------------------------------------------------------------------*/
//#define GRSETTING_USING_WOLAPI				// Uncomment if the class is
												// being used with the game
												// (as opposed to the test applet)
static uint32_t GR_SCORE_SCALE = 10000000;
static uint32_t GR_BASE_SKU = 8704;

// Errors
static const int GR_ERROR_BIND_FAILED		= -100;
static const int GR_ERROR_CONNECT_FAILED	= -101;

// Game Specific
static char GR_GAME_ID[5]			= { "IDNO" };
static char GR_PLAYER_COUNT[5]		= { "PLRS" };	
static char GR_CLAN_GAME[5]			= { "CLGM" };	
static char GR_DURATION[5]			= { "DURA" };	
static char GR_MAP_NAME[5]			= { "MPNM" };	
static char GR_SKU[5]				= { "GSKU" };	
static char GR_STYLE[5]				= { "GSTY" };	
static char GR_NUM_CLANS[5]			= { "NTMS" };	
static char GR_START_TIME[5]		= { "TIME" };	
static char GR_TOURNAMENT[5]		= { "TRNY" };	

// Player Specific
static char GR_LOGINS[5]			= { "LGL?" };
static char GR_SCORES[5]			= { "SCO?" };	
static char GR_CLANIDS[5]			= { "CLN?" };
static char GR_DURATIONS[5]			= { "DUR?" };
static char GR_IPS[5]				= { "IPL?" };
static char GR_DEATHS[5]			= { "DTH?" };
static char GR_KILLS[5]				= { "KIL?" };
static char GR_SELFKILLS[5]			= { "SKL?" };
static char GR_DAMAGEPOINTS[5]		= { "DMP?" };


/*----------------------------------------------------------------------.
| DATATYPES																|
`----------------------------------------------------------------------*/
typedef enum gamestyle
{
	GR_DEATHMATCH = 0, 
	GR_CAPTUREFLAG, 
	GR_FLAGBALL,
	GR_KINGREALM,
	GR_HIGHLANDER,
	GR_GAUNTLET,

	GR_NUM_GAMESTYLES
} GameStyle;

typedef enum language
{
	GR_ENGLISH = 0,
	GR_UK_ENGLISH,
	GR_GERMAN,
	GR_FRENCH,
	GR_DUTCH,
	GR_ITALIAN,
	GR_JAPANESE,
	GR_SPANISH,
	GR_SCANDINAVIAN,
	GR_KOREAN,

	NUM_LANGUAGES
} Language;


class RenegadeGameRes
{
public:
	// METHODS
	RenegadeGameRes(const char* host = NULL, int port = 0) :
					    _host(NULL), _port(0),
						_myplayercount(0), _game_id(0), _player_count(0), _clan_game(0), 
						_duration(0), _map_name(NULL), _sku(0), _style(GR_DEATHMATCH), 
						_num_clans(0), _start_time(0), _tournament(0), _logins(NULL), 
						_scores(NULL), _clan_ids(NULL), _durations(NULL), _ips(NULL), 
						_deaths(NULL), _kills(NULL), _selfkills(NULL), _damagepoints(NULL)
	{
		if( host != NULL )
		{
			setHost(host);
			setPort(port);
		}
	}

	~RenegadeGameRes();


	void setHost(const char* host)				
	{ 
		if( _host != NULL )
			delete[] _host;
		_host = new char[strlen(host)+1];	
		strcpy(_host, host); 
	}
	void setPort(int val)						{ _port = val; }
	void setGameID(uint32_t val)		{ _game_id = val; }	
	void setPlayerCount(uint8_t val)		{ _player_count = val; }	
	void setClanGame(uint8_t val)			{ _clan_game = val; }	
	void setDuration(uint32_t val)		{ _duration = val; }	
	void setMapName(const char* val);
	void setSKU(Language lang)					{ _sku = GR_BASE_SKU | lang; }	
	void setStyle(GameStyle val)				{ _style = (uint8_t)val; }	
	void setNumClans(uint8_t val)			{ _num_clans = val; }	
	void setStartTime(uint32_t val)	{ _start_time = val; }	
	void setTournament(uint8_t val)		{ _tournament = val; }	

	void addPlayer(const char* login = "INVALID", double score = 0.0, uint32_t clan_id = 0,
				   uint32_t duration = 0, uint32_t ip = 0, 
				   uint32_t deaths = 0, uint32_t kills = 0, 
				   uint32_t selfkills = 0, uint32_t damagepoints = 0);

	int sendResults();



private:	
	// METHODS
	char** _addToArr(char** arr, const char* item);
	uint32_t* _addToArr(uint32_t* arr, uint32_t item);


	// MEMBERS
	char*				_host;
	int					_port;
	int					_myplayercount;

	// Game Specific
	uint32_t	_game_id;
	uint8_t		_player_count;
	uint8_t		_clan_game;				// Boolean
	uint32_t	_duration;				// Secs since epoch
	char*				_map_name;				// Must be NULL terminated
	uint32_t	_sku;
	uint8_t		_style;					// Will be converted to an uint8_t
	uint8_t		_num_clans;
	uint32_t	_start_time;			// Secs since epoch
	uint8_t		_tournament;			// Boolean

	// Player Specific  (These are all arrays)
	char**				_logins;				// Must be NULL terminated
	uint32_t*	_scores;
	uint32_t*	_clan_ids;
	uint32_t*	_durations;				// Secs since epoch
	uint32_t*	_ips;					// As integers, not dotted quads
	uint32_t*	_deaths;
	uint32_t*	_kills;
	uint32_t*	_selfkills;
	uint32_t*	_damagepoints;	
};


#endif
