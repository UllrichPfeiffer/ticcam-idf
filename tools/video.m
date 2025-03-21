%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Example MATLAB Script to read mat file
% For TW_ID02 Cam
% TicWave Solutions GmbH
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
close all;
clear all;
clc;
whos('-file', 'video.mat');

load('video.mat', 'data');

% Determine the number of image
dimSizes = size(data);
numImages = dimSizes(1);

% loop images
for i = 1 : numImages
imagesc(squeeze(data(i, :, :))); colorbar(); title('Recorded Raw Data');  
    drawnow;
end
