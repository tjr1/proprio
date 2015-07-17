
clear all
% close all
clc

SUBJECT_ID = 3;
SESSION_ID = 23;

automatorParsing();

%% FIGURE 1
fig = figure;
fig_pos = get(fig,'Position');
set(fig, 'Position', [1 1 4/3 .5*4/3].*fig_pos);

% PLOT REWARD
subplot(1,2,1)
    hold on
    grid on
    
    plot(time_stamps(reward), cumsum(reward(reward)), 'b', 'linewidth', 2)

    xlabel('Time [seconds]')
    ylabel('Rewards Delievered')


% PLOT TARGETS
subplot(1,2,2)
    hold on
    grid on
    axis equal

    % Analyize Each Trial
    for index = 2:num_trials,

        trial_indices = trial_start_indices(index) + [1:trial_length(index)];
        trial_indices = trial_indices(1:end-1);

        grey_h = plot(x_pos(trial_indices)+0.001, y_pos(trial_indices)+0.001, 'color', [0.5 0.5 0.5]);

    end

    for index = 2:num_trials,

        trial_indices = trial_start_indices(index) + [1:trial_length(index)];
        trial_indices = trial_indices(1:end-1);

        % Start Position
        red_x_h = plot(x_pos(trial_indices(1)), y_pos(trial_indices(1)), ...
                         'linestyle', 'x', ...
                         'linewidth', 2, ...
                         'color', [0 1 0]);

        % End Position
        black_x_h = plot(x_pos(trial_indices(end)), y_pos(trial_indices(end)), ...
                         'linestyle', 'x', ...
                         'linewidth', 2, ...
                         'color', [1 0 0]);
    end
    
%     xlim([-0.005 0.025])
%     xlim([-0.015 0.015])
%     ylim(bsxfun(@plus, xlim, -0.025));
    
    axlim = axis;
    plot(axlim(1:2), [-0.02 -0.02], 'b:', 'linewidth', 3);
    plot([0 0], axlim(3:4), 'b:', 'linewidth', 3);
       
    xlabel('X Axis [millimeters]') 
    ylabel('Y Axis [millimeters]')
    
    legend([grey_h(1), red_x_h(1), black_x_h(1)], {'Handle Trajectory', 'Start Position', 'End Position'});