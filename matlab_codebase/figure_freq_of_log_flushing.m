
clear all
% close all
clc

SUBJECT_ID = 3;
SESSION_ID = 26;

automatorParsing();

%% FIGURE 2 - analyizing frequency of flushing the logger file
fig = figure;
fig_pos = get(fig,'Position');
set(fig, 'Position', [1 1 4/3 .5*4/3].*fig_pos);
    
trial_num = 2;

subplot(1,2,1)
    hold all
    grid on
    
    % Analyize Each Trial
    for index = 1,

        trial_indices = trial_start_indices(index) + [1:trial_length(index)];
        trial_indices = trial_indices(1:end-1);

        plot(time_stamps)
        
    end