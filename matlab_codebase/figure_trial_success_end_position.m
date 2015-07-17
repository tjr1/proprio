
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
    dt_trials = 30;
    y_max_size = 0.03;
    for i = 1:dt_trials:num_trials
        if bW
            patch([i i+dt_trials i+dt_trials i], [y_max_size y_max_size -y_max_size -y_max_size], ...
                  'k', 'edgecolor', 'none', 'facealpha', 0.3);
            bW = false;
        else
            patch([i i+dt_trials i+dt_trials i], [y_max_size y_max_size -y_max_size -y_max_size], ...
                  'w', 'edgecolor', 'none', 'facealpha', 0.1);
            bW = true;
        end
    end
    
    for i = 1:num_trials
        plot(i, x_pos(trial_success_indices(i)), 'b*')      
        plot(i, x_pos(trial_end_indices(i)), 'r*')        
                
    end
    
    ylim([-y_max_size y_max_size])
    
    