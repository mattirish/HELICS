function v = helics_error_execution_failure()
  persistent vInitialized;
  if isempty(vInitialized)
    vInitialized = helicsMEX(0, 1946183042);
  end
  v = vInitialized;
end