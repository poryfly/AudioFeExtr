# AudioFeExtr
MFCCExtr：wav,mp2音频MFCC特征提取程序，常规MFCC（梅尔系数）特征
HFCCExtr：wav,mp2音频HFCC特征提取程序,在MFCC的框架下，去掉DCT，修改滤波器组，改用HFF滤波器，突出人声的特性，抑制乐器的声音
MFCC_walshExtr:wav,mp2音频HFCC特征提取程序,在MFCC的框架下,替换fft，改为walsh变换，抗噪性增强
PhilipsExtr：matlab程序，提取philips音频指纹
