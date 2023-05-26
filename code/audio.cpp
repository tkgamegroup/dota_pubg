#include <flame/audio/source.h>

#include "audio.h"

// the sources that play in everywhere, usually for ui and bgm
static std::map<audio::BufferPtr, audio::SourcePtr> global_sources;

void play_global_sound(audio::BufferPtr buffer, float volume)
{
	auto it = global_sources.find(buffer);
	if (it == global_sources.end())
	{
		auto source = audio::Source::create();
		source->add_buffer(buffer);
		it = global_sources.emplace(buffer, source).first;
	}

	it->second->stop();
	it->second->set_volumn(volume);
	it->second->play();
}
