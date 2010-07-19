
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/gpio.h> 
#include <plat/gpio-cfg.h> 
#include <plat/map-base.h>
#include <plat/regs-clock.h>
#include <mach/gpio-jupiter.h>
#include "wm8994.h"

//------------------------------------------------
//		Debug Feature
//------------------------------------------------
#define AUDIO_SPECIFIC_DEBUG 0
#define SUBJECT "wm8994_universal.c"

#if AUDIO_SPECIFIC_DEBUG
#define DEBUG_LOG(format,...)\
	printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);
#else
#define DEBUG_LOG(format,...)
#endif

#define DEBUG_LOG_ERR(format,...)\
	printk ("["SUBJECT "(%d)] " format "\n", __LINE__, ## __VA_ARGS__);


//------------------------------------------------
// Definition of tunning volumes for wm8994
//------------------------------------------------
// DAC
#define TUNING_DAC1L_VOL		0xC0		// 610h
#define TUNING_DAC1R_VOL		0xC0		// 611h

// Speaker
#define TUNING_SPKMIXL_ATTEN	0x0		// 22h
#define TUNING_SPKMIXR_ATTEN	0x0		// 23h
#define TUNING_SPKL_VOL			0x3E		// 26h
#define TUNING_SPKR_VOL			0x3E		// 27h
#define TUNING_CLASSD_VOL		0x6		// 25h

// Headset
#define TUNING_EAR_OUTMIX5_VOL	0x0		// 31h
#define TUNING_EAR_OUTMIX6_VOL	0x0		// 32h
#define TUNING_OUTPUTL_VOL		0x34		// 1Ch
#define TUNING_OUTPUTR_VOL		0x34		// 1Dh
#define TUNING_HPOUTMIX_VOL	0x0

// Receiver
#define TUNING_RCV_OUTMIX5_VOL	0x0		// 31h
#define TUNING_RCV_OUTMIX6_VOL 0x0		// 32h
#define TUNING_OPGAL_VOL		0x3E		// 20h
#define TUNING_OPGAR_VOL		0x3E		// 21h
#define TUNING_HPOUT2_VOL		0x0		// 1Fh


extern int hw_version_check();

int audio_init(void)
{

	//CODEC LDO SETTING
	if (gpio_is_valid(GPIO_CODEC_LDO_EN))
	{
		if (gpio_request(GPIO_CODEC_LDO_EN, "CODEC_LDO_EN"))
			DEBUG_LOG_ERR("Failed to request CODEC_LDO_EN! \n");
		gpio_direction_output(GPIO_CODEC_LDO_EN, 0);
	}

	s3c_gpio_setpull(GPIO_CODEC_LDO_EN, S3C_GPIO_PULL_NONE);

	//CODEC XTAL CLK SETTING
	//b4 : AP Gpio emul, B5 : CODEC_XTAL_EN 
#if (defined CONFIG_JUPITER_VER_B5) || (defined CONFIG_ARIES_VER_B0)
	if (gpio_is_valid(GPIO_CODEC_XTAL_EN)) {
		if (gpio_request(GPIO_CODEC_XTAL_EN, "GPIO_CODEC_XTAL_EN")) 
			DEBUG_LOG_ERR("Failed to request GPIO_CODEC_XTAL_EN! \n");
		
		gpio_direction_output(GPIO_CODEC_XTAL_EN, 0);
	}
	s3c_gpio_setpull(GPIO_CODEC_XTAL_EN, S3C_GPIO_PULL_NONE);
#endif

	// MICBIAS_EN
	if (gpio_is_valid(GPIO_MICBIAS_EN)) {
		if (gpio_request(GPIO_MICBIAS_EN, "GPJ4")) 
			DEBUG_LOG_ERR("Failed to request GPIO_MICBIAS_EN! \n");
		gpio_direction_output(GPIO_MICBIAS_EN, 0);
	}
	s3c_gpio_setpull(GPIO_MICBIAS_EN, S3C_GPIO_PULL_NONE);

	s3c_gpio_slp_setpull_updown(GPIO_CODEC_LDO_EN, S3C_GPIO_PULL_NONE);
	s3c_gpio_slp_setpull_updown(GPIO_MICBIAS_EN, S3C_GPIO_PULL_NONE);

#if  (defined CONFIG_JUPITER_VER_B5) || (defined CONFIG_ARIES_VER_B0) || (defined CONFIG_ARIES_VER_B1)
	gpio_direction_output(GPIO_PCM_SEL, 0);
	s3c_gpio_setpull(GPIO_PCM_SEL, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_PCM_SEL, 0);
#endif

#if (defined CONFIG_JUPITER_VER_B5)  || (defined CONFIG_ARIES_VER_B0)
	s3c_gpio_slp_setpull_updown(GPIO_CODEC_XTAL_EN, S3C_GPIO_PULL_NONE);
#endif
	
	audio_ctrl_sleep_gpio(0);	// It's possible to set gpio to low.
	
	return 0;

}

int audio_power(int en)
{
#if (defined CONFIG_JUPITER_VER_B4)
	u32 val;

	DEBUG_LOG("LDO Enable = %d, XTAL CLK From AP EMUL", en);
	val = __raw_readl(S5P_CLK_OUT);
	val &= 0xFFFE0FFF; //clear bits 12~16
#endif

	if(en)
	{
		gpio_set_value(GPIO_CODEC_LDO_EN, 1);
#if (defined CONFIG_JUPITER_VER_B5)  || (defined CONFIG_ARIES_VER_B0)
		DEBUG_LOG("LDO Enable = %d, XTAL CLK From OSI", en);
		gpio_set_value(GPIO_CODEC_XTAL_EN, 1);
#elif (defined CONFIG_ARIES_VER_B1) || (defined CONFIG_ARIES_VER_B2) || (defined CONFIG_ARIES_VER_B3)
#if (defined CONFIG_SND_UNIVERSAL_WM8994_MASTER)
		if(hw_version_check())
			__raw_writel(__raw_readl(S5P_OTHERS) | (3<<8) , S5P_OTHERS); 
#endif
#elif (defined CONFIG_JUPITER_VER_B4)
#ifdef CONFIG_SND_UNIVERSAL_WM8994_MASTER
		val |= (0x11 << 12); //crystall
#else
		val |= (0x02 << 12); // epll
		val |= (0x5 << 20);
#endif	// end of CONFIG_SND_UNIVERSAL_WM8994_MASTER
		 __raw_writel(val, S5P_CLK_OUT);
#endif	//end of CONFIG_JUPITER_VER_05
	}
	else
	{
		gpio_set_value(GPIO_CODEC_LDO_EN, 0);
#if (defined CONFIG_JUPITER_VER_B5) ||(defined CONFIG_ARIES_VER_B0)
		DEBUG_LOG("LDO Disable = %d, XTAL CLK From OSI", en);
		gpio_set_value(GPIO_CODEC_XTAL_EN, 0);
#elif (defined CONFIG_JUPITER_VER_B4)
		DEBUG_LOG("LDO Disable = %d, XTAL CLK From AP EMUL", en);
#ifdef CONFIG_SND_UNIVERSAL_WM8994_MASTER
		val &= ~(0x11 << 12); //crystall
#else
		val &= ~(0x02 << 12); // epll
		val &= ~(0x5 << 20);
#endif	//end of CONFIG_SND_UNIVERSAL_WM8994_MASTER

		__raw_writel(val, S5P_CLK_OUT);	
#endif	//end of CONFIG_JUPITER_VER_05
	}
#if defined(CONFIG_JUPITER_VER_B4)
	DEBUG_LOG("AUDIO POWER COMPLETED : %d, val=0X%x", en, val);
#else
	DEBUG_LOG("AUDIO POWER COMPLETED : %d", en);
#endif

	return 0;
}

void audio_ctrl_mic_bias_gpio(int enable)
{
	DEBUG_LOG("enable = [%d]", enable);
	
	if(enable)
		gpio_set_value(GPIO_MICBIAS_EN, 1);
	else
		gpio_set_value(GPIO_MICBIAS_EN, 0);
}

// If enable is false, set gpio to low on sleep
void audio_ctrl_sleep_gpio(int enable)
{
	int state;

	if(enable)
	{
		DEBUG_LOG("Set gpio to low on sleep.");
		state = S3C_GPIO_SLP_OUT0;
	}
	else
	{	
		DEBUG_LOG("Set gpio to preserve on sleep.");
		state = S3C_GPIO_SLP_PREV;
	}

	// For preserving output of codec related pins.
	s3c_gpio_slp_cfgpin(GPIO_CODEC_LDO_EN, state);
		
	s3c_gpio_slp_cfgpin(GPIO_MICBIAS_EN, state);
	
#if (defined CONFIG_JUPITER_VER_B5)  || (defined CONFIG_ARIES_VER_B0)
	s3c_gpio_slp_cfgpin(GPIO_CODEC_XTAL_EN, state);
#endif
		
}

/*Audio Routing routines for the universal board..wm8994 codec*/
void wm8994_disable_playback_path(struct snd_soc_codec *codec, enum audio_path path)
{
	u16 val;

	DEBUG_LOG("Path = [%d]", path);
	
	val =wm8994_read(codec,WM8994_POWER_MANAGEMENT_1);

	switch(path)
	{
		case RCV:
			//disbale the HPOUT2
			val&= ~(WM8994_HPOUT2_ENA_MASK);
			break;

		case SPK:
			//disbale the SPKOUTL
			val&= ~(WM8994_SPKOUTL_ENA_MASK ); 
			break;

		case HP:
			//disble the HPOUT1
			val = wm8994_read(codec,WM8994_CHARGE_PUMP_1);
			val &= ~WM8994_CP_ENA_MASK ;
			val = WM8994_CP_ENA_DEFAULT ; // this is from wolfson  	
			wm8994_write(codec,WM8994_CHARGE_PUMP_1 ,val);

			val =wm8994_read(codec,WM8994_POWER_MANAGEMENT_1);
			val&=~(WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK );
			break;

		case SPK_HP :
			//disble the HPOUT1
			val = wm8994_read(codec,WM8994_CHARGE_PUMP_1);
			val &= ~WM8994_CP_ENA_MASK ;
			val = WM8994_CP_ENA_DEFAULT ; // this is from wolfson  	
			wm8994_write(codec,WM8994_CHARGE_PUMP_1 ,val);

			val =wm8994_read(codec,WM8994_POWER_MANAGEMENT_1);
			val&=~(WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK |WM8994_SPKOUTL_ENA_MASK);
			break;			

		default:
			DEBUG_LOG_ERR("Path[%d] is not invaild!\n", path);
			return;
			break;
	}
	
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_1 ,val);
}

void wm8994_disable_rec_path(struct snd_soc_codec *codec,enum mic_path prev_rec_path) 
{
	u16 val;

	DEBUG_LOG("");
	
	audio_ctrl_mic_bias_gpio(0);	// Disable MIC bias
	
        val =wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );

        switch(prev_rec_path)
        {
                case MAIN:
                //DEBUG_LOG("Disabling MAIN Mic Path..\n");
                val&= ~(WM8994_MICB1_ENA_MASK);
                break;

                case SUB:
                //DEBUG_LOG("Disbaling SUB Mic path..\n");
                val&= ~(WM8994_MICB1_ENA_MASK);
                break;
	
		default:
                break;
        }

        wm8994_write(codec,WM8994_POWER_MANAGEMENT_1 ,val);
}

void wm8994_record_headset_mic(struct snd_soc_codec *codec,enum mic_path prev_rec_path) 
{
	struct wm8994_priv *wm8994 = codec->private_data;
	
	u16 val;

	DEBUG_LOG("");
	
	wm8994_disable_rec_path(codec,prev_rec_path);

	audio_ctrl_mic_bias_gpio(1);
	
	DEBUG_LOG("Recording through Headset Mic\n");
	//Enable mic bias, vmid, bias generator
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
	val &= ~(WM8994_BIAS_ENA_MASK | WM8994_VMID_SEL_MASK | WM8994_MICB1_ENA_MASK  );
	val |= (WM8994_BIAS_ENA | WM8994_VMID_SEL_NORMAL |WM8994_MICB1_ENA  );  
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);

	//Enable Right Input Mixer,Enable IN1R PGA
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_2 );
	val &= ~(WM8994_IN1R_ENA_MASK |WM8994_MIXINR_ENA_MASK  );
	val |= (WM8994_MIXINR_ENA | WM8994_IN2R_ENA );
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_2,val);

	if(!wm8994->testmode_config_flag)
	{
		// Enable volume,unmute Right Line	
		val = wm8994_read(codec,WM8994_RIGHT_LINE_INPUT_1_2_VOLUME);	
		val &= ~(WM8994_IN1R_MUTE_MASK  );
		val |= WM8994_IN1R_VOL_25_5dB ;
		wm8994_write(codec,WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,val);
		
		// unmute right pga, set volume 
		val = wm8994_read(codec,WM8994_INPUT_MIXER_4 );
		val &= ~(WM8994_IN1R_TO_MIXINR_MASK |WM8994_IN1R_MIXINR_VOL_MASK ); 
		val |= (WM8994_IN1R_MIXINR_VOL | WM8994_IN1R_TO_MIXINR| 0x5 ); //volume 0db
		wm8994_write(codec,WM8994_INPUT_MIXER_4 ,val);
	}
	//connect in1rn to inr1 and in1rp to inrp
	val = wm8994_read(codec,WM8994_INPUT_MIXER_2);
	val &= ~( WM8994_IN1RN_TO_IN1R_MASK | WM8994_IN1RP_TO_IN1R_MASK);
	val |= (WM8994_IN1RN_TO_IN1R | WM8994_IN1RP_TO_IN1R)  ;	
	wm8994_write(codec,WM8994_INPUT_MIXER_2,val);
	
	//Digital Paths	
	//Enable right ADC and time slot
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_4);
	val &= ~(WM8994_ADCR_ENA_MASK |WM8994_AIF1ADC1R_ENA_MASK );
	val |= (WM8994_AIF1ADC1R_ENA | WM8994_ADCR_ENA  );
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_4 ,val);

	//ADC Right mixer routing
	val = wm8994_read(codec,WM8994_AIF1_ADC1_RIGHT_MIXER_ROUTING);
	val &= ~( WM8994_ADC1R_TO_AIF1ADC1R_MASK);
	val |= WM8994_ADC1R_TO_AIF1ADC1R;
	wm8994_write(codec,WM8994_AIF1_ADC1_RIGHT_MIXER_ROUTING,val);

}


void wm8994_record_main_mic(struct snd_soc_codec *codec,enum mic_path prev_rec_path) 
{
	struct wm8994_priv *wm8994 = codec->private_data;

	u16 val;

	DEBUG_LOG("");
	
	wm8994_disable_rec_path(codec,prev_rec_path);
	DEBUG_LOG("Recording through Main Mic\n");
	audio_ctrl_mic_bias_gpio(1);
	
	//Enable micbias,vmid,mic1
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
	val &= ~(WM8994_BIAS_ENA_MASK | WM8994_VMID_SEL_MASK | WM8994_MICB1_ENA_MASK  );
	val |= (WM8994_BIAS_ENA | WM8994_VMID_SEL_NORMAL |WM8994_MICB1_ENA  );  
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);
	//Enable left input mixer and IN1L PGA	
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_2  );
	val &= ~( WM8994_IN1L_ENA_MASK | WM8994_MIXINL_ENA_MASK );
	val |= (WM8994_MIXINL_ENA |WM8994_IN1L_ENA );
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_2,val);

	if(!wm8994->testmode_config_flag)
	{	
		// Unmute IN1L PGA, update volume
		val = wm8994_read(codec,WM8994_LEFT_LINE_INPUT_1_2_VOLUME );	
		val &= ~(WM8994_IN1L_MUTE_MASK );
		//val |=0x0B; //volume
			 val |=0x1F; //volume
		wm8994_write(codec,WM8994_LEFT_LINE_INPUT_1_2_VOLUME ,val);
	
		//Unmute the PGA
		val = wm8994_read(codec,WM8994_INPUT_MIXER_3 );
		val&= ~(WM8994_IN1L_TO_MIXINL_MASK	 );
		val |= (WM8994_IN1L_TO_MIXINL |WM8994_IN1L_MIXINL_VOL| 0x5);//0db
		wm8994_write(codec,WM8994_INPUT_MIXER_3 ,val); 
	}
	
	//Connect IN1LN ans IN1LP to the inputs
	val = wm8994_read(codec,WM8994_INPUT_MIXER_2 );	
	val &= (WM8994_IN1LN_TO_IN1L_MASK | WM8994_IN1LP_TO_IN1L_MASK );
	val |= ( WM8994_IN1LP_TO_IN1L | WM8994_IN1LN_TO_IN1L );
	wm8994_write(codec,WM8994_INPUT_MIXER_2 ,val);

	//Digital Paths
	
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_4 );
	val &= ~(WM8994_ADCL_ENA_MASK |WM8994_AIF1ADC1L_ENA_MASK  );
	val |= ( WM8994_AIF1ADC1L_ENA | WM8994_ADCL_ENA );
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_4 ,val);
	//Enable timeslots
	val = wm8994_read(codec,WM8994_AIF1_ADC1_LEFT_MIXER_ROUTING );
	val |=WM8994_ADC1L_TO_AIF1ADC1L ;  
	wm8994_write(codec,WM8994_AIF1_ADC1_LEFT_MIXER_ROUTING ,val);

	wm8994_write( codec, WM8994_GPIO_1, 0xA101 );   // GPIO1 is Input Enable
}

void wm8994_set_playback_receiver(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	u16 val;

	DEBUG_LOG("");
	
	if(!wm8994->testmode_config_flag)
	{
	        val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5);
		val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
	        val |= TUNING_RCV_OUTMIX5_VOL << WM8994_DACL_MIXOUTL_VOL_SHIFT;
	        wm8994_write(codec,WM8994_OUTPUT_MIXER_5, val );
	
		val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6);
		val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
		val |= TUNING_RCV_OUTMIX6_VOL << WM8994_DACR_MIXOUTR_VOL_SHIFT;
		wm8994_write(codec,WM8994_OUTPUT_MIXER_6, val );

		val = wm8994_read(codec, WM8994_LEFT_OPGA_VOLUME);
		val &= ~(WM8994_MIXOUTL_MUTE_N_MASK | WM8994_MIXOUTL_VOL_MASK);
		val |= WM8994_MIXOUTL_MUTE_N | TUNING_OPGAL_VOL;
		wm8994_write(codec,WM8994_LEFT_OPGA_VOLUME, val );

		val = wm8994_read(codec, WM8994_RIGHT_OPGA_VOLUME);
		val &= ~(WM8994_MIXOUTR_MUTE_N_MASK | WM8994_MIXOUTR_VOL_MASK);
		val |= WM8994_MIXOUTR_MUTE_N | TUNING_OPGAR_VOL;
		wm8994_write(codec,WM8994_RIGHT_OPGA_VOLUME, val );
	
	        val = wm8994_read(codec, WM8994_HPOUT2_VOLUME);
		val &= ~(WM8994_HPOUT2_MUTE_MASK | WM8994_HPOUT2_VOL_MASK);
	        val |= TUNING_HPOUT2_VOL << WM8994_HPOUT2_VOL_SHIFT;
	        wm8994_write(codec,WM8994_HPOUT2_VOLUME, val );

		//Unmute DAC1 left
		val = wm8994_read(codec,WM8994_DAC1_LEFT_VOLUME );
		val &= ~(WM8994_DAC1L_MUTE_MASK | WM8994_DAC1L_VOL_MASK);
		val |= TUNING_DAC1L_VOL; 
		wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME ,val);
	
		//Unmute and volume ctrl RightDAC
		val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
		val &= ~(WM8994_DAC1R_MUTE_MASK | WM8994_DAC1R_VOL_MASK);
		val |= TUNING_DAC1R_VOL;
		wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
	}

	val = wm8994_read(codec,WM8994_OUTPUT_MIXER_1);
        val &= ~(WM8994_DAC1L_TO_MIXOUTL_MASK);
        val |= (WM8994_DAC1L_TO_MIXOUTL);
        wm8994_write(codec,WM8994_OUTPUT_MIXER_1,val);

        val = wm8994_read(codec,WM8994_OUTPUT_MIXER_2);
        val &= ~(WM8994_DAC1R_TO_MIXOUTR_MASK);
        val |= (WM8994_DAC1R_TO_MIXOUTR);
        wm8994_write(codec,WM8994_OUTPUT_MIXER_2,val);

	val = wm8994_read(codec,WM8994_HPOUT2_MIXER);
        val &= ~(WM8994_MIXOUTLVOL_TO_HPOUT2_MASK | WM8994_MIXOUTRVOL_TO_HPOUT2_MASK);
        val |= (WM8994_MIXOUTRVOL_TO_HPOUT2 | WM8994_MIXOUTLVOL_TO_HPOUT2);
        wm8994_write(codec,WM8994_HPOUT2_MIXER,val);

        val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_5);
        val &= ~(WM8994_DAC1R_ENA_MASK | WM8994_DAC1L_ENA_MASK | WM8994_AIF1DAC1R_ENA_MASK | WM8994_AIF1DAC1L_ENA_MASK);
        val |= (WM8994_AIF1DAC1L_ENA | WM8994_AIF1DAC1R_ENA | WM8994_DAC1L_ENA | WM8994_DAC1R_ENA);
        wm8994_write(codec,WM8994_POWER_MANAGEMENT_5,val);

        val = wm8994_read(codec,WM8994_AIF1_DAC1_FILTERS_1);
        val = 0;
        wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1,val);

	val = wm8994_read(codec,WM8994_DAC1_LEFT_MIXER_ROUTING);
        val &= ~(WM8994_AIF1DAC1L_TO_DAC1L_MASK);
        val |= (WM8994_AIF1DAC1L_TO_DAC1L);
        wm8994_write(codec,WM8994_DAC1_LEFT_MIXER_ROUTING,val);

        val = wm8994_read(codec,WM8994_DAC1_RIGHT_MIXER_ROUTING);
        val &= ~(WM8994_AIF1DAC1R_TO_DAC1R_MASK);
        val |= (WM8994_AIF1DAC1R_TO_DAC1R);
        wm8994_write(codec,WM8994_DAC1_RIGHT_MIXER_ROUTING,val);

	val = wm8994_read(codec,WM8994_CLOCKING_1);
        val &= ~(WM8994_DSP_FS1CLK_ENA_MASK | WM8994_DSP_FSINTCLK_ENA_MASK);
        val |= (WM8994_DSP_FS1CLK_ENA | WM8994_DSP_FSINTCLK_ENA);
        wm8994_write(codec,WM8994_CLOCKING_1,val);

	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_3);
        val &= ~(WM8994_MIXOUTLVOL_ENA_MASK | WM8994_MIXOUTRVOL_ENA_MASK | WM8994_MIXOUTL_ENA_MASK | WM8994_MIXOUTR_ENA_MASK);
        val |= (WM8994_MIXOUTL_ENA | WM8994_MIXOUTR_ENA | WM8994_MIXOUTRVOL_ENA | WM8994_MIXOUTLVOL_ENA);
        wm8994_write(codec,WM8994_POWER_MANAGEMENT_3,val);
		
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
	val &= ~(WM8994_BIAS_ENA_MASK | WM8994_VMID_SEL_MASK | WM8994_HPOUT2_ENA_MASK );
	val |= (WM8994_BIAS_ENA | WM8994_VMID_SEL_NORMAL | WM8994_HPOUT2_ENA );
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);
}


void wm8994_set_playback_headset(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	u16 val;
	
	DEBUG_LOG("");
			
	// Enbale DAC1L to HPOUT1L path
	val = wm8994_read(codec, WM8994_OUTPUT_MIXER_1);
	val &=  ~WM8994_DAC1L_TO_HPOUT1L_MASK;
	val |= WM8994_DAC1L_TO_HPOUT1L ;  	
	wm8994_write(codec,WM8994_OUTPUT_MIXER_1, val);

	// Enbale DAC1R to HPOUT1R path
	val = wm8994_read(codec, WM8994_OUTPUT_MIXER_2);
	val &= ~WM8994_DAC1R_TO_HPOUT1R_MASK;
	val |= WM8994_DAC1R_TO_HPOUT1R;
	wm8994_write(codec, WM8994_OUTPUT_MIXER_2, val);

	//Enable Charge Pump	
	val = wm8994_read(codec, WM8994_CHARGE_PUMP_1);
	val &= ~WM8994_CP_ENA_MASK ;
	val = WM8994_CP_ENA | WM8994_CP_ENA_DEFAULT ; // this is from wolfson  	
	wm8994_write(codec, WM8994_CHARGE_PUMP_1, val);

	// Intermediate HP settings
	val = wm8994_read(codec, WM8994_ANALOGUE_HP_1); 	
	val &= ~(WM8994_HPOUT1R_DLY_MASK |WM8994_HPOUT1R_OUTP_MASK |WM8994_HPOUT1R_RMV_SHORT_MASK |
		WM8994_HPOUT1L_DLY_MASK |WM8994_HPOUT1L_OUTP_MASK | WM8994_HPOUT1L_RMV_SHORT_MASK);
	val |= (WM8994_HPOUT1L_RMV_SHORT | WM8994_HPOUT1L_OUTP|WM8994_HPOUT1L_DLY |WM8994_HPOUT1R_RMV_SHORT | 
		WM8994_HPOUT1R_OUTP | WM8994_HPOUT1R_DLY);
	wm8994_write(codec, WM8994_ANALOGUE_HP_1, val);

	// Headset Control
	if(!wm8994->testmode_config_flag)
	{
	        val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5);
		val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
	        val |= TUNING_EAR_OUTMIX5_VOL << WM8994_DACL_MIXOUTL_VOL_SHIFT;
	        wm8994_write(codec,WM8994_OUTPUT_MIXER_5, val );
	
		val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6);
		val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
		val |= TUNING_EAR_OUTMIX6_VOL << WM8994_DACR_MIXOUTR_VOL_SHIFT;
		wm8994_write(codec,WM8994_OUTPUT_MIXER_6, val );

		val = wm8994_read(codec,WM8994_LEFT_OUTPUT_VOLUME);
		val &= ~(WM8994_HPOUT1L_MUTE_N_MASK | WM8994_HPOUT1L_VOL_MASK);
		val |= (WM8994_HPOUT1L_MUTE_N | TUNING_OUTPUTL_VOL);
		wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME, val);

		val = wm8994_read(codec,WM8994_RIGHT_OUTPUT_VOLUME);
		val &= ~(WM8994_HPOUT1R_MUTE_N_MASK | WM8994_HPOUT1R_VOL_MASK);
		val |= (WM8994_HPOUT1R_MUTE_N | TUNING_OUTPUTR_VOL);
		wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME, val);
	
		//Unmute DAC1 left
		val = wm8994_read(codec,WM8994_DAC1_LEFT_VOLUME );
		val &= ~(WM8994_DAC1L_MUTE_MASK | WM8994_DAC1L_VOL_MASK);
		val |= TUNING_DAC1L_VOL; 
		wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME ,val);
	
		//Unmute and volume ctrl RightDAC
		val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
		val &= ~(WM8994_DAC1R_MUTE_MASK | WM8994_DAC1R_VOL_MASK);
		val |= TUNING_DAC1R_VOL; //0 db volume	
		wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
	}

	//Configuring the Digital Paths
	//Enable Dac1 and DAC2 and the Timeslot0 for AIF1	
	val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_5 ); 	
	val &= ~(WM8994_DAC1R_ENA_MASK | WM8994_DAC1L_ENA_MASK|WM8994_AIF1DAC1R_ENA_MASK |  WM8994_AIF1DAC1L_ENA_MASK );
	val |= (WM8994_AIF1DAC1L_ENA | WM8994_AIF1DAC1R_ENA  | WM8994_DAC1L_ENA |WM8994_DAC1R_ENA );
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_5 ,val);

	// Unmute the AF1DAC1	
	val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1 ); 	
	val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
	wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1 ,val);
	
	// Enable the Timeslot0 to DAC1L
	val = wm8994_read(codec, WM8994_DAC1_LEFT_MIXER_ROUTING  ); 	
	val &= ~(WM8994_AIF1DAC1L_TO_DAC1L_MASK);
	val |= WM8994_AIF1DAC1L_TO_DAC1L;
	wm8994_write(codec,WM8994_DAC1_LEFT_MIXER_ROUTING ,val);

	//Enable the Timeslot0 to DAC1R
	val = wm8994_read(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING  ); 	
	val &= ~(WM8994_AIF1DAC1R_TO_DAC1R_MASK);
	val |= WM8994_AIF1DAC1R_TO_DAC1R;
	wm8994_write(codec,WM8994_DAC1_RIGHT_MIXER_ROUTING ,val);
		
	//Enable vmid,bias, hp left and right
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
	val &= ~(WM8994_BIAS_ENA_MASK | WM8994_VMID_SEL_MASK |WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK);
	val |= (WM8994_BIAS_ENA | WM8994_VMID_SEL_NORMAL |WM8994_HPOUT1R_ENA	 |WM8994_HPOUT1L_ENA);  
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);
}

void wm8994_set_playback_speaker(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	u16 val;
	int ret;

	DEBUG_LOG("testmode_config_flag = [%d]",wm8994->testmode_config_flag);

	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_3 );
	val &= ~( WM8994_SPKLVOL_ENA_MASK   );
	val |= WM8994_SPKLVOL_ENA;
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_3 ,val);

	// Speaker Volume Control
	if(!wm8994->testmode_config_flag)
	{
		// Unmute the SPKMIXVOLUME
		val = wm8994_read(codec, WM8994_SPKMIXL_ATTENUATION);
		val &= ~(WM8994_SPKMIXL_VOL_MASK);
		val |= TUNING_SPKMIXL_ATTEN;	
		wm8994_write(codec, WM8994_SPKMIXL_ATTENUATION, val);
		
		val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
		val &= ~(WM8994_SPKMIXR_VOL_MASK);
		val |= TUNING_SPKMIXR_ATTEN;	
		wm8994_write(codec, WM8994_SPKMIXR_ATTENUATION, val);

		val = wm8994_read(codec,WM8994_SPEAKER_VOLUME_LEFT);
		val &= ~(WM8994_SPKOUTL_MUTE_N_MASK | WM8994_SPKOUTL_VOL_MASK);
		val |= (WM8994_SPKOUTL_MUTE_N | TUNING_SPKL_VOL);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);

		val = wm8994_read(codec,WM8994_SPEAKER_VOLUME_RIGHT);
		val &= ~(WM8994_SPKOUTR_MUTE_N_MASK | WM8994_SPKOUTR_VOL_MASK);
		val |= (WM8994_SPKOUTR_MUTE_N | TUNING_SPKR_VOL);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);

		val = wm8994_read(codec,WM8994_CLASSD);
		val &= ~(WM8994_SPKOUTL_BOOST_MASK);
		val |= TUNING_CLASSD_VOL << WM8994_SPKOUTL_BOOST_SHIFT;
		wm8994_write(codec, WM8994_CLASSD, val);
	
		//Unmute DAC1 left
		val = wm8994_read(codec,WM8994_DAC1_LEFT_VOLUME );
		val &= ~(WM8994_DAC1L_MUTE_MASK | WM8994_DAC1L_VOL_MASK);
		val |= TUNING_DAC1L_VOL; 
		wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME ,val);
	
		//Unmute and volume ctrl RightDAC
		val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
		val &= ~(WM8994_DAC1R_MUTE_MASK | WM8994_DAC1R_VOL_MASK);
		val |= TUNING_DAC1R_VOL; //0 db volume	
		wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
	}

	val = wm8994_read(codec,WM8994_SPKOUT_MIXERS );
	val |= WM8994_SPKMIXL_TO_SPKOUTL | WM8994_SPKMIXR_TO_SPKOUTL ;
	wm8994_write(codec,WM8994_SPKOUT_MIXERS,val );

	//Unmute the DAC path
	val = wm8994_read(codec,WM8994_SPEAKER_MIXER );
	val &= ~(WM8994_DAC1L_TO_SPKMIXL_MASK );
	val |= WM8994_DAC1L_TO_SPKMIXL ;
	wm8994_write(codec,WM8994_SPEAKER_MIXER ,val);

	// Eable DAC1 Left and timeslot left
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_5 );	
	val &= ~( WM8994_DAC1L_ENA_MASK | WM8994_AIF1DAC1L_ENA_MASK  );
	val |= (WM8994_AIF1DAC1L_ENA | WM8994_DAC1L_ENA );
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_5,val);   

	//Unmute
	val = wm8994_read(codec,WM8994_AIF1_DAC1_FILTERS_1 );
	val &= ~(WM8994_AIF1DAC1_MUTE_MASK );
	val |= WM8994_AIF1DAC1_UNMUTE ;
	wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1,val );

	//enable timeslot0 to left dac
	val = wm8994_read(codec,WM8994_DAC1_LEFT_MIXER_ROUTING );
	val &= ~(WM8994_AIF1DAC1L_TO_DAC1L_MASK );
	val |= WM8994_AIF1DAC1L_TO_DAC1L;
	wm8994_write(codec,WM8994_DAC1_LEFT_MIXER_ROUTING ,val);
	
	//Enbale bias,vmid and Left speaker
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
	val &= ~(WM8994_BIAS_ENA_MASK | WM8994_VMID_SEL_MASK |WM8994_SPKOUTL_ENA_MASK  );
	val |= (WM8994_BIAS_ENA | WM8994_VMID_SEL_NORMAL |  WM8994_SPKOUTL_ENA  );  
	ret = wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);
}

void wm8994_set_playback_speaker_headset(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	u16 val;
	int ret;

	DEBUG_LOG("testmode_config_flag = [%d]",wm8994->testmode_config_flag);

	//------------------  Ear Path Settings ------------------

	// Headset Control
	if(!wm8994->testmode_config_flag)
	{
	        val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5);
		val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
	        val |= TUNING_EAR_OUTMIX5_VOL << WM8994_DACL_MIXOUTL_VOL_SHIFT;
	        wm8994_write(codec,WM8994_OUTPUT_MIXER_5, val );
	
		val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6);
		val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
		val |= TUNING_EAR_OUTMIX6_VOL << WM8994_DACR_MIXOUTR_VOL_SHIFT;
		wm8994_write(codec,WM8994_OUTPUT_MIXER_6, val );

		val = wm8994_read(codec,WM8994_LEFT_OUTPUT_VOLUME);
		val &= ~(WM8994_HPOUT1L_MUTE_N_MASK | WM8994_HPOUT1L_VOL_MASK);
		val |= (WM8994_HPOUT1L_MUTE_N | TUNING_OUTPUTL_VOL);
		wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME, val);

		val = wm8994_read(codec,WM8994_RIGHT_OUTPUT_VOLUME);
		val &= ~(WM8994_HPOUT1R_MUTE_N_MASK | WM8994_HPOUT1R_VOL_MASK);
		val |= (WM8994_HPOUT1R_MUTE_N | TUNING_OUTPUTR_VOL);
		wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME, val);
	}

	// Enbale DAC1L to HPOUT1L path
	val = wm8994_read(codec, WM8994_OUTPUT_MIXER_1);
	val &=  ~WM8994_DAC1L_TO_HPOUT1L_MASK;
	val |= WM8994_DAC1L_TO_HPOUT1L ;  	
	wm8994_write(codec,WM8994_OUTPUT_MIXER_1, val);

	// Enbale DAC1R to HPOUT1R path
	val = wm8994_read(codec, WM8994_OUTPUT_MIXER_2);
	val &= ~WM8994_DAC1R_TO_HPOUT1R_MASK;
	val |= WM8994_DAC1R_TO_HPOUT1R;
	wm8994_write(codec, WM8994_OUTPUT_MIXER_2, val);

	//Enable Charge Pump	
	val = wm8994_read(codec, WM8994_CHARGE_PUMP_1);
	val &= ~WM8994_CP_ENA_MASK ;
	val = WM8994_CP_ENA | WM8994_CP_ENA_DEFAULT ; // this is from wolfson  	
	wm8994_write(codec, WM8994_CHARGE_PUMP_1, val);

	// Intermediate HP settings
	val = wm8994_read(codec, WM8994_ANALOGUE_HP_1); 	
	val &= ~(WM8994_HPOUT1R_DLY_MASK |WM8994_HPOUT1R_OUTP_MASK |WM8994_HPOUT1R_RMV_SHORT_MASK |
		WM8994_HPOUT1L_DLY_MASK |WM8994_HPOUT1L_OUTP_MASK | WM8994_HPOUT1L_RMV_SHORT_MASK);
	val |= (WM8994_HPOUT1L_RMV_SHORT | WM8994_HPOUT1L_OUTP|WM8994_HPOUT1L_DLY |WM8994_HPOUT1R_RMV_SHORT | 
		WM8994_HPOUT1R_OUTP | WM8994_HPOUT1R_DLY);
	wm8994_write(codec, WM8994_ANALOGUE_HP_1, val);
	
	//Configuring the Digital Paths
	
	//Enable the Timeslot0 to DAC1R
	val = wm8994_read(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING  ); 	
	val &= ~(WM8994_AIF1DAC1R_TO_DAC1R_MASK);
	val |= WM8994_AIF1DAC1R_TO_DAC1R;
	wm8994_write(codec,WM8994_DAC1_RIGHT_MIXER_ROUTING ,val);

	//------------------  Speaker Path Settings ------------------

	// Speaker Volume Control
	if(!wm8994->testmode_config_flag)
	{
		// Unmute the SPKMIXVOLUME
		val = wm8994_read(codec, WM8994_SPKMIXL_ATTENUATION);
		val &= ~(WM8994_SPKMIXL_VOL_MASK);
		val |= TUNING_SPKMIXL_ATTEN;	
		wm8994_write(codec, WM8994_SPKMIXL_ATTENUATION, val);
		
		val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
		val &= ~(WM8994_SPKMIXR_VOL_MASK);
		val |= TUNING_SPKMIXR_ATTEN;	
		wm8994_write(codec, WM8994_SPKMIXR_ATTENUATION, val);

		val = wm8994_read(codec,WM8994_SPEAKER_VOLUME_LEFT);
		val &= ~(WM8994_SPKOUTL_MUTE_N_MASK | WM8994_SPKOUTL_VOL_MASK);
		val |= (WM8994_SPKOUTL_MUTE_N | TUNING_SPKL_VOL);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);

		val = wm8994_read(codec,WM8994_SPEAKER_VOLUME_RIGHT);
		val &= ~(WM8994_SPKOUTR_MUTE_N_MASK | WM8994_SPKOUTR_VOL_MASK);
		val |= (WM8994_SPKOUTR_MUTE_N | TUNING_SPKR_VOL);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);

		val = wm8994_read(codec,WM8994_CLASSD);
		val &= ~(WM8994_SPKOUTL_BOOST_MASK);
		val |= TUNING_CLASSD_VOL << WM8994_SPKOUTL_BOOST_SHIFT;
		wm8994_write(codec, WM8994_CLASSD, val);
	
		//Unmute DAC1 left
		val = wm8994_read(codec,WM8994_DAC1_LEFT_VOLUME );
		val &= ~(WM8994_DAC1L_MUTE_MASK | WM8994_DAC1L_VOL_MASK);
		val |= TUNING_DAC1L_VOL; 
		wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME ,val);
	
		//Unmute and volume ctrl RightDAC
		val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
		val &= ~(WM8994_DAC1R_MUTE_MASK | WM8994_DAC1R_VOL_MASK);
		val |= TUNING_DAC1R_VOL; //0 db volume	
		wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
	}

	val = wm8994_read(codec,WM8994_SPKOUT_MIXERS );
	val |= WM8994_SPKMIXL_TO_SPKOUTL | WM8994_SPKMIXR_TO_SPKOUTL ;
	wm8994_write(codec,WM8994_SPKOUT_MIXERS,val );

	//Unmute the DAC path
	val = wm8994_read(codec,WM8994_SPEAKER_MIXER );
	val &= ~(WM8994_DAC1L_TO_SPKMIXL_MASK );
	val |= WM8994_DAC1L_TO_SPKMIXL ;
	wm8994_write(codec,WM8994_SPEAKER_MIXER ,val);

	//------------------  Common Settings ------------------

	//Enable DAC1 and DAC2 and the Timeslot0 for AIF1	
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_5 );	
	val &= ~(WM8994_DAC1R_ENA_MASK | WM8994_DAC1L_ENA_MASK|WM8994_AIF1DAC1R_ENA_MASK |  WM8994_AIF1DAC1L_ENA_MASK );
	val |= (WM8994_AIF1DAC1L_ENA | WM8994_AIF1DAC1R_ENA  | WM8994_DAC1L_ENA |WM8994_DAC1R_ENA );
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_5,val);   

	// Unmute the AIF1DAC1	
	val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1 ); 	
	val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
	val |= WM8994_AIF1DAC1_UNMUTE ;
	wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1 ,val);

	// Enable the Timeslot0 to DAC1L
	val = wm8994_read(codec, WM8994_DAC1_LEFT_MIXER_ROUTING  ); 	
	val &= ~(WM8994_AIF1DAC1L_TO_DAC1L_MASK);
	val |= WM8994_AIF1DAC1L_TO_DAC1L;
	wm8994_write(codec,WM8994_DAC1_LEFT_MIXER_ROUTING ,val);
	
	//Enbale bias,vmid, hp left and right and Left speaker
	val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_3);
	val &= ~(WM8994_SPKLVOL_ENA_MASK);
	val |= WM8994_SPKLVOL_ENA;
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_3 ,val);
	
	val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_1);
	val &= ~(WM8994_BIAS_ENA_MASK | WM8994_VMID_SEL_MASK |
		WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK|WM8994_SPKOUTL_ENA_MASK);
	val |= (WM8994_BIAS_ENA | WM8994_VMID_SEL_NORMAL |
		WM8994_HPOUT1R_ENA	 |WM8994_HPOUT1L_ENA |WM8994_SPKOUTL_ENA);
	ret = wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);
}                                                      

void wm8994_set_voicecall_receiver(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	int val;
	
	DEBUG_LOG("");

	wm8994_write(codec, WM8994_CHARGE_PUMP_1, WM8994_CP_ENA_DEFAULT);	// Turn off charge pump.
	
	// Analogue Input Configuration -Main MIC
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_2, 
		WM8994_TSHUT_ENA | WM8994_TSHUT_OPDIS | WM8994_MIXINL_ENA | WM8994_IN1L_ENA);
	
	wm8994_write(codec, WM8994_INPUT_MIXER_2, WM8994_IN1LP_TO_IN1L | WM8994_IN1LN_TO_IN1L); 	// differential(3) or single ended(1)
		
	/* Digital Path Enables and Unmutes*/	
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_4, WM8994_AIF2ADCL_ENA | WM8994_ADCL_ENA);	
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_5 , 
		WM8994_AIF2DACL_ENA | WM8994_AIF2DACR_ENA | WM8994_AIF1DAC1L_ENA | WM8994_AIF1DAC1R_ENA | 
		WM8994_DAC1L_ENA | WM8994_DAC1R_ENA);
	
	wm8994_write(codec, WM8994_AIF2_DAC_FILTERS_1, 0x0000 );	//AIF2DAC Unmute, Mono Mix diable, Fast Ramp

	// AIF1 & AIF2 Output is connected to DAC1	
	wm8994_write(codec, WM8994_DAC1_LEFT_MIXER_ROUTING, WM8994_AIF2DACL_TO_DAC1L | WM8994_AIF1DAC1L_TO_DAC1L);	
	wm8994_write(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING, WM8994_AIF2DACR_TO_DAC1R | WM8994_AIF1DAC1R_TO_DAC1R);	

	// Tx -> AIF2 Path
	wm8994_write(codec, WM8994_DAC2_LEFT_MIXER_ROUTING, WM8994_ADC1_TO_DAC2L);	
	wm8994_write(codec, WM8994_DAC2_RIGHT_MIXER_ROUTING, WM8994_ADC1_TO_DAC2R);

	if(!wm8994->testmode_config_flag)
	{	
		// Volume Control - Input
		wm8994_write(codec, WM8994_INPUT_MIXER_3, WM8994_IN1L_TO_MIXINL | WM8994_IN1L_MIXINL_VOL ); 	// IN1L -> MIXINL, 30dB
	
		val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME );	
		val &= ~WM8994_IN1L_MUTE_MASK;	// Unmute IN1L
        	val |= WM8994_IN1L_VOL_30dB;
		wm8994_write(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME , val);

		// Volume Control - Output
		val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5);
		val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
		val |= TUNING_RCV_OUTMIX5_VOL << WM8994_DACL_MIXOUTL_VOL_SHIFT;
		wm8994_write(codec,WM8994_OUTPUT_MIXER_5, val );
		
		val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6);
		val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
		val |= TUNING_RCV_OUTMIX6_VOL << WM8994_DACR_MIXOUTR_VOL_SHIFT;
		wm8994_write(codec,WM8994_OUTPUT_MIXER_6, val );
	
		val = wm8994_read(codec, WM8994_LEFT_OPGA_VOLUME);
		val &= ~(WM8994_MIXOUTL_MUTE_N_MASK | WM8994_MIXOUTL_VOL_MASK);
		val |= WM8994_MIXOUTL_MUTE_N | TUNING_OPGAL_VOL;
		wm8994_write(codec,WM8994_LEFT_OPGA_VOLUME, val );
	
		val = wm8994_read(codec, WM8994_RIGHT_OPGA_VOLUME);
		val &= ~(WM8994_MIXOUTR_MUTE_N_MASK | WM8994_MIXOUTR_VOL_MASK);
		val |= WM8994_MIXOUTR_MUTE_N | TUNING_OPGAR_VOL;
		wm8994_write(codec,WM8994_RIGHT_OPGA_VOLUME, val );
		
		val = wm8994_read(codec, WM8994_HPOUT2_VOLUME);
		val &= ~(WM8994_HPOUT2_MUTE_MASK | WM8994_HPOUT2_VOL_MASK);
		val |= TUNING_HPOUT2_VOL << WM8994_HPOUT2_VOL_SHIFT;
		wm8994_write(codec,WM8994_HPOUT2_VOLUME, val );
	
		//Unmute DAC1 left
		val = wm8994_read(codec,WM8994_DAC1_LEFT_VOLUME );
		val &= ~(WM8994_DAC1L_MUTE_MASK | WM8994_DAC1L_VOL_MASK);
		val |= TUNING_DAC1L_VOL; 
		wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME ,val);
		
		//Unmute and volume ctrl RightDAC
		val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
		val &= ~(WM8994_DAC1R_MUTE_MASK | WM8994_DAC1R_VOL_MASK);
		val |= TUNING_DAC1R_VOL; //0 db volume	
		wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
	}

	// Output Mixing
	wm8994_write(codec, WM8994_OUTPUT_MIXER_1, WM8994_DAC1L_TO_MIXOUTL);
	wm8994_write(codec, WM8994_OUTPUT_MIXER_2, WM8994_DAC1R_TO_MIXOUTR );
	
	/*Clocking*/	
	wm8994_write(codec,WM8994_CLOCKING_1, 
		WM8994_DSP_FS1CLK_ENA | WM8994_DSP_FS2CLK_ENA | WM8994_DSP_FSINTCLK_ENA);	
	wm8994_write(codec,WM8994_CLOCKING_2,
		0x1 << WM8994_TOCLK_DIV_SHIFT | 0x6 << WM8994_DBCLK_DIV_SHIFT |0x3 << WM8994_OPCLK_DIV_SHIFT);
	wm8994_write(codec,WM8994_AIF2_RATE, 0x3 << WM8994_AIF2CLK_RATE_SHIFT);	
	
	// AIF2 Interface
	wm8994_write(codec,WM8994_AIF2_CONTROL_1, 	//Left Justified, BCLK invert, LRCLK Invert
		WM8994_AIF2ADCR_SRC | WM8994_AIF2_BCLK_INV |WM8994_AIF2_LRCLK_INV | 0x1 << WM8994_AIF2_FMT_SHIFT);	
	wm8994_write(codec,WM8994_AIF2_CONTROL_2, 0x0000 );
	wm8994_write(codec,WM8994_AIF2_MASTER_SLAVE, WM8994_AIF2_MSTR);	//Master
	
	/*FLL 1 Setting*/	
	wm8994_write(codec,WM8994_FLL1_CONTROL_1, WM8994_FLL1_FRACN_ENA | WM8994_FLL1_ENA);
	wm8994_write(codec,WM8994_FLL1_CONTROL_2, WM8994_FLL1_OUTDIV_8);
	wm8994_write(codec,WM8994_FLL1_CONTROL_3, 0x86C2 );
	wm8994_write(codec,WM8994_FLL1_CONTROL_4, 0x00E0 );
	wm8994_write(codec,WM8994_FLL1_CONTROL_5, 0x0C88 );
	
	/*FLL2 Setting*/	
	wm8994_write(codec,WM8994_FLL2_CONTROL_1, WM8994_FLL2_FRACN_ENA | WM8994_FLL2_ENA);	
	wm8994_write(codec,WM8994_FLL2_CONTROL_2, 0x2F00 );	
	wm8994_write(codec,WM8994_FLL2_CONTROL_3, 0x3126 );	
	wm8994_write(codec,WM8994_FLL2_CONTROL_5, 0x0C88 );	
	wm8994_write(codec,WM8994_AIF2_CLOCKING_1, 0x0019 );	
	wm8994_write(codec,WM8994_FLL2_CONTROL_4, 0x0100 );
	
	/*GPIO Configuration*/
	wm8994_write(codec,WM8994_GPIO_3, 0x8100 );	
	wm8994_write(codec,WM8994_GPIO_4, 0x8100 );	
	wm8994_write(codec,WM8994_GPIO_5, 0x8100 );	
	wm8994_write(codec,WM8994_GPIO_6, 0xA101 );	
	wm8994_write(codec,WM8994_GPIO_7, 0x0100 );

	// Analogue Output Configuration
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_3, 
		WM8994_MIXOUTLVOL_ENA | WM8994_MIXOUTRVOL_ENA |WM8994_MIXOUTL_ENA | WM8994_MIXOUTR_ENA);
	wm8994_write(codec, WM8994_HPOUT2_MIXER, WM8994_MIXOUTLVOL_TO_HPOUT2 |WM8994_MIXOUTRVOL_TO_HPOUT2); 
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, 
		WM8994_HPOUT2_ENA | WM8994_VMID_SEL_NORMAL | WM8994_BIAS_ENA);

	//	wm8994_write(codec, WM8994_INPUT_MIXER_4, 0x0000 ); 
}


void wm8994_set_voicecall_headset(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	int val;

	DEBUG_LOG("");
	
	/*DCS*/
	wm8994_write(codec,0x57, 0x9C9C );   
	wm8994_write(codec,0x55, 0x054E );   
	wm8994_write(codec,0x54, 0x300f );   
	wm8994_write(codec,0x5B, 0x0080 );   
	wm8994_write(codec,0x5C, 0x0080 );   
	/* Power - UP seqeunce- COLD Start Up*/
	wm8994_write(codec,0x39, 0x006C );   
	wm8994_write(codec,0x01, 0x0003 );   
	wm8994_write(codec,0x5B, 0x00FF );  
	wm8994_write(codec,0x5C, 0x00FF );  
	wm8994_write(codec,0x4C, 0x9F25 );  
	wm8994_write(codec,0x01, 0x0313 );  
	wm8994_write(codec,0x60, 0x0022 );  
	wm8994_write(codec,0x54, 0x333F );  
	wm8994_write(codec,0x60, 0x00EE );  
	/*Analogue Input Configuration*/
	wm8994_write(codec,0x02, 0x6110 );
	wm8994_write(codec,0x28, 0x0003 );	 
	
	/*Analogue Output Configuration*/	
	wm8994_write(codec,0x2D, 0x0100 );   
	wm8994_write(codec,0x2E, 0x0100 );   
	wm8994_write(codec,0x4C, 0x9F25 );   
	wm8994_write(codec,0x60, 0x00EE );   
	/*Digital Path Enables and Unmutes*/	
	wm8994_write(codec,0x04, 0x3003 );   
	wm8994_write(codec,0x05, 0x3303 );   
	wm8994_write(codec,0x420, 0x0000 );  
	wm8994_write(codec,0x520, 0x0000 );  
	wm8994_write(codec,0x601, 0x0005 );  
	wm8994_write(codec,0x602, 0x0005 );  
	wm8994_write(codec,0x603, 0x000C );  
	wm8994_write(codec,0x612, 0x01C0 );  
	wm8994_write(codec,0x613, 0x01C0 );  
	wm8994_write(codec,0x620, 0x0000 );  
	wm8994_write(codec,0x621, 0x01C1 );  

	// Tx -> AIF2 Path
	wm8994_write(codec, WM8994_DAC2_LEFT_MIXER_ROUTING, WM8994_ADC1_TO_DAC2L);	
	wm8994_write(codec, WM8994_DAC2_RIGHT_MIXER_ROUTING, WM8994_ADC1_TO_DAC2R);

	/*Clocking*/	
	wm8994_write(codec,0x208, 0x000E );  
	wm8994_write(codec,0x210, 0x0073 ); 
	wm8994_write(codec,0x211, 0x0003 );  
	/*AIF2 Interface*/	
	wm8994_write(codec,0x311, 0x0000 );
	wm8994_write(codec,0x312, 0x4000 );	//Master
	wm8994_write(codec,0x310, 0x4188 );	//Left Justified, BCLK invert, LRCLK Invert
	/*FLL 1 Setting*/		
	wm8994_write(codec,0x220, 0x0005 );	
	wm8994_write(codec,0x221, 0x0700 );	
	wm8994_write(codec,0x222, 0x86C2 );	
	wm8994_write(codec,0x224, 0x0C88 );	
	wm8994_write(codec,0x223, 0x00E0 );	
	wm8994_write(codec,0x200, 0x0011 ); 
	/*FLL2	Setting*/	
	wm8994_write(codec,0x240, 0x0005 );  
	wm8994_write(codec,0x241, 0x2F00 );  
	wm8994_write(codec,0x242, 0x3126 );  
	wm8994_write(codec,0x244, 0x0C88 );  
	wm8994_write(codec,0x204, 0x0019 );  
	wm8994_write(codec,0x243, 0x0100 );  
	/*GPIO Configuration*/	
	wm8994_write(codec,0x700, 0xA101 );  
	wm8994_write(codec,0x701, 0x8100 );  
	wm8994_write(codec,0x702, 0x8100 );  
	wm8994_write(codec,0x703, 0x8100 );  
	wm8994_write(codec,0x704, 0x8100 );  
	wm8994_write(codec,0x705, 0xA101 );  
	wm8994_write(codec,0x706, 0x0100 );  

	/* Unmute*/
	if(!wm8994->testmode_config_flag)
	{
		// Volume Control - Input
		val = WM8994_IN1R_VU;
		val &= ~WM8994_IN1R_MUTE_MASK;	// Unmute IN1R
		val |= WM8994_IN1R_VOL_21dB;
		wm8994_write(codec, WM8994_RIGHT_LINE_INPUT_1_2_VOLUME , val);

		wm8994_write(codec,WM8994_INPUT_MIXER_4, 
			WM8994_IN2R_TO_MIXINR | WM8994_IN1R_TO_MIXINR | WM8994_IN1R_MIXINR_VOL | 0x3 );   // -6dB

		// Volume Control - Output
	        val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5);
		val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
	        val |= TUNING_EAR_OUTMIX5_VOL << WM8994_DACL_MIXOUTL_VOL_SHIFT;
	        wm8994_write(codec,WM8994_OUTPUT_MIXER_5, val );
	
		val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6);
		val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
		val |= TUNING_EAR_OUTMIX6_VOL << WM8994_DACR_MIXOUTR_VOL_SHIFT;
		wm8994_write(codec,WM8994_OUTPUT_MIXER_6, val );

		val = wm8994_read(codec,WM8994_LEFT_OUTPUT_VOLUME);
		val &= ~(WM8994_HPOUT1L_MUTE_N_MASK | WM8994_HPOUT1L_VOL_MASK);
		val |= (WM8994_HPOUT1L_MUTE_N | TUNING_OUTPUTL_VOL);
		wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME, val);

		val = wm8994_read(codec,WM8994_RIGHT_OUTPUT_VOLUME);
		val &= ~(WM8994_HPOUT1R_MUTE_N_MASK | WM8994_HPOUT1R_VOL_MASK);
		val |= (WM8994_HPOUT1R_MUTE_N | TUNING_OUTPUTR_VOL);
		wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME, val);
	
		//Unmute DAC1 left
		val = wm8994_read(codec,WM8994_DAC1_LEFT_VOLUME );
		val &= ~(WM8994_DAC1L_MUTE_MASK | WM8994_DAC1L_VOL_MASK);
		val |= TUNING_DAC1L_VOL; 
		wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME ,val);
	
		//Unmute and volume ctrl RightDAC
		val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
		val &= ~(WM8994_DAC1R_MUTE_MASK | WM8994_DAC1R_VOL_MASK);
		val |= TUNING_DAC1R_VOL; //0 db volume	
		wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
	}
	
	/*DCS*/
	wm8994_write(codec,0x57, 0x9C9C );  
	wm8994_write(codec,0x55, 0x054E ); 
	wm8994_write(codec,0x54, 0x300f );  
	wm8994_write(codec,0x5B, 0x0080 ); 
	wm8994_write(codec,0x5C, 0x0080 ); 
}

void wm8994_set_voicecall_speaker(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	int val;

	DEBUG_LOG("");

	wm8994_write(codec,0x39, 0x006C );/*wm8994_write(codec,WM8994_ANTIPOP_2, WM8994_VMID_RAMP_ENA|WM8994_VMID_BUF_ENA|WM8994_STARTUP_BIAS_ENA );*/
	/*Analogue Configuration*/	
	wm8994_write(codec,0x01, 0x0003 );/*wm8994_write(codec,WM8994_POWER_MANAGEMENT_1, WM8994_VMID_SEL_NORMAL|WM8994_BIAS_ENA );*/   
	/*Analogue Input Configuration*/
	wm8994_write(codec,0x02, 0x6240 );/*wm8994_write(codec,WM8994_POWER_MANAGEMENT_2, WM8994_TSHUT_ENA|WM8994_TSHUT_OPDIS|WM8994_MIXINL_ENA|WM8994_IN1L_ENA );*/    
	wm8994_write(codec,0x28, 0x0010 );    

	/*Analogue Output Configuration*/	
	wm8994_write(codec,0x03, 0x0300 );    

	if(!wm8994->testmode_config_flag)
	{
		// Volume Control - Input
		wm8994_write(codec, WM8994_INPUT_MIXER_3, WM8994_IN1L_TO_MIXINL | WM8994_IN1L_MIXINL_VOL ); 	// IN1L -> MIXINL, 30dB
	
		val = WM8994_IN1L_VU;
		val &= ~WM8994_IN1L_MUTE_MASK;	// Unmute IN1L
		val |= WM8994_IN1L_VOL_30dB;
		wm8994_write(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME , val);
	
		// Volume Control - Output
		// Unmute the SPKMIXVOLUME
		val = wm8994_read(codec, WM8994_SPKMIXL_ATTENUATION);
		val &= ~(WM8994_SPKMIXL_VOL_MASK);
		val |= TUNING_SPKMIXL_ATTEN;	
		wm8994_write(codec, WM8994_SPKMIXL_ATTENUATION, val);
		
		val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
		val &= ~(WM8994_SPKMIXR_VOL_MASK);
		val |= TUNING_SPKMIXR_ATTEN;	
		wm8994_write(codec, WM8994_SPKMIXR_ATTENUATION, val);

		val = wm8994_read(codec,WM8994_SPEAKER_VOLUME_LEFT);
		val &= ~(WM8994_SPKOUTL_MUTE_N_MASK | WM8994_SPKOUTL_VOL_MASK);
		val |= (WM8994_SPKOUTL_MUTE_N | TUNING_SPKL_VOL);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);

		val = wm8994_read(codec,WM8994_SPEAKER_VOLUME_RIGHT);
		val &= ~(WM8994_SPKOUTR_MUTE_N_MASK | WM8994_SPKOUTR_VOL_MASK);
		val |= (WM8994_SPKOUTR_MUTE_N | TUNING_SPKR_VOL);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);

		val = wm8994_read(codec,WM8994_CLASSD);
		val &= ~(WM8994_SPKOUTL_BOOST_MASK);
		val |= TUNING_CLASSD_VOL << WM8994_SPKOUTL_BOOST_SHIFT;
		wm8994_write(codec, WM8994_CLASSD, val);
	
		//Unmute DAC1 left
		val = wm8994_read(codec,WM8994_DAC1_LEFT_VOLUME );
		val &= ~(WM8994_DAC1L_MUTE_MASK | WM8994_DAC1L_VOL_MASK);
		val |= TUNING_DAC1L_VOL; 
		wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME ,val);
	
		//Unmute and volume ctrl RightDAC
		val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
		val &= ~(WM8994_DAC1R_MUTE_MASK | WM8994_DAC1R_VOL_MASK);
		val |= TUNING_DAC1R_VOL; //0 db volume	
		wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
	}

	val = wm8994_read(codec,WM8994_SPKOUT_MIXERS );
	val |= WM8994_SPKMIXL_TO_SPKOUTL | WM8994_SPKMIXR_TO_SPKOUTL ;
	wm8994_write(codec,WM8994_SPKOUT_MIXERS,val );
	
	wm8994_write(codec,0x36, 0x0003 );    
	/* Digital Path Enables and Unmutes*/	
	wm8994_write(codec,0x04, 0x2002 );    
	wm8994_write(codec,0x05, 0x3303 );    
	wm8994_write(codec,0x520, 0x0000 );   
	wm8994_write(codec,0x601, 0x0005 );   
	wm8994_write(codec,0x602, 0x0005 );   
	wm8994_write(codec,0x603, 0x000C );   
	wm8994_write(codec,0x612, 0x01C0 );   
	wm8994_write(codec,0x613, 0x01C0 );   
	wm8994_write(codec,0x620, 0x0000 );   
	wm8994_write(codec,0x621, 0x01C0 );   
	wm8994_write(codec,0x200, 0x0001 );   

	// Tx -> AIF2 Path
	wm8994_write(codec, WM8994_DAC2_LEFT_MIXER_ROUTING, WM8994_ADC1_TO_DAC2L);	
	wm8994_write(codec, WM8994_DAC2_RIGHT_MIXER_ROUTING, WM8994_ADC1_TO_DAC2R);

//	wm8994_write(codec,0x204, 0x0009 );   
	/*Clocking*/		
	wm8994_write(codec,0x208, 0x000E );   
	wm8994_write(codec,0x210, 0x0073 );   
	wm8994_write(codec,0x211, 0x0003 ); 
	/*AIF2 Interface*/	
	wm8994_write(codec,0x311, 0x0000 );
	wm8994_write(codec,0x312, 0x4000 );   //Master
	wm8994_write(codec,0x310, 0x4188 );   //Left Justified, BCLK invert, LRCLK Invert
	/*FLL 1 Setting*/		
	wm8994_write(codec,0x220, 0x0005 );   
	wm8994_write(codec,0x221, 0x0700 );   
	wm8994_write(codec,0x222, 0x86C2 );   
	wm8994_write(codec,0x224, 0x0C88 );   
	wm8994_write(codec,0x223, 0x00E0 );   
	wm8994_write(codec,0x200, 0x0011 );   
	/*FLL2  Setting*/	
	wm8994_write(codec,0x240, 0x0005 );   
	wm8994_write(codec,0x241, 0x2F00 );   
	wm8994_write(codec,0x242, 0x3126 );   
	wm8994_write(codec,0x244, 0x0C88 );   
	wm8994_write(codec,0x204, 0x0019 );   
	wm8994_write(codec,0x243, 0x0100 );   
	/*GPIO Configuration*/	
	wm8994_write(codec,0x701, 0x8100 );   
	wm8994_write(codec,0x702, 0x8100 );   
	wm8994_write(codec,0x703, 0x8100 );   
	wm8994_write(codec,0x704, 0x8100 );   
	wm8994_write(codec,0x705, 0xA101 );   
	wm8994_write(codec,0x706, 0x0100 );   

	wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, 
		WM8994_SPKOUTL_ENA | WM8994_VMID_SEL_NORMAL | WM8994_BIAS_ENA);
}

void wm8994_set_voicecall_bluetooth(struct snd_soc_codec *codec)
{
	DEBUG_LOG("");

	wm8994_write(codec, WM8994_POWER_MANAGEMENT_6, 		// GPIO5/DACDAT2 and GPIO8/DACDAT3 select
		0x2 << WM8994_AIF3_ADCDAT_SRC_SHIFT |WM8994_AIF2_ADCDAT_SRC);	

	wm8994_write(codec, WM8994_CLOCKING_1, WM8994_DSP_FS1CLK_ENA | WM8994_DSP_FS2CLK_ENA | WM8994_DSP_FSINTCLK_ENA);

	wm8994_write(codec, WM8994_AIF2_CONTROL_1, 	//Left Justified, BCLK invert, LRCLK Invert
		WM8994_AIF2ADCR_SRC | WM8994_AIF2_BCLK_INV |WM8994_AIF2_LRCLK_INV | 0x1 << WM8994_AIF2_FMT_SHIFT);	
	wm8994_write(codec, WM8994_AIF2_CONTROL_2, 0x0000 );

	wm8994_write(codec, WM8994_AIF2_MASTER_SLAVE, WM8994_AIF2_MSTR | WM8994_AIF2_CLK_FRC | WM8994_AIF2_LRCLK_FRC);	// Master mode

	// GPIO Setting
	wm8994_write(codec, WM8994_GPIO_5, WM8994_GP5_DIR | WM8994_GP5_DB);	
	wm8994_write(codec, WM8994_GPIO_7, WM8994_GP7_DB);

	wm8994_write(codec, WM8994_GPIO_3, WM8994_GP3_DIR | WM8994_GP3_DB);	
	wm8994_write(codec, WM8994_GPIO_4, WM8994_GP4_DIR | WM8994_GP4_DB);
	wm8994_write(codec, WM8994_GPIO_6, WM8994_GP6_DIR | WM8994_GP6_PD | WM8994_GP6_DB |0x1 << WM8994_GP6_FN_SHIFT);

	wm8994_write(codec, WM8994_GPIO_8, WM8994_GP8_DIR | WM8994_GP8_DB);
	wm8994_write(codec, WM8994_GPIO_9, WM8994_GP9_DB);
	wm8994_write(codec, WM8994_GPIO_10, WM8994_GP10_DB);
	wm8994_write(codec, WM8994_GPIO_11, WM8994_GP11_DB);

	// Clocking
	wm8994_write(codec, WM8994_AIF2_RATE, 0x0003 << WM8994_AIF2CLK_RATE_SHIFT);
	wm8994_write(codec, WM8994_FLL2_CONTROL_5, 0x0C88); // REG 224 FLL1 Cntr5
	wm8994_write(codec, WM8994_FLL2_CONTROL_2, 0x2F00);	// REG 221 FLL1 Cntr2, FLL1 Setting
	wm8994_write(codec, WM8994_FLL2_CONTROL_3, 0x3126);	// REG 222 FLL1 Cntr3, K Value
	wm8994_write(codec, WM8994_FLL2_CONTROL_4, 0x0100);	// REG 223 FLL1 Cntr4, N Value
	wm8994_write(codec, WM8994_FLL2_CONTROL_1, WM8994_FLL2_FRACN_ENA | WM8994_FLL2_ENA);

	wm8994_write(codec, WM8994_AIF2_CLOCKING_1, 0x0019);   // REG 200 Enable AIF1 Clock, AIF1 Clock Source = FLL1
}
	
	
void wm8994_set_fmradio_headset(struct snd_soc_codec *codec)
{	
	struct wm8994_priv *wm8994 = codec->private_data;

	u16 val;
	
	DEBUG_LOG("Routing ear path : FM Radio -> EAR Out");

	wm8994_write(codec,0x101, 0x0004 );/*Control interface*/
	wm8994_write(codec,0x39, 0x006C ); 
	wm8994_write(codec,0x01, 0x0003 ); 
	
	if(!wm8994->testmode_config_flag)
	{
		wm8994_write(codec,0x29, 0x0100 );
		wm8994_write(codec,0x2A, 0x0100 );		

	        val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5);
		val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
	        val |= TUNING_EAR_OUTMIX5_VOL << WM8994_DACL_MIXOUTL_VOL_SHIFT;
	        wm8994_write(codec,WM8994_OUTPUT_MIXER_5, val );
	
		val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6);
		val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
		val |= TUNING_EAR_OUTMIX6_VOL << WM8994_DACR_MIXOUTR_VOL_SHIFT;
		wm8994_write(codec,WM8994_OUTPUT_MIXER_6, val );

		val = wm8994_read(codec,WM8994_LEFT_OUTPUT_VOLUME);
		val &= ~(WM8994_HPOUT1L_MUTE_N_MASK | WM8994_HPOUT1L_VOL_MASK);
		val |= (WM8994_HPOUT1L_MUTE_N | TUNING_OUTPUTL_VOL);
		wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME, val);

		val = wm8994_read(codec,WM8994_RIGHT_OUTPUT_VOLUME);
		val &= ~(WM8994_HPOUT1R_MUTE_N_MASK | WM8994_HPOUT1R_VOL_MASK);
		val |= (WM8994_HPOUT1R_MUTE_N | TUNING_OUTPUTR_VOL);
		wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME, val);
	}
		
	wm8994_write(codec,0x700, 0x0112 );
	wm8994_write(codec,0x02, 0x6800 ); 
	wm8994_write(codec,0x221, 0x0700 );
	wm8994_write(codec,0x224, 0x1140 );
	wm8994_write(codec,0x220, 0x0002 );
	wm8994_write(codec,0x200, 0x0011 );
	wm8994_write(codec,0x208, 0x000A );
	wm8994_write(codec,0x02, 0x6BA0 ); 
	wm8994_write(codec,0x03, 0x00F0 ); 
	wm8994_write(codec,0x19, 0x0115 ); 
	wm8994_write(codec,0x1B, 0x0115 ); 
	wm8994_write(codec,0x28, 0x0044 ); 
	wm8994_write(codec,0x2D, 0x0040 ); 
	wm8994_write(codec,0x2E, 0x0040 ); 
	wm8994_write(codec,0x4C, 0x9F25 ); 
						
	wm8994_write(codec,0x01, 0x0303 ); 
	wm8994_write(codec,0x60, 0x0022 ); 
	wm8994_write(codec,0x54, 0x0033 ); 
					
	wm8994_write(codec,0x60, 0x00EE ); 
}
	
void wm8994_set_fmradio_speaker(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	u16 val;

	DEBUG_LOG("Routing spk path : FM Radio -> SPK Out");
		
	wm8994_write(codec,0x101, 0x0004 );/*Control interface*/
	wm8994_write(codec,0x39, 0x006C ); /*Anti pop*/
	wm8994_write(codec,0x01, 0x0003 ); /*Anti pop*/
	
	if(!wm8994->testmode_config_flag)
	{	
		/*Input PGA-Input Mixer*/
		wm8994_write(codec,0x29, 0x0100 ); 
		wm8994_write(codec,0x2A, 0x0100 ); 

		// Unmute the SPKMIXVOLUME
		val = wm8994_read(codec, WM8994_SPKMIXL_ATTENUATION);
		val &= ~(WM8994_SPKMIXL_VOL_MASK);
		val |= TUNING_SPKMIXL_ATTEN;	
		wm8994_write(codec, WM8994_SPKMIXL_ATTENUATION, val);
			
		val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
		val &= ~(WM8994_SPKMIXR_VOL_MASK);
		val |= TUNING_SPKMIXR_ATTEN;	
		wm8994_write(codec, WM8994_SPKMIXR_ATTENUATION, val);
	
		val = wm8994_read(codec,WM8994_SPEAKER_VOLUME_LEFT);
		val &= ~(WM8994_SPKOUTL_MUTE_N_MASK | WM8994_SPKOUTL_VOL_MASK);
		val |= (WM8994_SPKOUTL_MUTE_N | TUNING_SPKL_VOL);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
	
		val = wm8994_read(codec,WM8994_SPEAKER_VOLUME_RIGHT);
		val &= ~(WM8994_SPKOUTR_MUTE_N_MASK | WM8994_SPKOUTR_VOL_MASK);
		val |= (WM8994_SPKOUTR_MUTE_N | TUNING_SPKR_VOL);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
	
		val = wm8994_read(codec,WM8994_CLASSD);
		val &= ~(WM8994_SPKOUTL_BOOST_MASK);
		val |= TUNING_CLASSD_VOL << WM8994_SPKOUTL_BOOST_SHIFT;
		wm8994_write(codec, WM8994_CLASSD, val);
		
	}

	val = wm8994_read(codec,WM8994_SPKOUT_MIXERS );
	val |= WM8994_SPKMIXL_TO_SPKOUTL | WM8994_SPKMIXR_TO_SPKOUTL ;
	wm8994_write(codec,WM8994_SPKOUT_MIXERS,val );
	
	/*Path enable*/ 				
	wm8994_write(codec,0x01, 0x1003 ); /*SPKOUT L enable*/					
	wm8994_write(codec,0x700, 0x0112 );/*GPIO1*/
	wm8994_write(codec,0x02, 0x6800 ); /*Power management*/
	/*FLL1*/	
	wm8994_write(codec,0x221, 0x0700 );
	wm8994_write(codec,0x224, 0x1140 );
	wm8994_write(codec,0x220, 0x0002 );
	wm8994_write(codec,0x200, 0x0011 );
	/*Clocking*/	
	wm8994_write(codec,0x208, 0x000A );
	/*Analogue Input Configuration*/
	wm8994_write(codec,0x02, 0x6BA0 ); /*MIXIN L-R, IN2 L-R*/
	wm8994_write(codec,0x03, 0x0300 ); /*SPK_Vol_EN L-R*/
		
	/*Output MIxer-Output PGA*/
	wm8994_write(codec,WM8994_SPKOUT_MIXERS, 0x0018 );/*SPKMIX L-R, SPKOUT L*/	
	/*Input volume*/	
	wm8994_write(codec,0x19, 0x0115 ); 
	wm8994_write(codec,0x1B, 0x0115 ); 
	/*Input-Input PGA*/ 
	wm8994_write(codec,0x28, 0x0044 ); 
	/*DAC-Output Mixer*/	
	wm8994_write(codec,0x36, 0x00C0 ); /*MIXIN L-R, SPKMIX L-R*/	
}

