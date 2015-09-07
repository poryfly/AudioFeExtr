% function dist = main(filename1,filename2)
% fprintf('%s&&%s\n',filename1,filename2)
%clc;
clear;
version='MyPhi_v2.7_start';

tic

method = 10;

        targf = 4000;     
%         FrameLen=2048; distunit = 16.201389  %for philips,method 10,5000,2048,1/2;        
%         FrameLen=2048;  distunit = 15.989583 %for philips,method 10,5000,2048,1/4;
       % FrameLen=2048;  distunit = 16.018503 %for philips,method 10,5000,2048,1/33;
        FrameLen=2048;  distunit = 16.012752 %for philips,method 10,5000,2048,1/33;,for targf = 4000


forsearch = 0;
%% 不同音频
[x fs1]=wavread('..\..\exe-test\歌曲1-最炫民族风-30s.wav');
[y fs2]=wavread('..\..\exe-test\乐曲1-Exodus.wav');
% % % % % % 
% [x fs1]=wavread('..\..\exe-test\歌曲1-最炫民族风-30s.wav');
% [y fs2]=wavread('..\..\exe-test\乐曲2-英雄的黎明.wav');
% % % % % % % % % % % % nkm   
% % % % % % % % % % % % % 
% [x fs1]=wavread('..\..\exe-test\歌曲1-最炫民族风-30s.wav');
% [y fs2]=wavread('..\..\exe-test\广告1-男声-蓝光整合.wav');
% % % % % % % % % % % % % 
% [x fs1]=wavread('..\..\exe-test\歌曲1-最炫民族风-30s.wav');
% [y fs2]=wavread('..\..\exe-test\广告2-女声-宝中旅游大理.wav');
% % % % % % % % % % 
% [x fs1]=wavread('..\..\exe-test\歌曲1-最炫民族风-30s.wav');
% [y fs2]=wavread('..\..\exe-test\歌曲2-小苹果-30s.wav');
%% 相似音频
% 歌曲1与歌曲1-pop
% 歌曲1与歌曲1-rock
% 歌曲1与歌曲1-classsical
% 歌曲1与歌曲1-转32MP3
% 歌曲1与歌曲1-转64MP3
% 歌曲1与歌曲1转128MP3
% 歌曲1与歌曲1-转256MP3
% 歌曲1与歌曲1-SNR50
% 歌曲1与歌曲1-SNR25
% 歌曲1与歌曲1-SNR10
% 
% [x fs1]=wavread('..\..\exe-test\歌曲1-最炫民族风-30s.wav');
% [y fs2]=wavread('..\..\exe-test\歌曲1-最炫民族风-30s-pop-高低音加重中音减弱.wav');
% % % % % % % % % % 
% % % % % % % % % % % 
% [x fs1]=wavread('..\..\exe-test\歌曲1-最炫民族风-30s.wav');
% [y fs2]=wavread('..\..\exe-test\歌曲1-最炫民族风-30s-rock-高低音加重中音减弱.wav');
% % % % % % % % % 
% % [x fs1]=wavread('..\..\exe-test\歌曲1-最炫民族风-30s.wav');
% % [y fs2]=wavread('..\..\exe-test\歌曲1-最炫民族风-30s-classical-高低音加重中音减弱.wav');
% % % % % % % % % % % % % % % % 
% % [x fs1]=wavread('..\..\exe-test\歌曲1-最炫民族风-30s.wav');
% % [y fs2]=wavread('..\..\exe-test\歌曲1-最炫民族风-30s-256.wav');
% % % % % % % 
% [x fs1]=wavread('..\..\exe-test\歌曲1-最炫民族风-30s.wav');
% [y fs2]=wavread('..\..\exe-test\歌曲1-最炫民族风-30s-snr5.wav');
% % % % % % % % % 
% [x fs1]=wavread('..\..\exe-test\BKADl-48k-m.wav');
% [y fs2]=wavread('..\..\exe-test\BKADr-48k-m.wav');
% % % % % % % % % % % % % % % % % % % % 
% [x fs1]=wavread('..\..\exe-test\BKADr-48k-m.wav');
% [y fs2]=wavread('..\..\exe-test\BKADr-48k-m-change4.wav');
% % % 
% 
% % % % % % % 
% 

if fs1~=targf
    x=resample(x,targf,fs1);
    fs1=targf;
end
if fs2~=targf
    y=resample(y,targf,fs2);
    fs2=targf;
end

% % %filter range
maxfrq=2000;
minfrq=0;

if forsearch 
    y = [zeros(10,1);y];
    cnt = 15;
else
    cnt = 1;
end
mean_dist = zeros(1,cnt);
for k=1:cnt
      
        BKADrorg= FeatureExtraction_Philips(x,FrameLen,fs1);
        BKADlorg= FeatureExtraction_Philips(y,FrameLen,fs2);
       % BKADrorg= FeatureExtraction_Philips_1div4(x,FrameLen,fs1);
       % BKADlorg= FeatureExtraction_Philips_1div4(y,FrameLen,fs2);
        mean_dist(k)=Dist_hanmin(BKADrorg,BKADlorg);
    
        y = y(11:end);
end
 toc;
 
 fprintf('%d,归一后=%d\n',mean_dist(1),mean_dist(1)/distunit);
 fprintf('%f,归一后=%7f\n\n\n',mean_dist(1),mean_dist(1)/distunit);
 
 if forsearch 
 mean_dist = mean_dist/distunit
 stem(mean_dist);
 title('rock 256 1/2');
 end

