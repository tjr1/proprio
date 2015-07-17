
%% ANALYSIS FILE FOR DATA GENERATED WITH 'trainerBot'

if ~exist('SUBJECT_ID')
    SUBJECT_ID = 3;
end
if ~exist('SESSION_ID')   
    SESSION_ID = 23;
end

%% Find Files
cur_dir = pwd;
[pathstr,name,ext] = fileparts(cur_dir);
fullDataPath = [pathstr '\codeBase\data'];

subjects = dir(fullDataPath);
valid_names = strfind({subjects.name}, 'SUBJECT_');
valid_ind = ~cellfun(@isempty,valid_names);
subjects = subjects(valid_ind);


%% Subjects
for subjectID = subjects(SUBJECT_ID),
    
    subjectPath = [fullDataPath '\' subjectID.name];
    sessions = dir(subjectPath);
    sessions = sessions(3:end);
    
    % Sessions
    for sessionID = sessions(SESSION_ID),
        
        sessionPath = [subjectPath '\' sessionID.name];
        dataFile = importdata(sessionPath);
    end
end


%% Analyize Trials

% Basic Vars
num_fields = size(dataFile.data,2);
num_data_pts = size(dataFile.data,1);

% Grab Field Names
% name_fields = strsplit(dataFile.textdata{3}, {',', ':'});
% name_fields = name_fields(end);
name_fields = regexp (dataFile.textdata{3}, ':', 'split');
name_fields = regexp (name_fields{2}, ',', 'split');

% Trial Start Indices
trial_start_indices = find(dataFile.data(:,ismember(name_fields, 'TIME_STAMP')) == 0)';
trial_length = diff([trial_start_indices, num_data_pts]);
trial_end_indices = bsxfun(@plus, trial_start_indices, trial_length)-1;
num_trials = length(trial_start_indices);

% State Machine
trial_success_indices = [];
state_machine = dataFile.data(:,ismember(name_fields, 'STATE_MACHINE'));
for i = 1:num_trials
    ind_S = find(state_machine([trial_start_indices(i):(trial_start_indices(i)+trial_length(i))]) == 2,1);
    if isempty(ind_S)
        trial_success_indices(i) = trial_end_indices(i);
    else
        trial_success_indices(i) = ind_S + trial_start_indices(i);
    end
end

trial_num_failure = [];
for i = 1:num_trials
    state_vector = state_machine([trial_start_indices(i):(trial_start_indices(i)+trial_length(i))]);
    state_vector = state_vector([true;diff(state_vector)~=0]);
    ind_F = find(state_vector == 77);
    if isempty(ind_F)
        trial_num_failure(i) = 0;
    else
        trial_num_failure(i) = numel(ind_F);
    end
end


% Time Stamps
time_stamps = dataFile.data(:,ismember(name_fields, 'TIME_STAMP'))./1000;
sampling_rate = 1/mean(diff(time_stamps/1000));

% Reward
reward = dataFile.data(:,ismember(name_fields, 'REWARD'))';
reward = diff([0 reward]);
reward(reward < 0) = 0;
reward = logical(reward);

% Falcon Position Data
y_dim = find(ismember(name_fields, 'POS_Z'));
x_dim = find(ismember(name_fields, 'POS_Y'));

x_pos = dataFile.data(:,x_dim);
y_pos = dataFile.data(:,y_dim);

% Sipper Tube & Handle
cap_sen_lick   = dataFile.data(:,ismember(name_fields, 'CAP_SENSOR_SIPPER'));
cap_sen_handle = dataFile.data(:,ismember(name_fields, 'CAP_SENSOR_HANDLE'));

filt_cap_sen_lick = high_pass_filter(cap_sen_lick, 200, sampling_rate);


