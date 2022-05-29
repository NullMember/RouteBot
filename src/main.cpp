#include <dpp/dpp.h>
#include <RtAudio.h>
#include <RingBuffer.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>

#define MAX_CHANNELS                16
#define BUFFER_SIZE_COEFF           1024
#define PREFERRED_BUFFER_SIZE       128
#define PREFERRED_SAMPLE_RATE       48000

typedef struct ChannelData
{
    dpp::snowflake user_id;
    bool acquired;
    const size_t channel;
    RingBuffer<int16_t> ring_buffer;
} ChannelData_t;

inline int get_channel_num(ChannelData_t** channels, dpp::snowflake user_id) {
    for (size_t i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i]->user_id == user_id) {
            return i;
        }
    }
    return -1;
}

inline int claim_empty_channel(ChannelData_t** channels, dpp::snowflake user_id) {
    for (size_t i = 0; i < MAX_CHANNELS; i++) {
        if(!channels[i]->acquired) {
            channels[i]->user_id = user_id;
            channels[i]->acquired = true;
            channels[i]->ring_buffer.flush();
            return channels[i]->channel;
        }
    }
    return -1;
}

inline int claim_channel(ChannelData_t** channels, dpp::snowflake user_id, size_t channel) {
    if (channel < MAX_CHANNELS) {
        channels[channel]->user_id = user_id;
        channels[channel]->acquired = true;
        channels[channel]->ring_buffer.flush();
        return channels[channel]->channel;
    }
    return -1;
}

inline void unclaim_channel_with_userid(ChannelData_t** channels, dpp::snowflake user_id) {
    for (size_t i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i]->user_id == user_id) {
            channels[i]->acquired = false;
        }
    }
}

inline void unclaim_channel_with_channelnum(ChannelData_t** channels, size_t channel) {
    if (channel < MAX_CHANNELS) {
        channels[channel]->acquired = false;
    }
}

inline void unclaim_channel_all(ChannelData_t** channels) {
    for (size_t i = 0; i < MAX_CHANNELS; i++) {
        channels[i]->acquired = false;
    }
}

int audio_callback(
    void* output_buffer, 
    void* input_buffer, 
    unsigned int frames, 
    double stream_time, 
    RtAudioStreamStatus status, 
    void* user_data) {
        ChannelData_t** channels = (ChannelData_t**)user_data;
        int16_t* input = (int16_t*)input_buffer;
        int16_t* output = (int16_t*)output_buffer;

        for (size_t channel = 0; channel < MAX_CHANNELS; channel++) {
            channels[channel]->ring_buffer.read((output + (channel * frames)), frames);
        }

        return 0;
}

void error_callback(RtAudioError::Type type, const std::string &errorText) {
    std::cout << "RtAudio Error: " << errorText << std::endl;
}

int main(int argc, char const *argv[])
{
    #ifdef __WIN32__
    RtAudio audio(RtAudio::Api::WINDOWS_ASIO);
    std::string device = "ReaRoute ASIO (x64)";
    #elif __APPLE__
    RtAudio audio(RtAudio::Api::MACOSX_CORE);
    std::string device = "Existential Audio Inc.: BlackHole 16ch";
    #endif
    RtAudio::StreamParameters input_parameters;
    RtAudio::StreamParameters output_parameters;
    RtAudio::StreamOptions stream_options;
    
    unsigned int* buffer_size = new unsigned int;
    *buffer_size = PREFERRED_BUFFER_SIZE; 
    
    ChannelData_t** channels = new ChannelData_t*[MAX_CHANNELS];
    int16_t* conversion_buffer = new int16_t[65536];

    for(unsigned int i = 0; i < audio.getDeviceCount(); i++) {
        auto info = audio.getDeviceInfo(i);
        if(info.probed == true) {
            if (info.name.compare(device) == 0) {
                input_parameters.deviceId = i;
                input_parameters.firstChannel = 0;
                input_parameters.nChannels = info.inputChannels;
                
                output_parameters.deviceId = i;
                output_parameters.firstChannel = 0;
                output_parameters.nChannels = info.outputChannels;
                
                stream_options.flags = RTAUDIO_NONINTERLEAVED;

                audio.openStream(
                    &output_parameters, 
                    &input_parameters, 
                    RTAUDIO_SINT16, 
                    PREFERRED_SAMPLE_RATE, 
                    buffer_size, 
                    audio_callback, 
                    (void*)channels, 
                    &stream_options
                );
                break;
            }
        }
    }
    
    for (size_t i = 0; i < MAX_CHANNELS; i++) {
        ChannelData_t* channel = new ChannelData_t{
            0, 
            false,
            i,
            RingBuffer<int16_t>(*(buffer_size) * BUFFER_SIZE_COEFF)
        };
        channels[i] = channel;
    }
    audio.startStream();
 
    dpp::cluster bot("OTgwMDQ2MjQxNjM3NDcwMjg4.G6MTfy.-KcwWS9nXUr1cNGKoUP7tJibCWTpMRitn4N21Q", dpp::i_default_intents | dpp::i_message_content | dpp::i_guild_members);
 
    bot.on_log(dpp::utility::cout_logger());
 
    /* Use the on_message_create event to look for commands */
    bot.on_message_create([&bot, &channels](const dpp::message_create_t & event) {
 
        std::stringstream ss(event.msg.content);
        std::string command;
 
        ss >> command;
 
        /* Tell the bot to record */
        if (command == ".join") {
            dpp::guild * g = dpp::find_guild(event.msg.guild_id);
            if (!g->connect_member_voice(event.msg.author.id)) {
                bot.message_create(dpp::message(
                    event.msg.channel_id, 
                    "You don't seem to be on a voice channel! :("
                ));
            }
        }
 
        /* Tell the bot to stop recording */
        if (command == ".stop") {
            event.from->disconnect_voice(event.msg.guild_id);
            unclaim_channel_all(channels);
        }

        if (command == ".info") {
            std::ostringstream os;
            os << event.msg.author.id;
            bot.message_create(dpp::message(
                event.msg.channel_id,
                os.str()
            ));
        }
    });
 
    bot.on_voice_receive([&bot, &channels, &conversion_buffer](const dpp::voice_receive_t &event) {
        int channel = get_channel_num(channels, event.user_id);
        if (channel < 0) {
            channel = claim_empty_channel(channels, event.user_id);
        }
        if (channel >= 0) {
            int16_t* data = (int16_t*)event.audio_data.data();
            for (size_t i = 0; i < event.audio_data.length() >> 1; i = i + 2) {
                conversion_buffer[i >> 1] = (data[i] + data[i + 1]) >> 1;
            }
            channels[channel]->ring_buffer.write(conversion_buffer, event.audio_data.size() >> 2);
        }        
    });
 
    /* Start bot */
    bot.start(false);
    audio.stopStream();
    audio.closeStream();
    return 0;
}