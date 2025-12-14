# Set thread scheduling policy to "All processors" (0)
$subgroup = "54533251-82be-4824-96c1-47b60b740d00"
$setting = "bae08b81-2d5e-4688-ad6a-13243356654b"

powercfg /setacvalueindex SCHEME_CURRENT $subgroup $setting 0
powercfg /setdcvalueindex SCHEME_CURRENT $subgroup $setting 0
powercfg /setactive SCHEME_CURRENT
