#include "source.h"
#include "impl/source_impl.h"

namespace audio
{
source::source()
	: _impl(std::make_unique<priv::source_impl>())
{
}

source::~source() = default;

source::source(source&& rhs)
	: _impl(std::move(rhs._impl))
{
	rhs._impl = nullptr;
}

source& source::operator=(source&& rhs)
{
	_impl = std::move(rhs._impl);
	rhs._impl = nullptr;

	return *this;
}

void source::play()
{
	if(_impl)
	{
		_impl->play();
	}
}

void source::stop()
{
	if(_impl)
	{
		_impl->stop();
	}
}

void source::pause()
{
	if(_impl)
	{
		_impl->pause();
	}
}

bool source::is_playing() const
{
	return _impl && _impl->is_playing();
}

bool source::is_paused() const
{
	if(_impl)
	{
		return _impl->is_paused();
	}
	return false;
}

bool source::is_stopped() const
{
	if(_impl)
	{
		return _impl->is_stopped();
	}
	return true;
}

bool source::is_looping() const
{
	if(_impl)
	{
		return _impl->is_looping();
	}
	return false;
}

void source::set_loop(bool on)
{
	if(_impl)
	{
		_impl->set_loop(on);
	}
}

void source::set_volume(float volume)
{
	if(_impl)
	{
		_impl->set_volume(volume);
	}
}

/* pitch, speed stretching */
void source::set_pitch(float pitch)
{
	// if pitch == 0.f pitch = 0.0001f;
	if(_impl)
	{
		_impl->set_pitch(pitch);
	}
}

void source::set_position(const float* position3)
{
	if(_impl)
	{
		_impl->set_position(position3);
	}
}

void source::set_velocity(const float* velocity3)
{
	if(_impl)
	{
		_impl->set_velocity(velocity3);
	}
}

void source::set_orientation(const float* direction3, const float* up3)
{
	if(_impl)
	{
		_impl->set_orientation(direction3, up3);
	}
}

void source::set_volume_rolloff(float rolloff)
{
	if(_impl)
	{
		_impl->set_volume_rolloff(rolloff);
	}
}

void source::set_distance(float mind, float maxd)
{
	if(_impl)
	{
		_impl->set_distance(mind, maxd);
	}
}

void source::set_playing_offset(sound_data::duration_t offset)
{
	if(_impl)
	{
		_impl->set_playing_offset(float(offset.count()));
	}
}

sound_data::duration_t source::get_playing_offset() const
{
	if(_impl)
	{
		return sound_data::duration_t(_impl->get_playing_offset());
	}
	return sound_data::duration_t(0);
}

sound_data::duration_t source::get_playing_duration() const
{
	if(_impl)
	{
		return sound_data::duration_t(_impl->get_playing_duration());
	}
	return sound_data::duration_t(0);
}

bool source::is_valid() const
{
	return _impl && _impl->is_valid();
}

void source::bind(const sound& snd)
{
	if(is_valid())
	{
		_impl->bind(snd._impl.get());
	}
}
}