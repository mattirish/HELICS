function v = helics_log_level_trace()
  persistent vInitialized;
  if isempty(vInitialized)
    vInitialized = helicsMEX(0, 45);
  end
  v = vInitialized;
end
