#include "source_impl.h"
#include "../logger.h"
#include "check.h"
#include "sound_impl.h"

namespace audio
{
namespace priv
{
source_impl::source_impl()
{
	create();
}

source_impl::~source_impl()
{
	purge();
}

source_impl::source_impl(source_impl&& rhs)
	: _handle(std::move(rhs._handle))
{
	rhs._handle = 0;
}

source_impl& source_impl::operator=(source_impl&& rhs)
{
	_handle = std::move(rhs._handle);
	rhs._handle = 0;

	return *this;
}

bool source_impl::create()
{
	if(_handle)
		return true;

	al_check(alGenSources(1, &_handle));

	return _handle != 0;
}

bool source_impl::bind(sound_impl* sound)
{
	if(sound == nullptr)
	{
		return true;
	}

	unbind();

	bind_sound(sound);

	const auto buffer = sound->native_handle();

	al_check(alSourcei(_handle, AL_SOURCE_RELATIVE, AL_FALSE));

	al_check(alSourcei(_handle, AL_BUFFER, ALint(buffer)));

	// optional info
	ALint channels = 1;
	al_check(alGetBufferi(buffer, AL_CHANNELS, &channels));
	if(channels > 1)
	{
		log_info("Sound is not mono. 3D Attenuation will not work.");
	}

	return true;
}

void source_impl::unbind()
{
	stop();

	al_check(alSourcei(_handle, AL_BUFFER, 0));

	unbind_sound();
}

void source_impl::purge()
{
	if(!_handle)
		return;

	unbind();

	al_check(alDeleteSources(1, &_handle));

	_handle = 0;
}

void source_impl::set_playing_offset(float seconds)
{
	al_check(alSourcef(_handle, AL_SEC_OFFSET, seconds));
}

float source_impl::get_playing_offset() const
{
	ALfloat seconds = 0.0f;
	al_check(alGetSourcef(_handle, AL_SEC_OFFSET, &seconds));
	return static_cast<float>(seconds);
}

float source_impl::get_playing_duration() const
{
	sound_impl::native_handle_type buffer = 0;
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if(_bound_sound == nullptr)
			return 1.0f;
		buffer = _bound_sound->native_handle();
	}

	ALint size_in_bytes = 0;
	ALint channels = 1;
	ALint bits = 1;
	ALint frequency = 1;

	al_check(alGetBufferi(buffer, AL_SIZE, &size_in_bytes));
	al_check(alGetBufferi(buffer, AL_CHANNELS, &channels));
	al_check(alGetBufferi(buffer, AL_BITS, &bits));
	al_check(alGetBufferi(buffer, AL_FREQUENCY, &frequency));

	const auto length_in_samples = (size_in_bytes * 8) / (channels * bits);
	return float(length_in_samples) / float(frequency);
}

void source_impl::play() const
{
	al_check(alSourcePlay(_handle));
}

void source_impl::stop() const
{
	al_check(alSourceStop(_handle));
}

void source_impl::pause() const
{
	al_check(alSourcePause(_handle));
}

bool source_impl::is_playing() const
{
	ALint state = AL_INITIAL;
	al_check(alGetSourcei(_handle, AL_SOURCE_STATE, &state));
	return (state == AL_PLAYING);
}

bool source_impl::is_paused() const
{
	ALint state = AL_INITIAL;
	al_check(alGetSourcei(_handle, AL_SOURCE_STATE, &state));
	return (state == AL_PAUSED);
}

bool source_impl::is_stopped() const
{
	ALint state = AL_INITIAL;
	al_check(alGetSourcei(_handle, AL_SOURCE_STATE, &state));
	return (state == AL_STOPPED);
}

bool source_impl::is_binded() const
{
	ALint buffer = 0;
	al_check(alGetSourcei(_handle, AL_BUFFER, &buffer));
	return (buffer != 0);
}

void source_impl::set_loop(bool on)
{
	al_check(alSourcei(_handle, AL_LOOPING, on ? AL_TRUE : AL_FALSE));
}

void source_impl::set_volume(float volume)
{
	al_check(alSourcef(_handle, AL_GAIN, volume));
}

/* pitch, speed stretching */
void source_impl::set_pitch(float pitch)
{
	// if pitch == 0.f pitch = 0.0001f;
	al_check(alSourcef(_handle, AL_PITCH, pitch));
}

void source_impl::set_position(const float* position3)
{
	al_check(alSourcefv(_handle, AL_POSITION, position3));
}

void source_impl::set_velocity(const float* velocity3)
{
	al_check(alSourcefv(_handle, AL_VELOCITY, velocity3));
}

void source_impl::set_orientation(const float* direction3, const float* up3)
{
	float orientation6[] = {-direction3[0], -direction3[1], -direction3[2], up3[0], up3[1], up3[2]};
	al_check(alSourcefv(_handle, AL_ORIENTATION, orientation6));
}

void source_impl::set_volume_rolloff(float rolloff)
{
	al_check(alSourcef(_handle, AL_ROLLOFF_FACTOR, rolloff));
}

void source_impl::set_distance(float mind, float maxd)
{

	// The distance that the source will be the loudest (if the listener is
	// closer, it won't be any louder than if they were at this distance)
	al_check(alSourcef(_handle, AL_REFERENCE_DISTANCE, mind));

	// The distance that the source will be the quietest (if the listener is
	// farther, it won't be any quieter than if they were at this distance)
	al_check(alSourcef(_handle, AL_MAX_DISTANCE, maxd));
}

bool source_impl::is_valid() const
{
	return _handle != 0;
}

bool source_impl::is_looping() const
{
	ALint loop;
	al_check(alGetSourcei(_handle, AL_LOOPING, &loop));
	return loop != 0;
}

source_impl::native_handle_type source_impl::native_handle() const
{
	return _handle;
}

void source_impl::bind_sound(sound_impl* sound)
{
	std::lock_guard<std::mutex> lock(_mutex);

	if(_bound_sound == sound)
		return;

	_bound_sound = sound;
	_bound_sound->bind_to_source(this);
}

void source_impl::unbind_sound()
{
	std::lock_guard<std::mutex> lock(_mutex);

	if(_bound_sound)
	{
		_bound_sound->unbind_from_source(this);
		_bound_sound = nullptr;
	}
}
}
}