function mean_dist= Dist_hanmin( BKADrorg,BKADlorg )
%UNTITLED Summary of this function goes here
%   Detailed explanation goes here
%threshold
thr = 1;

dim1 = size(BKADrorg);
dim2 = size(BKADlorg);


%帧数不一取小的
frame_num=min(dim1(2),dim2(2));
feature_num=dim1(1);

%帧数起始点
startp=1;
endp=frame_num;


for j=startp:endp
    m = 0;
    for i=1:feature_num%mfcc coef
%          plot(BKADlorg(:,j),'r');
%          hold on
%          plot(BKADrorg(:,j),'b');
%         if BKADlorg(i,j)>=thr && BKADrorg(i,j)>=thr
%           %   [BKADlorg(i,j) BKADrorg(i,j)]
%             temp = 0;
%         else
%           %  [BKADlorg(i,j) BKADrorg(i,j)]
%             temp = BKADlorg(i,j)-BKADrorg(i,j);
%             temp = temp*temp;
%         end
%         
        temp = BKADlorg(i,j)~=BKADrorg(i,j);
        %[temp BKADlorg(i,j) BKADrorg(i,j)]
        %temp = temp*temp;
        m = m+temp;
    end;
%    dist(j) = sqrt(m);
    dist(j) = m;

 end;


mean_dist = sum(dist)/frame_num;
%stem(dist)
end

