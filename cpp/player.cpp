#include "player.h"

void cPlayer::start()
{

}

void cPlayer::update()
{

}

struct cPlayerCreate : cPlayer::Create
{
	cPlayerPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cPlayer;
	}
}cPlayer_create;
cPlayer::Create& cPlayer::create = cPlayer_create;
