function y = high_pass_filter(x, freq, sampling_Rate)

    f_0 = sampling_Rate;    % sampling rate of waveform (Hz)
    f_stop = freq;          % stop frequency (Hz)
    f_Nyquist = f_0/2;      % the Nyquist limit
    n = length(x);

    % Fourier Filtering
    f_all = linspace(-f_Nyquist,f_Nyquist,n);
    desired_response = ones(n,1);
    desired_response(abs(f_all)<=f_stop) = 0;

    y = real((ifft(fft(x).*fftshift(desired_response))));
    
end