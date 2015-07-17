
clear all
% close all
clc

SUBJECT_ID = 3;
SESSION_ID = 23;

automatorParsing();

%% FIGURE

fig = figure;
fig_pos = get(fig,'Position');
set(fig, 'Position', [1 1 1 1].*fig_pos);

    hold on
    grid on
    
    bW = true;
    dt_trials = 20;
    y_max_size = max(trial_num_failure);
    for i = 1:dt_trials:num_trials
        if bW
            color = 'k'; bW = false;
        else
            color = 'w'; bW = true;
        end
        patch([i i+dt_trials i+dt_trials i], [y_max_size y_max_size -y_max_size -y_max_size], ...
              color, 'edgecolor', 'none', 'facealpha', 0.3);
    end
    
    for i = 1:num_trials
        plot(i, trial_num_failure(i), 'r*')        
        
    end
    
    ylim([0 y_max_size])
    
    