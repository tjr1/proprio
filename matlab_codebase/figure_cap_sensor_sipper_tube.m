
clear all
% close all
clc

SUBJECT_ID = 3;
SESSION_ID = 26;

automatorParsing();

%% PLOT SIPPER (ONLY WORKS FOR SUBJECT 3, SESSION 2)
figure
    hold all
    grid on
        
    % Analyze Each Trial
%     for index = 2:num_trials,
% 
%         trial_indices = trial_start_indices(index) + [1:trial_length(index)];
%         trial_indices = trial_indices(1:end-1);
% 
%         %line_h = plot(time_stamps(trial_indices)/1000, cap_sen_lick(trial_indices));
%         %line2_h = plot(time_stamps(trial_indices)/1000, cap_sen_handle(trial_indices)); %, 'color', get(line_h, 'color'))        
%     end
    
    % Analyze Each Trial
    for index = 2:num_trials,
        trial_indices = trial_start_indices(index) + [1:trial_length(index)];
        trial_indices = trial_indices(1:end-1);

        xdata = time_stamps(trial_indices) - time_stamps(trial_indices(1));
        ydata = cap_sen_handle(trial_indices);
        
        ydata_mask = ydata > 110;
        ydata_mask = diff([0; ydata_mask]);
        ydata_mask(ydata_mask < 0) = 0;
        ydata_mask = logical(ydata_mask);
        
        %plot(xdata, ydata);
        %plot(xdata(ydata_mask), ydata(ydata_mask), 'g*');
        plot(xdata(ydata_mask), ones(1,sum(ydata_mask))+index, 'r*');
        
    end
    
    xlim([0 40])
    xlabel('Time [seconds]')
    
    ylabel('Trial Number')
    
    title('Lick Raster')