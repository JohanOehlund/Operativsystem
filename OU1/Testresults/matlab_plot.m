%--------------------------------------------------------------------------
% Script: plot_result_effektiva
% Plots the time against the number of characters.
%--------------------------------------------------------------------------
% Authors:
% Johan Öhlund(c15jod@cs.umu.se)
%--------------------------------------------------------------------------
func_nlog=@(x) x.*log(x);
func_linear=@(x) x;
func_quad=@(x) x.^2;
func_cubic=@(x) 10.*x.^3;
func_2n=@(x) 2.^x;
fileID = fopen('result9.txt','r');
formatSpec = '%d %d %f %f %f';
sizeA = [5 Inf];

A = fscanf(fileID,formatSpec,sizeA);
A=A';
steps=      A(:,1);
stepsNaive= A(:,2);
resNaive=   A(:,3)*0.000001;
resTopDown= A(:,4)*0.000001;
resBottomUp=A(:,5)*0.000001;
%resNaive=   A(:,3);
%resTopDown= A(:,4);
%resBottomUp=A(:,5);
fclose(fileID);

hold on
grid on
plotNaive=plot(stepsNaive,resNaive);
plot2n=plot(stepsNaive,func_2n(stepsNaive));

title('Result - False Parentheses');
ylabel('Operaions');
xlabel('String length');
legend('Naive','2^n');
hold off

figure
title('Result - False Parentheses');
ylabel('Time (ms)');
xlabel('String length');
hold on

plotTopDown=plot(steps,resTopDown);
%plotBottomUp=plot(steps,resBottomUp);
plotLinear = plot(steps,func_linear(steps));
plotQuad = plot(steps,func_quad(steps));
plotCubic = plot(steps,func_cubic(steps));
legend('BottomUp','Linear','Quad','Cubic');
grid on

