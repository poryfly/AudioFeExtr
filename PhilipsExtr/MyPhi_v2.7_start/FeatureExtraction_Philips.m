
function coef = FeatureExtraction_Philips(input,framelen,fs)

startf = 300;
endf = 2000;
bandnum = 33;
f = GetfreqBand(bandnum,startf,endf);
bin = [Freq2Bin(startf,fs,framelen) Freq2Bin(f,fs,framelen)];
E = zeros(bandnum,1);

%half_framelen = 0.5*framelen;
datasize = size(input,1);
psize = floor(datasize/framelen)*framelen;

non_overlap = floor(1/33*framelen);%非重叠部分为帧长的1/33
x=enframe(input(1:psize),framelen,non_overlap);%分帧，每帧长FrameLen，
[NumFrame,framelen]=size(x);
diffE = zeros(NumFrame,bandnum-1);
par = zeros(NumFrame-1,bandnum-1);
coef = par;

for i=1:NumFrame

    yy=x(i,:);
    y = yy/max(abs(yy));
    s=y'.*(hamming(framelen)); %hamming 窗
    
    t=abs(fft(s)); %傅立叶变换
    t=t.^2; %功率普
%     m(i,:)=filtMgt*t(1:half_framelen);%滤波处理
    for j=1:bandnum
     E(j) = sum(t(bin(j):bin(j+1))); 
    end
    
    for j=1:bandnum-1
     diffE(i,j) =  E(j) - E(j+1); 
     if i>1
     par(i-1,j) = diffE(i,j) - diffE(i-1,j);
     coef(i-1,j) = par(i-1,j)>0;
     %[par(i-1,j) coef(i-1,j)]
     end
    end
    
end
coef = coef';
end
