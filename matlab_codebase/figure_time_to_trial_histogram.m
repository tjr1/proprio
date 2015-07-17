
clear all
% close all
clc

SUBJECT_ID = 3;
SESSION_ID = 26;

automatorParsing();

%% Time to Trial
figure
    hold all
    grid on
        
    % Analyze Each Trial
    for index = 2:num_trials-1,

        start_ind = trial_start_indices(index);
        end_ind = trial_start_indices(index+1);
        
        trial_length_ms(index) = time_stamps(end_ind-1) - time_stamps(start_ind+1);
    end
    
    plot([2:num_trials-1], trial_length_ms(2:end), 'r*')
    


