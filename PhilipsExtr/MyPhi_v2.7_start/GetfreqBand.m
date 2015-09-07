function z = GetfreqBand(bandnum,startf,endf)

log_startf = log10(startf); 
log_endf = log10(endf);

y = log_endf-log_startf;

z = 10.^([1:bandnum]*y/bandnum +log_startf);
z = floor(z);