# Machine Support

snd-soc-smdkc100-wm8580-objs  := smdkc100_wm8580.o
snd-soc-smdkc100-s5m8751-objs := smdkc100_s5m8751.o
snd-soc-smdkc100-wm9713-objs  := smdkc100_wm9713.o

snd-soc-universal-ak4671-objs := universal_ak4671.o
snd-soc-universal-wm8994-objs := universal_wm8994.o

snd-soc-smdk6442-wm8580-objs  := smdk6442_wm8580.o
snd-soc-smdkc110-pcm-wm8580-objs := smdkc110_pcmdev_wm8580.o
snd-soc-universal-pcm-wm8994-objs := universal_pcm_wm8994.o

obj-$(CONFIG_SND_S3C_I2S_V50)  += s3c-pcm-lp.o
obj-$(CONFIG_SND_S3C_I2S_V50)  += s3c-i2s.o

obj-$(CONFIG_SND_S3C_AC97)     += s3c-ac97.o
obj-$(CONFIG_SND_S3C_AC97)     += s3c-pcm.o
obj-$(CONFIG_SND_S3C_PCMDEV)   += s3c-pcm.o
#obj-$(CONFIG_SND_S3C_PCMDEV)   += s3c-pcm-lp.o
obj-$(CONFIG_SND_S3C_PCMDEV)   += s3c-pcmdev.o

# S5PC100 has AC97 and I2S(v-3.2 and 5.0) controller.
# SMDKC100 has WM8580 and WM9713 onboard and S5M8751 as a module.
obj-$(CONFIG_SND_SMDKC100_S5M8751) += snd-soc-smdkc100-s5m8751.o
obj-$(CONFIG_SND_SMDKC100_WM8580)  += snd-soc-smdkc100-wm8580.o
obj-$(CONFIG_SND_SMDKC100_WM9713)  += snd-soc-smdkc100-wm9713.o

obj-$(CONFIG_SND_SMDKC110_WM8580)  += snd-soc-smdkc100-wm8580.o
obj-$(CONFIG_SND_UNIVERSAL_WM8994)  += snd-soc-universal-wm8994.o

obj-$(CONFIG_SND_SMDK6442_WM8580)  += snd-soc-smdk6442-wm8580.o

obj-$(CONFIG_SND_S5PC100_SOC_UNIVERSAL_AK4671) += snd-soc-universal-ak4671.o

obj-$(CONFIG_SND_SMDKC110_WM8580_PCM)   +=snd-soc-smdkc110-pcm-wm8580.o
obj-$(CONFIG_SND_C110_PCM)      +=snd-soc-universal-pcm-wm8994.o
