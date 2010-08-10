/*
 * wm8994.h  --  WM8994 Soc Audio driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _WM8994_H
#define _WM8994_H

#include <sound/soc.h>

extern struct snd_soc_codec_device soc_codec_dev_wm8994;
// We don't use array	- DW Shim.
//extern struct snd_soc_dai wm8994_dai[];

/* Sources for AIF1/2 SYSCLK - use with set_dai_sysclk() */
#define WM8994_SYSCLK_MCLK1 1
#define WM8994_SYSCLK_MCLK2 2
#define WM8994_SYSCLK_FLL1  3
#define WM8994_SYSCLK_FLL2  4

#define WM8994_FLL1 1
#define WM8994_FLL2 2

//-----------------------------------------------------------
// Added belows codes by Samsung Electronics.

#ifdef CONFIG_KEPLER_AUDIO_A1026
#include "wm8994_def_kepler.h"
#else
#include "wm8994_def_behold3.h"
#endif


extern struct snd_soc_dai wm8994_dai;

#define WM8994_SYSCLK_MCLK     1
#define WM8994_SYSCLK_FLL      2

#define AUDIO_COMMON_DEBUG 0
#define SET_AUDIO_LOG
//#define WM8994_REGISTER_DUMP

#define ATTACH_ADDITINAL_PCM_DRIVER	// for VT call.

//------------------------------------------------
// Definitions of enum type
//------------------------------------------------
enum playback_path	{ PLAYBACK_OFF, RCV, SPK, HP, BT, DUAL, RING_SPK, RING_HP, RING_DUAL, EXTRA_DOCK_SPEAKER, TV_OUT};
enum mic_path		{ MAIN, SUB, BT_REC, MIC_OFF};
enum fmradio_path { FMR_OFF, FMR_SPK, FMR_HP, FMR_SPK_MIX, FMR_HP_MIX, FMR_DUAL_MIX};
enum fmradio_mix_path	{ FMR_MIX_OFF, FMR_MIX_HP, FMR_MIX_SPK, FMR_MIX_DUAL};
enum power_state	{ CODEC_OFF, CODEC_ON };
enum recognition	{REC_OFF, REC_ON};
enum state{OFF, ON};

#define DEACTIVE				0x00
#define PLAYBACK_ACTIVE		0x01
#define CAPTURE_ACTIVE		0x02
#define CALL_ACTIVE			0x04
#define FMRADIO_ACTIVE		0x08

#define PCM_STREAM_DEACTIVE		0x00
#define PCM_STREAM_PLAYBACK	0x01
#define PCM_STREAM_CAPTURE		0x02

#define CMD_FMR_INPUT_DEACTIVE		0	// Codec Input PGA off for reducing white noise.
#define CMD_FMR_INPUT_ACTIVE		1	// Codec Input PGA on
#define CMD_FMR_FLAG_CLEAR		2	// Radio flag clear for shutdown - to reduce pop up noise.
#define CMD_FMR_END			3	// Codec off in FM radio mode - to reduce pop up noise.
#define CMD_RECOGNITION_DEACTIVE	4	// Distingush recognition gain. To use default MIC gain.
#define CMD_RECOGNITION_ACTIVE		5	// Distingush recognition gain. To use MIC gain for recognition.
#define CMD_CALL_FLAG_CLEAR		6	// Call flag clear for shutdown - to reduce pop up noise.
#define CMD_CALL_END			7	// Codec off in call mode - to reduce pop up noise.

typedef void (*select_route)(struct snd_soc_codec *);
typedef void (*select_mic_route)(struct snd_soc_codec *);

struct wm8994_setup_data {
	int i2c_bus;
	unsigned short i2c_address;
};

struct wm8994_priv {
	//u16 reg_cache[WM8994_REGISTER_COUNT];
	struct snd_soc_codec codec;
	int master;
	int sysclk_source;
	unsigned int mclk_rate;
	unsigned int sysclk_rate;
	unsigned int fs;
	unsigned int bclk;
	unsigned int hw_version;		// For wolfson H/W version. 1 = Rev B, 3 = Rev D
	unsigned int codec_state;
	unsigned int  stream_state;
	enum playback_path cur_path;
	enum mic_path rec_path;
	enum fmradio_path fmradio_path;
	enum fmradio_mix_path fmr_mix_path;
	enum power_state power_state;
	enum state recognition_active;		// for control gain to voice recognition.
	enum state ringtone_active;
	select_route *universal_playback_path;
	select_route *universal_voicecall_path;
	select_mic_route *universal_mic_path;
	int testmode_config_flag;	// for testmode.
};

#if defined SET_AUDIO_LOG
extern unsigned int gbAudioLogEnable;		// This variable must be defined in cpufreq.c(drivers/cpufreq.c).
#endif

#if AUDIO_COMMON_DEBUG
#define DEBUG_LOG(format,...)\
	printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);
#define DEBUG_LOG_INFO(format,...)\
	printk (KERN_DEBUG "[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);
#else
#if defined SET_AUDIO_LOG
#define DEBUG_LOG(format,...)\
	gbAudioLogEnable ? printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__) : 0;
#define DEBUG_LOG_INFO(format,...)\
	gbAudioLogEnable ? printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__) : \
		printk (KERN_DEBUG "[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);
#else
#define DEBUG_LOG(format,...)
#define DEBUG_LOG_INFO(format,...)\
	printk (KERN_DEBUG "[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);
#endif
#endif

#define DEBUG_LOG_ERR(format,...)\
	printk (KERN_ERR "[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);

// Definitions of function prototype.
inline unsigned int wm8994_read(struct snd_soc_codec *codec,unsigned int reg);
int wm8994_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int value);
void wm8994_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *codec_dai);
int audio_init(void);
int audio_power(int en);
void audio_ctrl_mic_bias_gpio(int enable);
void wm8994_set_off(struct snd_soc_codec *codec);
void wm8994_disable_playback_path(struct snd_soc_codec *codec, enum playback_path path);
void wm8994_disable_fmradio_path(struct snd_soc_codec *codec, enum fmradio_path path);
void wm8994_disable_rec_path(struct snd_soc_codec *codec,enum mic_path rec_path);
void  wm8994_record_main_mic( struct snd_soc_codec *codec);
void wm8994_record_headset_mic( struct snd_soc_codec *codec);
void wm8994_record_bluetooth(struct snd_soc_codec *codec);
void wm8994_set_playback_receiver(struct snd_soc_codec *codec);
void wm8994_set_playback_headset(struct snd_soc_codec *codec);
void wm8994_set_playback_speaker(struct snd_soc_codec *codec);
void wm8994_set_playback_speaker_headset(struct snd_soc_codec *codec);
void wm8994_set_playback_bluetooth(struct snd_soc_codec *codec);
void wm8994_set_playback_extra_dock_speaker(struct snd_soc_codec *codec);
void wm8994_set_voicecall_common_setting(struct snd_soc_codec *codec);
void wm8994_set_voicecall_receiver(struct snd_soc_codec *codec);
void wm8994_set_voicecall_headset(struct snd_soc_codec *codec);

#ifdef CONFIG_KEPLER_AUDIO_A1026
void wm8994_set_voicecall_tty(struct snd_soc_codec *codec);
void wm8994_set_voicecall_receiver_audience(struct snd_soc_codec *codec); //hdlnc_ldj_0417_A1026
void wm8994_set_voicecall_factory_subMIC(struct snd_soc_codec *codec); //hdlnc_ldj_0417_A1026
#else
void wm8994_set_voicecall_hac(struct snd_soc_codec *codec);
#endif

void wm8994_set_voicecall_speaker(struct snd_soc_codec *codec);
void wm8994_set_voicecall_bluetooth(struct snd_soc_codec *codec);
void wm8994_set_voicecall_headphone(struct snd_soc_codec *codec);
void wm8994_set_fmradio_common(struct snd_soc_codec *codec, int onoff);
void wm8994_set_fmradio_headset(struct snd_soc_codec *codec);
void wm8994_set_fmradio_speaker(struct snd_soc_codec *codec);
void wm8994_set_fmradio_headset_mix(struct snd_soc_codec *codec);
void wm8994_set_fmradio_speaker_mix(struct snd_soc_codec *codec);
void wm8994_set_fmradio_speaker_headset_mix(struct snd_soc_codec *codec);
#if defined WM8994_REGISTER_DUMP
void wm8994_register_dump(struct snd_soc_codec *codec);
#endif

extern int call_state(void); // hdlnc_bp_ysyim

#endif
